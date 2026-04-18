// ── ESP Buzzwire Game — Main Entry Point ────────────────────────────────────
//
// All logic flows through the game state machine (game.cpp).
// This file is responsible only for hardware init and the Arduino loop.
//
// Serial output at 115200 baud gives a live log of all game events.
// To enable verbose sensor/LED debug output set DEBUG_LOGGING=1 in config.h.

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "game_config.h"
#include "scoreboard.h"
#include "wifi_manager.h"
#include "webserver.h"
#include "sensors.h"
#include "leds.h"
#include "matrix.h"
#include "game.h"
#include "promode.h"

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("╔══════════════════════════╗"));
    Serial.println(F("║   ESP Buzzwire  v1.0.0   ║"));
    Serial.println(F("╚══════════════════════════╝"));

    // Buzzer off by default
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    sensorsSetup();
    ledsSetup();
    matrixSetup();

#if PRO_MODE_ENABLED
    promodeSetup();
    Serial.print(F("[INIT] Pro Mode: ENABLED  sensor="));
  #if   PRO_MODE_SENSOR == SENSOR_IR
    Serial.println(F("IR"));
  #elif PRO_MODE_SENSOR == SENSOR_PIR
    Serial.println(F("PIR"));
  #elif PRO_MODE_SENSOR == SENSOR_BOTH
    Serial.println(F("IR+PIR"));
  #endif
#else
    Serial.println(F("[INIT] Pro Mode: DISABLED"));
#endif

#if DEBUG_LOGGING
    Serial.println(F("[INIT] Debug logging: ON"));
#endif

    gameSetup();

    // Register WebSocket push callbacks
    gameOnStateChange([](GameState state, unsigned long, int) {
        wsBroadcastState();
        if (state == STATE_WIN) wsBroadcastScores();
    });
    promodeOnPhaseChange([](bool, unsigned long) { wsBroadcastState(); });

    if (!LittleFS.begin()) {
        Serial.println(F("[FS] LittleFS mount failed — web UI unavailable"));
    }

    configInit();
    configLoad();

    scoreboardInit();
    wifiManagerSetup();
    webServerSetup();

    Serial.println(F("[INIT] Ready — touch the START pad to begin!"));
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
    wifiManagerLoop();
    webServerLoop();
    gameLoop();
}
