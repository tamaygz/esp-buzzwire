#include "promode.h"
#include "config.h"
#include "sensors.h"
#include "matrix.h"
#include <Arduino.h>

// ── Internal State ──────────────────────────────────────────────────────────
static unsigned long phaseStart = 0;
static bool greenPhase = true;

// ── Setup ───────────────────────────────────────────────────────────────────
void promodeSetup() {
    pinMode(PRO_RED_LED_PIN,   OUTPUT);
    pinMode(PRO_GREEN_LED_PIN, OUTPUT);
    digitalWrite(PRO_RED_LED_PIN,   LOW);
    digitalWrite(PRO_GREEN_LED_PIN, LOW);
}

// ── Reset ───────────────────────────────────────────────────────────────────
void promodeReset() {
    phaseStart = millis();
    greenPhase = true;
    digitalWrite(PRO_GREEN_LED_PIN, HIGH);
    digitalWrite(PRO_RED_LED_PIN,   LOW);
}

// ── Update (call every loop tick) ───────────────────────────────────────────
void promodeUpdate() {
    unsigned long now = millis();
    unsigned long elapsed = now - phaseStart;

    if (greenPhase) {
        if (elapsed >= (unsigned long)PRO_GREEN_DURATION) {
            greenPhase = false;
            phaseStart = now;
            digitalWrite(PRO_GREEN_LED_PIN, LOW);
            digitalWrite(PRO_RED_LED_PIN,   HIGH);
        }
    } else {
        if (elapsed >= (unsigned long)PRO_RED_DURATION) {
            greenPhase = true;
            phaseStart = now;
            digitalWrite(PRO_GREEN_LED_PIN, HIGH);
            digitalWrite(PRO_RED_LED_PIN,   LOW);
        }
    }

    // Reflect state on matrix
    if (greenPhase) {
        matrixShowLetter('G', CRGB::Green);
    } else {
        matrixShowLetter('R', CRGB::Red);
    }
}

// ── Phase Queries ───────────────────────────────────────────────────────────
bool promodeIsGreen() {
    return greenPhase;
}

bool promodeIsRed() {
    return !greenPhase;
}

// ── Movement Detection ──────────────────────────────────────────────────────
bool promodeMovementDetected() {
#if PRO_MODE_SENSOR == SENSOR_IR
    return irIsMoving();
#elif PRO_MODE_SENSOR == SENSOR_PIR
    return pirIsMoving();
#elif PRO_MODE_SENSOR == SENSOR_BOTH
    return irIsMoving() || pirIsMoving();
#else
    return false;
#endif
}
