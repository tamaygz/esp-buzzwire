#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// ── Setup ───────────────────────────────────────────────────────────────────
void sensorsSetup();

// ── Contact Detection (debounced) ───────────────────────────────────────────
bool isWireTouched();
bool isStartTouched();
bool isFinishTouched();

// ── IR Sensor ───────────────────────────────────────────────────────────────
void irCalibrate();
bool irIsMoving();

// ── PIR Sensor ──────────────────────────────────────────────────────────────
bool pirIsMoving();

#endif // SENSORS_H
