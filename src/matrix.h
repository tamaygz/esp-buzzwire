#ifndef MATRIX_H
#define MATRIX_H

#include <Arduino.h>
#include <FastLED.h>

// ── Setup ───────────────────────────────────────────────────────────────────
void matrixSetup();

// ── Drawing ─────────────────────────────────────────────────────────────────
void matrixClear();
void matrixDrawChar(int x, int y, char c, CRGB color);
void matrixScrollText(const char* text, CRGB color, int delayMs);
void matrixShowLetter(char c, CRGB color);
void matrixShowNumber(int n, CRGB color);

// ── Scroll Control ───────────────────────────────────────────────────────────
// Force the next matrixScrollText() call to restart from the right edge,
// regardless of the text content or pointer. Call whenever entering a new
// game state so stale scroll position does not carry over.
void matrixScrollReset();

// ── Utility ─────────────────────────────────────────────────────────────────
uint16_t XY(uint8_t x, uint8_t y);

#endif // MATRIX_H
