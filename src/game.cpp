#include "game.h"
#include "config.h"
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
static bool buzzerFired   = false;   // Ensures FAIL buzzer fires exactly once
static char winMsg[32]    = {};      // WIN message built once in enterState(STATE_WIN)
static char lastProLetter = '\0';    // Cached Pro Mode letter (G/R); '\0' = unset
static int  lastElapsedSec = -1;     // Cached elapsed seconds for normal-mode matrix

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
    matrixScrollReset();   // Always start fresh on state entry

    // Reset one-shot / cached state for the new state
    buzzerFired    = false;
    lastProLetter  = '\0';
    lastElapsedSec = -1;

    // Build WIN message at entry so it is ready before the first loop tick.
    // snprintf() runs exactly once per WIN state, not every loop tick.
    if (newState == STATE_WIN) {
        snprintf(winMsg, sizeof(winMsg), "WIN! %lus %df",
                 elapsed / 1000UL, failCount);
        Serial.print(F("[GAME] WIN message: "));
        Serial.println(winMsg);
    }
}

// ── IDLE State ──────────────────────────────────────────────────────────────
static void handleIdle() {
    ledsIdle();
    matrixScrollText("TOUCH START", CRGB::Cyan, SCROLL_IDLE_MS);

    if (isStartTouched()) {
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
        matrixScrollText("GO!", CRGB::Green, SCROLL_COUNTDOWN_MS);
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
    if (isWireTouched()) {
        failCount++;
        Serial.print(F("[GAME] Wire touched — FAIL #"));
        Serial.println(failCount);
        enterState(STATE_FAIL);
        return;
    }

    // Finish pad → WIN
    if (isFinishTouched()) {
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
        matrixScrollText("FAIL", CRGB::Red, SCROLL_FAIL_MS);
    } else if (dt < (FAIL_DISPLAY_MS * 3) / 4) {
        matrixShowNumber(failCount, CRGB::Orange);
    } else {
        matrixScrollText("GO BACK", CRGB::Orange, SCROLL_FAIL_MS);
    }

    if (dt >= (unsigned long)FAIL_DISPLAY_MS) {
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
    matrixScrollText(winMsg, CRGB::Green, SCROLL_WIN_MS);

    if (dt >= (unsigned long)WIN_DISPLAY_MS) {
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
