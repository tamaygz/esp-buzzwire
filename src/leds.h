#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

// ── Setup ───────────────────────────────────────────────────────────────────
void ledsSetup();

// ── Effects (all non-blocking, call every loop tick) ────────────────────────
void ledsIdle();
void ledsCountdown(int step);       // step: 3,2,1,0 (0 = GO)
void ledsPlaying();
void ledsFail();
void ledsWin();
void ledsProGreen();
void ledsProRed();
void ledsClear();

#endif // LEDS_H
