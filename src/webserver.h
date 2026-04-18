#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>

#define WS_MAX_CLIENTS 2

void webServerSetup();
void webServerLoop();   // Flush queued WebSocket text messages

// ── Push events to all connected WS clients ──────────────────────────────────
void wsBroadcastState();         // Game state snapshot
void wsBroadcastConfig();        // Full config JSON
void wsBroadcastScores();        // Scoreboard JSON
void wsBroadcastSysInfo();       // Free heap, uptime, RSSI, AP/STA mode

#endif // WEBSERVER_H
