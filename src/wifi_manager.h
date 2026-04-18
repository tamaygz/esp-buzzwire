#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

// AP name shown in the captive-portal scan list.
// Password must be ≥ 8 chars to be accepted; leave blank for an open AP.
#define WIFI_AP_SSID            "Buzzwire-Game"
#define WIFI_AP_PASS            ""
// Seconds before the config portal closes and the device reboots.
#define WIFI_PORTAL_TIMEOUT_S   180

bool        wifiManagerSetup();  // Blocks until STA connected (reboots on portal timeout)
void        wifiManagerLoop();   // No-op; kept for API compatibility
void        wifiReset();         // Erases saved credentials and reboots into config portal

bool        wifiIsConnected();
const char* wifiGetIP();
const char* wifiGetSSID();
bool        wifiGetApMode();     // Always false; server only starts after STA connect

#endif // WIFI_MANAGER_H
