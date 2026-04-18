#include "game_config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

GameConfig cfg;

static const char CONFIG_PATH[] = "/config.json";

// ── Defaults ─────────────────────────────────────────────────────────────────
void configInit() {
    cfg.proModeEnabled    = PRO_MODE_ENABLED;
    cfg.proModeSensor     = PRO_MODE_SENSOR;
    cfg.proGreenMin       = PRO_GREEN_MIN;
    cfg.proGreenMax       = PRO_GREEN_MAX;
    cfg.proRedMin         = PRO_RED_MIN;
    cfg.proRedMax         = PRO_RED_MAX;
    cfg.irMoveThreshold   = IR_MOVE_THRESHOLD;
    cfg.debounceMs        = DEBOUNCE_MS;
    cfg.stripBrightness   = STRIP_BRIGHTNESS;
    cfg.matrixBrightness  = MATRIX_BRIGHTNESS;
    cfg.failDisplayMs     = FAIL_DISPLAY_MS;
    cfg.winDisplayMs      = WIN_DISPLAY_MS;
    cfg.countdownStepMs   = COUNTDOWN_STEP_MS;
    cfg.strobeIntervalMs  = STROBE_INTERVAL_MS;
    cfg.strobeFlashCount  = STROBE_FLASH_COUNT;
    cfg.scrollIdleMs      = SCROLL_IDLE_MS;
    cfg.scrollCountdownMs = SCROLL_COUNTDOWN_MS;
    cfg.scrollFailMs      = SCROLL_FAIL_MS;
    cfg.scrollWinMs       = SCROLL_WIN_MS;
}

// ── Load from LittleFS ───────────────────────────────────────────────────────
bool configLoad() {
    if (!LittleFS.exists(CONFIG_PATH)) return false;

    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    cfg.proModeEnabled    = doc["proModeEnabled"]    | cfg.proModeEnabled;
    cfg.proModeSensor     = doc["proModeSensor"]     | cfg.proModeSensor;
    cfg.proGreenMin       = doc["proGreenMin"]       | cfg.proGreenMin;
    cfg.proGreenMax       = doc["proGreenMax"]       | cfg.proGreenMax;
    cfg.proRedMin         = doc["proRedMin"]         | cfg.proRedMin;
    cfg.proRedMax         = doc["proRedMax"]         | cfg.proRedMax;
    cfg.irMoveThreshold   = doc["irMoveThreshold"]   | cfg.irMoveThreshold;
    cfg.debounceMs        = doc["debounceMs"]        | cfg.debounceMs;
    cfg.stripBrightness   = doc["stripBrightness"]   | cfg.stripBrightness;
    cfg.matrixBrightness  = doc["matrixBrightness"]  | cfg.matrixBrightness;
    cfg.failDisplayMs     = doc["failDisplayMs"]     | cfg.failDisplayMs;
    cfg.winDisplayMs      = doc["winDisplayMs"]      | cfg.winDisplayMs;
    cfg.countdownStepMs   = doc["countdownStepMs"]   | cfg.countdownStepMs;
    cfg.strobeIntervalMs  = doc["strobeIntervalMs"]  | cfg.strobeIntervalMs;
    cfg.strobeFlashCount  = doc["strobeFlashCount"]  | cfg.strobeFlashCount;
    cfg.scrollIdleMs      = doc["scrollIdleMs"]      | cfg.scrollIdleMs;
    cfg.scrollCountdownMs = doc["scrollCountdownMs"] | cfg.scrollCountdownMs;
    cfg.scrollFailMs      = doc["scrollFailMs"]      | cfg.scrollFailMs;
    cfg.scrollWinMs       = doc["scrollWinMs"]       | cfg.scrollWinMs;

    return true;
}

// ── Save to LittleFS ─────────────────────────────────────────────────────────
bool configSave() {
    JsonDocument doc;
    doc["proModeEnabled"]    = cfg.proModeEnabled;
    doc["proModeSensor"]     = cfg.proModeSensor;
    doc["proGreenMin"]       = cfg.proGreenMin;
    doc["proGreenMax"]       = cfg.proGreenMax;
    doc["proRedMin"]         = cfg.proRedMin;
    doc["proRedMax"]         = cfg.proRedMax;
    doc["irMoveThreshold"]   = cfg.irMoveThreshold;
    doc["debounceMs"]        = cfg.debounceMs;
    doc["stripBrightness"]   = cfg.stripBrightness;
    doc["matrixBrightness"]  = cfg.matrixBrightness;
    doc["failDisplayMs"]     = cfg.failDisplayMs;
    doc["winDisplayMs"]      = cfg.winDisplayMs;
    doc["countdownStepMs"]   = cfg.countdownStepMs;
    doc["strobeIntervalMs"]  = cfg.strobeIntervalMs;
    doc["strobeFlashCount"]  = cfg.strobeFlashCount;
    doc["scrollIdleMs"]      = cfg.scrollIdleMs;
    doc["scrollCountdownMs"] = cfg.scrollCountdownMs;
    doc["scrollFailMs"]      = cfg.scrollFailMs;
    doc["scrollWinMs"]       = cfg.scrollWinMs;

    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// ── Reset to compile-time defaults ──────────────────────────────────────────
void configReset() {
    configInit();
    configSave();
}
