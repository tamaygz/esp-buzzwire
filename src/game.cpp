#include "game.h"
#include "config.h"
#include "game_config.h"
#include "scoreboard.h"
#include "sensors.h"
#include "leds.h"
#include "matrix.h"
#include "promode.h"

// ── Internal State ──────────────────────────────────────────────────────────
static GameState currentState   = STATE_IDLE;
static unsigned long timerStart = 0;
static unsigned long stateStart = 0;
static int failCount            = 0;
static unsigned long elapsed    = 0;
static int countdownStep        = 3;

// ── Per-State One-Shot Flags ─────────────────────────────────────────────────
// These are reset in enterState() and set the first time their action fires.
static bool buzzerFired   = false;
static char winMsg[32]    = {};
static char lastProLetter = '\0';
static int  lastElapsedSec = -1;

// ── State Change Callback ──────────────────────────────────────────────────
static void (*sStateChangeCb)(GameState state, unsigned long elapsed, int fails) = nullptr;

void gameOnStateChange(void (*cb)(GameState state, unsigned long elapsed, int fails)) {
    sStateChangeCb = cb;
}

// ── Remote Control Flags ─────────────────────────────────────────────────
static bool remoteStart         = false;
static bool remoteReset         = false;
static bool remoteSimWire       = false;
static bool remoteSimFinish     = false;

void gameRemoteStart()       { remoteStart      = true; }
void gameRemoteReset()       { remoteReset      = true; }
void gameRemoteSimWire()     { remoteSimWire    = true; }
void gameRemoteSimFinish()   { remoteSimFinish  = true; }

// ── State Name Helper ────────────────────────────────────────────────────────
const char* gameGetStateName(GameState s) {
    switch (s) {
        case STATE_IDLE:      return "IDLE";
        case STATE_COUNTDOWN: return "COUNTDOWN";
        case STATE_PLAYING:   return "PLAYING";
        case STATE_FAIL:      return "FAIL";
        case STATE_WIN:       return "WIN";
        default:              return "UNKNOWN";
    }
}

// ── Setup ───────────────────────────────────────────────────────────────────
void gameSetup() {
    currentState   = STATE_IDLE;
    timerStart     = 0;
    stateStart     = millis();
    failCount      = 0;
    elapsed        = 0;
    countdownStep  = 3;
    buzzerFired    = false;
    lastProLetter  = '\0';
    lastElapsedSec = -1;
}

// ── State Transition Helper ──────────────────────────────────────────────────
// Resets the per-state timer, clears the matrix scroll position, resets
// one-shot flags, and logs the transition so it is always visible in Serial.
static void enterState(GameState newState) {
    Serial.print(F("[GAME] "));
    Serial.print(gameGetStateName(currentState));
    Serial.print(F(" → "));
    Serial.println(gameGetStateName(newState));

    currentState = newState;
    stateStart   = millis();
    matrixScrollReset();

    buzzerFired    = false;
    lastProLetter  = '\0';
    lastElapsedSec = -1;

    if (newState == STATE_WIN) {
        snprintf(winMsg, sizeof(winMsg), "WIN! %lus %df",
                 elapsed / 1000UL, failCount);
        Serial.print(F("[GAME] WIN message: "));
        Serial.println(winMsg);
        scoreboardAdd(elapsed, failCount);
    }

    if (newState == STATE_FAIL || newState == STATE_WIN || newState == STATE_IDLE) {
        digitalWrite(PRO_RED_LED_PIN,   LOW);
        digitalWrite(PRO_GREEN_LED_PIN, LOW);
    }

    if (sStateChangeCb) sStateChangeCb(newState, elapsed, failCount);
}

// ── IDLE State ──────────────────────────────────────────────────────────────
static void handleIdle() {
    ledsIdle();
    matrixScrollText("TOUCH START", CRGB::Cyan, cfg.scrollIdleMs);

    if (remoteStart || isStartTouched()) {
        remoteStart   = false;
        failCount     = 0;
        elapsed       = 0;
        countdownStep = 3;
        ledsClear();
        enterState(STATE_COUNTDOWN);
    }
}

// ── COUNTDOWN State ─────────────────────────────────────────────────────────
static void handleCountdown() {
    unsigned long now      = millis();
    int           stepIndex = (int)((now - stateStart) / COUNTDOWN_STEP_MS);

    if (stepIndex < 3) {
        int digit = 3 - stepIndex;
        if (countdownStep != digit) {
            countdownStep = digit;
            Serial.print(F("[GAME] Countdown: "));
            Serial.println(digit);
        }
        matrixShowNumber(digit, CRGB::Yellow);
        ledsCountdown(digit);
    } else if (stepIndex == 3) {
        if (countdownStep != 0) {
            countdownStep = 0;
            Serial.println(F("[GAME] GO!"));
        }
        matrixScrollText("GO!", CRGB::Green, cfg.scrollCountdownMs);
        ledsCountdown(0);
    } else {
        // Countdown complete → PLAYING
        timerStart = millis();
        ledsClear();

#if PRO_MODE_ENABLED
        // Only calibrate IR when it is actually used (avoids ~64ms delay for PIR-only builds)
  #if (PRO_MODE_SENSOR == SENSOR_IR) || (PRO_MODE_SENSOR == SENSOR_BOTH)
        irCalibrate();
  #endif
        promodeReset();
#endif

        enterState(STATE_PLAYING);
    }
}

// ── PLAYING State ───────────────────────────────────────────────────────────
static void handlePlaying() {
    unsigned long now = millis();
    elapsed = now - timerStart;

    // Wire contact → FAIL
    if (remoteReset) {
        remoteReset = false;
        ledsClear();
        enterState(STATE_IDLE);
        return;
    }

    if (remoteSimWire || isWireTouched()) {
        remoteSimWire = false;
        failCount++;
        Serial.print(F("[GAME] Wire touched — FAIL #"));
        Serial.println(failCount);
        enterState(STATE_FAIL);
        return;
    }

    // Finish pad → WIN
    if (remoteSimFinish || isFinishTouched()) {
        remoteSimFinish = false;
        Serial.print(F("[GAME] Finish touched — WIN! time="));
        Serial.print(elapsed / 1000UL);
        Serial.print(F("s fails="));
        Serial.println(failCount);
        enterState(STATE_WIN);
        return;
    }

#if PRO_MODE_ENABLED
    // Pro Mode: advance phase timer and update discrete indicator LEDs
    promodeUpdate();

    // Movement during RED phase → FAIL
    if (promodeIsRed() && promodeMovementDetected()) {
        failCount++;
        Serial.print(F("[GAME] Pro Mode movement during RED — FAIL #"));
        Serial.println(failCount);
        enterState(STATE_FAIL);
        return;
    }

    // Matrix: only redraw when the phase changes (G ↔ R) to avoid redundant
    // FastLED.show() calls every loop tick.
    // LED strip effects are called every tick (self-throttled via PLAY_UPDATE_MS)
    // so the smooth beatsin8 breathing animation continues uninterrupted.
    {
        char newLetter = promodeIsGreen() ? 'G' : 'R';
        if (newLetter != lastProLetter) {
            lastProLetter = newLetter;
            if (newLetter == 'G') {
                matrixShowLetter('G', CRGB::Green);
            } else {
                matrixShowLetter('R', CRGB::Red);
            }
        }
        if (promodeIsGreen()) {
            ledsProGreen();
        } else {
            ledsProRed();
        }
    }
#else
    // Normal mode: update matrix only when the displayed second changes
    {
        int sec = (int)(elapsed / 1000);
        if (sec != lastElapsedSec) {
            lastElapsedSec = sec;
            matrixShowNumber(sec, CRGB::White);
        }
    }
    ledsPlaying();
#endif

    DEBUG_LOG_VAL("[GAME] elapsed=", elapsed);
}

// ── FAIL State ──────────────────────────────────────────────────────────────
static void handleFail() {
    unsigned long now = millis();
    unsigned long dt  = now - stateStart;

    // Single 500 ms buzzer tone — fired exactly once via buzzerFired flag
    // (avoids the fragile dt<10 window that could be missed if loop is slow)
    if (!buzzerFired) {
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerFired = true;
    } else if (dt >= 500) {
        digitalWrite(BUZZER_PIN, LOW);
    }

    ledsFail();

    // Sequence: "FAIL" scroll → fail-count digit → "GO BACK" scroll
    if (dt < FAIL_DISPLAY_MS / 2) {
        matrixScrollText("FAIL", CRGB::Red, cfg.scrollFailMs);
    } else if (dt < (cfg.failDisplayMs * 3) / 4) {
        matrixShowNumber(failCount, CRGB::Orange);
    } else {
        matrixScrollText("GO BACK", CRGB::Orange, cfg.scrollFailMs);
    }

    if (dt >= cfg.failDisplayMs) {
        digitalWrite(BUZZER_PIN, LOW);
        ledsClear();
        timerStart = 0;
        enterState(STATE_IDLE);
    }
}

// ── WIN State ───────────────────────────────────────────────────────────────
static void handleWin() {
    unsigned long now = millis();
    unsigned long dt  = now - stateStart;

    // Three short beeps: 100 ms on / 100 ms off × 3
    if      (dt <  100) { digitalWrite(BUZZER_PIN, HIGH); }
    else if (dt <  200) { digitalWrite(BUZZER_PIN, LOW);  }
    else if (dt <  300) { digitalWrite(BUZZER_PIN, HIGH); }
    else if (dt <  400) { digitalWrite(BUZZER_PIN, LOW);  }
    else if (dt <  500) { digitalWrite(BUZZER_PIN, HIGH); }
    else                { digitalWrite(BUZZER_PIN, LOW);  }

    ledsWin();

    // winMsg was formatted once in enterState(STATE_WIN); just scroll it here.
    matrixScrollText(winMsg, CRGB::Green, cfg.scrollWinMs);

    if (dt >= cfg.winDisplayMs) {
        ledsClear();
        enterState(STATE_IDLE);
    }
}

// ── Main Loop ───────────────────────────────────────────────────────────────
void gameLoop() {
    switch (currentState) {
        case STATE_IDLE:      handleIdle();      break;
        case STATE_COUNTDOWN: handleCountdown(); break;
        case STATE_PLAYING:   handlePlaying();   break;
        case STATE_FAIL:      handleFail();      break;
        case STATE_WIN:       handleWin();       break;
    }
}

// ── Accessors ───────────────────────────────────────────────────────────────
GameState gameGetState() {
    return currentState;
}

unsigned long gameGetElapsed() {
    return elapsed;
}

int gameGetFailCount() {
    return failCount;
}
