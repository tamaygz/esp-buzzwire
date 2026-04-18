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
// irCalibrate() must be called before irIsMoving(); blocks for ~64 ms.
void irCalibrate();
bool irIsMoving();
int  irGetBaseline();   // Returns last calibrated ADC baseline (0 if not yet calibrated)

// ── PIR Sensor ──────────────────────────────────────────────────────────────
bool pirIsMoving();

#endif // SENSORS_H
