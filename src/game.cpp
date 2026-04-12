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

// ── Setup ───────────────────────────────────────────────────────────────────
void gameSetup() {
    currentState = STATE_IDLE;
    timerStart   = 0;
    stateStart   = millis();
    failCount    = 0;
    elapsed      = 0;
}

// ── State Transition Helpers ────────────────────────────────────────────────
static void enterState(GameState newState) {
    currentState = newState;
    stateStart   = millis();
}

// ── IDLE State ──────────────────────────────────────────────────────────────
static void handleIdle() {
    ledsIdle();
    matrixScrollText("TOUCH START", CRGB::Cyan, 80);

    if (isStartTouched()) {
        // Reset game variables
        failCount     = 0;
        elapsed       = 0;
        countdownStep = 3;
        ledsClear();
        enterState(STATE_COUNTDOWN);
        Serial.println(F("[GAME] Start touched — countdown!"));
    }
}

// ── COUNTDOWN State ─────────────────────────────────────────────────────────
static void handleCountdown() {
    unsigned long now = millis();
    int stepIndex = (int)((now - stateStart) / COUNTDOWN_STEP_MS);

    if (stepIndex < 3) {
        int digit = 3 - stepIndex;
        if (countdownStep != digit) {
            countdownStep = digit;
            Serial.print(F("[GAME] Countdown: "));
            Serial.println(digit);
        }
        matrixShowNumber(3 - stepIndex, CRGB::Yellow);
        ledsCountdown(3 - stepIndex);
    } else if (stepIndex == 3) {
        if (countdownStep != 0) {
            countdownStep = 0;
            Serial.println(F("[GAME] GO!"));
        }
        matrixShowLetter('G', CRGB::Green);
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
        Serial.println(F("[GAME] Playing — timer started"));
    }
}

// ── PLAYING State ───────────────────────────────────────────────────────────
static void handlePlaying() {
    unsigned long now = millis();
    elapsed = now - timerStart;

    // Check wire contact → FAIL
    if (isWireTouched()) {
        failCount++;
        Serial.print(F("[GAME] FAIL #"));
        Serial.println(failCount);
        enterState(STATE_FAIL);
        return;
    }

    // Check finish pad → WIN
    if (isFinishTouched()) {
        Serial.print(F("[GAME] WIN! Time: "));
        Serial.print(elapsed / 1000.0, 1);
        Serial.print(F("s  Fails: "));
        Serial.println(failCount);
        enterState(STATE_WIN);
        return;
    }

#if PRO_MODE_ENABLED
    // Pro Mode: update traffic light and check movement during red
    promodeUpdate();

    if (promodeIsRed() && promodeMovementDetected()) {
        failCount++;
        Serial.print(F("[GAME] PRO MODE FAIL (movement during RED) #"));
        Serial.println(failCount);
        enterState(STATE_FAIL);
        return;
    }

    // LED strip reflects pro mode phase
    if (promodeIsGreen()) {
        ledsProGreen();
    } else {
        ledsProRed();
    }
#else
    // Normal mode: show elapsed time on matrix, green pulse on strip
    int secs = (int)(elapsed / 1000);
    matrixShowNumber(secs, CRGB::White);
    ledsPlaying();
#endif
}

// ── FAIL State ──────────────────────────────────────────────────────────────
static void handleFail() {
    unsigned long now = millis();
    unsigned long dt = now - stateStart;

    // Buzz once at start of fail
    if (dt < 10) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else if (dt >= 500) {
        digitalWrite(BUZZER_PIN, LOW);
    }

    // LED strobe
    ledsFail();

    // Matrix message
    if (dt < FAIL_DISPLAY_MS / 2) {
        matrixScrollText("FAIL", CRGB::Red, 60);
    } else {
        matrixScrollText("GO BACK", CRGB::Orange, 60);
    }

    // Return to idle after FAIL_DISPLAY_MS
    if (dt >= (unsigned long)FAIL_DISPLAY_MS) {
        digitalWrite(BUZZER_PIN, LOW);
        ledsClear();
        timerStart = 0;
        enterState(STATE_IDLE);
        Serial.println(F("[GAME] Returning to IDLE — go back to start"));
    }
}

// ── WIN State ───────────────────────────────────────────────────────────────
static void handleWin() {
    unsigned long now = millis();
    unsigned long dt = now - stateStart;

    // Triple beep at start
    if (dt < 100) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else if (dt < 200) {
        digitalWrite(BUZZER_PIN, LOW);
    } else if (dt < 300) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else if (dt < 400) {
        digitalWrite(BUZZER_PIN, LOW);
    } else if (dt < 500) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Confetti on strip
    ledsWin();

    // Scrolling win message: "WIN! Xs Yf"
    static char winMsg[32];
    snprintf(winMsg, sizeof(winMsg), "WIN! %lus %df",
             elapsed / 1000, failCount);
    matrixScrollText(winMsg, CRGB::Green, 80);

    // Return to idle after WIN_DISPLAY_MS
    if (dt >= (unsigned long)WIN_DISPLAY_MS) {
        ledsClear();
        enterState(STATE_IDLE);
        Serial.println(F("[GAME] Returning to IDLE"));
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
