#include "promode.h"
#include "config.h"
#include "sensors.h"
#include <Arduino.h>

// ── Internal State ──────────────────────────────────────────────────────────
static unsigned long phaseStart    = 0;
static unsigned long phaseDuration = 0;
static bool greenPhase = true;

static unsigned long randomDuration(unsigned long lo, unsigned long hi) {
    return lo + (random(hi - lo + 1));
}

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
    phaseStart    = millis();
    phaseDuration = randomDuration(PRO_GREEN_MIN, PRO_GREEN_MAX);
    greenPhase    = true;
    digitalWrite(PRO_GREEN_LED_PIN, HIGH);
    digitalWrite(PRO_RED_LED_PIN,   LOW);
    Serial.print(F("[PROMODE] Phase reset → GREEN ("));  Serial.print(phaseDuration);  Serial.println(F("ms)"));
}

// ── Update (call every loop tick during PLAYING) ─────────────────────────────
// Manages the GREEN ↔ RED phase timer and drives the discrete indicator LEDs.
// Matrix display is intentionally left to the game state machine (game.cpp).
void promodeUpdate() {
    unsigned long now     = millis();
    unsigned long elapsed = now - phaseStart;

    if (greenPhase) {
        if (elapsed >= phaseDuration) {
            greenPhase    = false;
            phaseStart    = now;
            phaseDuration = randomDuration(PRO_RED_MIN, PRO_RED_MAX);
            digitalWrite(PRO_GREEN_LED_PIN, LOW);
            digitalWrite(PRO_RED_LED_PIN,   HIGH);
            Serial.print(F("[PROMODE] → RED phase (freeze!) "));  Serial.print(phaseDuration);  Serial.println(F("ms"));
        }
    } else {
        if (elapsed >= phaseDuration) {
            greenPhase    = true;
            phaseStart    = now;
            phaseDuration = randomDuration(PRO_GREEN_MIN, PRO_GREEN_MAX);
            digitalWrite(PRO_GREEN_LED_PIN, HIGH);
            digitalWrite(PRO_RED_LED_PIN,   LOW);
            Serial.print(F("[PROMODE] → GREEN phase (safe to move) "));  Serial.print(phaseDuration);  Serial.println(F("ms"));
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
