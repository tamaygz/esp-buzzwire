#include "wifi_manager.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

static WiFiManager sWm;

// ── Public API ────────────────────────────────────────────────────────────────

// Blocks until connected to WiFi.
// If no credentials are saved (or connection fails), opens a captive-portal AP.
// Reboots automatically on portal timeout to avoid hanging in an unknown state.
bool wifiManagerSetup() {
    sWm.setDebugOutput(true);
    sWm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);

    // Pass password only when one is configured (must be ≥ 8 chars per spec)
    const bool hasPass = strlen(WIFI_AP_PASS) >= 8;
    const bool ok = hasPass
        ? sWm.autoConnect(WIFI_AP_SSID, WIFI_AP_PASS)
        : sWm.autoConnect(WIFI_AP_SSID);

    if (!ok) {
        Serial.println(F("[WIFI] Config portal timed out — rebooting"));
        ESP.restart();
    }

    Serial.print(F("[WIFI] Connected to "));
    Serial.print(WiFi.SSID());
    Serial.print(F(" IP="));
    Serial.println(WiFi.localIP());
    return true;
}

// No-op: WiFiManager is fully blocking during setup; nothing to process in loop.
void wifiManagerLoop() {}

// Erase stored credentials and reboot into the config portal.
void wifiReset() {
    Serial.println(F("[WIFI] Resetting credentials — rebooting into config portal"));
    sWm.resetSettings();
    delay(300);
    ESP.restart();
}

bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }

const char* wifiGetIP() {
    static char buf[16];
    WiFi.localIP().toString().toCharArray(buf, sizeof(buf));
    return buf;
}

// WiFi.SSID() returns a String; keep a static copy to return a stable c_str.
const char* wifiGetSSID() {
    static char buf[33];
    WiFi.SSID().toCharArray(buf, sizeof(buf));
    return buf;
}

// The AsyncWebServer only starts after autoConnect() returns in STA mode.
bool wifiGetApMode() { return false; }
