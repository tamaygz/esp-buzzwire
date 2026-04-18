#include "promode.h"
#include "config.h"
#include "game_config.h"
#include "sensors.h"
#include <Arduino.h>

// ── Internal State ──────────────────────────────────────────────────────────
static unsigned long phaseStart    = 0;
static unsigned long phaseDuration = 0;
static bool greenPhase = true;

static void (*sPhaseChangeCb)(bool isGreen, unsigned long durationMs) = nullptr;

void promodeOnPhaseChange(void (*cb)(bool isGreen, unsigned long durationMs)) {
    sPhaseChangeCb = cb;
}

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
    phaseDuration = randomDuration(cfg.proGreenMin, cfg.proGreenMax);
    greenPhase    = true;
    digitalWrite(PRO_GREEN_LED_PIN, HIGH);
    digitalWrite(PRO_RED_LED_PIN,   LOW);
    Serial.print(F("[PROMODE] Phase reset → GREEN ("));  Serial.print(phaseDuration);  Serial.println(F("ms)"));
    if (sPhaseChangeCb) sPhaseChangeCb(true, phaseDuration);
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
            phaseDuration = randomDuration(cfg.proRedMin, cfg.proRedMax);
            digitalWrite(PRO_GREEN_LED_PIN, LOW);
            digitalWrite(PRO_RED_LED_PIN,   HIGH);
            Serial.print(F("[PROMODE] → RED phase (freeze!) "));  Serial.print(phaseDuration);  Serial.println(F("ms"));
            if (sPhaseChangeCb) sPhaseChangeCb(false, phaseDuration);
        }
    } else {
        if (elapsed >= phaseDuration) {
            greenPhase    = true;
            phaseStart    = now;
            phaseDuration = randomDuration(cfg.proGreenMin, cfg.proGreenMax);
            digitalWrite(PRO_GREEN_LED_PIN, HIGH);
            digitalWrite(PRO_RED_LED_PIN,   LOW);
            Serial.print(F("[PROMODE] → GREEN phase (safe to move) "));  Serial.print(phaseDuration);  Serial.println(F("ms"));
            if (sPhaseChangeCb) sPhaseChangeCb(true, phaseDuration);
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
// Dispatches to the sensor(s) selected in runtime config.
bool promodeMovementDetected() {
    if (cfg.proModeSensor == SENSOR_IR) return irIsMoving();
    if (cfg.proModeSensor == SENSOR_PIR) return pirIsMoving();
    if (cfg.proModeSensor == SENSOR_BOTH) return irIsMoving() || pirIsMoving();
    return false;
}
