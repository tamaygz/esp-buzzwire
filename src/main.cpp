// ── ESP Buzzwire Game — Main Entry Point ────────────────────────────────────
//
// All logic flows through the game state machine.
// Module setup functions are called here, then gameLoop() drives everything.

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "leds.h"
#include "matrix.h"
#include "game.h"
#include "promode.h"

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("=== ESP Buzzwire Game ==="));

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    sensorsSetup();
    ledsSetup();
    matrixSetup();

#if PRO_MODE_ENABLED
    promodeSetup();
    Serial.println(F("[INIT] Pro Mode: ENABLED"));
#else
    Serial.println(F("[INIT] Pro Mode: DISABLED"));
#endif

    gameSetup();
    Serial.println(F("[INIT] Ready — touch the START pad!"));
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    gameLoop();
}
