#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>
#include "config.h"

// ── Runtime-editable game configuration ─────────────────────────────────────
// All fields default to the compile-time constants in config.h.
// Loaded from /config.json at boot; saved back on user request.
struct GameConfig {
    // Pro Mode
    bool     proModeEnabled;
    uint8_t  proModeSensor;     // 0=IR, 1=PIR, 2=BOTH
    uint32_t proGreenMin;
    uint32_t proGreenMax;
    uint32_t proRedMin;
    uint32_t proRedMax;

    // Sensor tuning
    uint16_t irMoveThreshold;
    uint32_t debounceMs;

    // LED strip
    uint8_t  stripBrightness;
    uint8_t  matrixBrightness;

    // Timing
    uint32_t failDisplayMs;
    uint32_t winDisplayMs;
    uint32_t countdownStepMs;

    // Strip effect timing
    uint32_t strobeIntervalMs;
    uint8_t  strobeFlashCount;

    // Matrix scroll speeds
    uint32_t scrollIdleMs;
    uint32_t scrollCountdownMs;
    uint32_t scrollFailMs;
    uint32_t scrollWinMs;
};

// ── Global config instance (defined in game_config.cpp) ─────────────────────
extern GameConfig cfg;

// ── API ──────────────────────────────────────────────────────────────────────
void configInit();               // Populate cfg with compile-time defaults (does NOT save)
bool configLoad();               // Load /config.json → cfg. Returns false if file missing.
bool configSave();               // Save cfg → /config.json. Returns false on error.
void configReset();              // Restore defaults, then save.

#endif // GAME_CONFIG_H
