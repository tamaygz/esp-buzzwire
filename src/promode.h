#ifndef PROMODE_H
#define PROMODE_H

#include <Arduino.h>

// ── Setup ───────────────────────────────────────────────────────────────────
void promodeSetup();

// ── Runtime ─────────────────────────────────────────────────────────────────
void promodeUpdate();              // Call every loop tick during PLAYING; updates phase timer + discrete LEDs
bool promodeIsGreen();
bool promodeIsRed();
bool promodeMovementDetected();    // Checks sensor(s) per PRO_MODE_SENSOR config
void promodeReset();               // Reset phase timer (call on game start)

#endif // PROMODE_H
