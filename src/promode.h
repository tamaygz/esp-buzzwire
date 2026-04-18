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

// ── Event Callback ──────────────────────────────────────────────────────────
// Registered function is called every time the phase changes (GREEN→RED or RED→GREEN).
// @param isGreen    true = now green (safe), false = now red (freeze)
// @param durationMs Duration of the new phase in ms
void promodeOnPhaseChange(void (*cb)(bool isGreen, unsigned long durationMs));

#endif // PROMODE_H
