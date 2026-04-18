#include "promode.h"
#include "config.h"
#include "sensors.h"
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
    DEBUG_LOGLN("[PROMODE] Setup complete — both LEDs off");
}

// ── Reset ───────────────────────────────────────────────────────────────────
// Restarts the phase timer at GREEN. Call this at the start of every PLAYING
// phase so the player always begins with the maximum movement window.
void promodeReset() {
    phaseStart = millis();
    greenPhase = true;
    digitalWrite(PRO_GREEN_LED_PIN, HIGH);
    digitalWrite(PRO_RED_LED_PIN,   LOW);
    Serial.println(F("[PROMODE] Phase reset → GREEN"));
}

// ── Update (call every loop tick during PLAYING) ─────────────────────────────
// Manages the GREEN ↔ RED phase timer and drives the discrete indicator LEDs.
// Matrix display is intentionally left to the game state machine (game.cpp).
void promodeUpdate() {
    unsigned long now     = millis();
    unsigned long elapsed = now - phaseStart;

    if (greenPhase) {
        if (elapsed >= (unsigned long)PRO_GREEN_DURATION) {
            greenPhase = false;
            phaseStart = now;
            digitalWrite(PRO_GREEN_LED_PIN, LOW);
            digitalWrite(PRO_RED_LED_PIN,   HIGH);
            Serial.println(F("[PROMODE] → RED phase (freeze!)"));
        }
    } else {
        if (elapsed >= (unsigned long)PRO_RED_DURATION) {
            greenPhase = true;
            phaseStart = now;
            digitalWrite(PRO_GREEN_LED_PIN, HIGH);
            digitalWrite(PRO_RED_LED_PIN,   LOW);
            Serial.println(F("[PROMODE] → GREEN phase (safe to move)"));
        }
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
// Dispatches to the sensor(s) selected by PRO_MODE_SENSOR at compile time.
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
