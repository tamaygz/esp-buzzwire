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
    currentState = STATE_IDLE;
    timerStart   = 0;
    stateStart   = millis();
    failCount    = 0;
    elapsed      = 0;
    countdownStep = 3;
}

// ── State Transition Helper ──────────────────────────────────────────────────
// Resets the per-state timer, clears the matrix scroll position, and logs
// the transition so it is always visible in the Serial monitor.
static void enterState(GameState newState) {
    Serial.print(F("[GAME] "));
    Serial.print(gameGetStateName(currentState));
    Serial.print(F(" → "));
    Serial.println(gameGetStateName(newState));

    currentState = newState;
    stateStart   = millis();
    matrixScrollReset();   // Always start fresh on state entry
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
        irCalibrate();
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

    // Matrix: 'G' (green) = player may move; 'R' (red) = freeze
    if (promodeIsGreen()) {
        matrixShowLetter('G', CRGB::Green);
        ledsProGreen();
    } else {
        matrixShowLetter('R', CRGB::Red);
        ledsProRed();
    }
#else
    // Normal mode: live elapsed time on matrix (seconds), slow green pulse
    matrixShowNumber((int)(elapsed / 1000), CRGB::White);
    ledsPlaying();
#endif

    DEBUG_LOG_VAL("[GAME] elapsed=", elapsed);
}

// ── FAIL State ──────────────────────────────────────────────────────────────
static void handleFail() {
    unsigned long now = millis();
    unsigned long dt  = now - stateStart;

    // Single 500 ms buzzer tone at state entry
    if (dt < 10) {
        digitalWrite(BUZZER_PIN, HIGH);
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

    // Build message once and keep the static buffer address stable for the scroll
    static char winMsg[32];
    snprintf(winMsg, sizeof(winMsg), "WIN! %lus %df",
             elapsed / 1000UL, failCount);
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
