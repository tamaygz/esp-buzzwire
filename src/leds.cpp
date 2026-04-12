#include "leds.h"
#include "config.h"
#include <FastLED.h>

// ── Strip Array & Controller ────────────────────────────────────────────────
static CRGB stripLeds[STRIP_NUM_LEDS];
static CLEDController* gStripCtrl = nullptr;

// ── Internal Timing ─────────────────────────────────────────────────────────
static unsigned long lastUpdate = 0;
static uint8_t hueOffset = 0;
static uint8_t strobeCount = 0;
static bool strobeOn = false;
static unsigned long strobeTimer = 0;

// ── Setup ───────────────────────────────────────────────────────────────────
void ledsSetup() {
    gStripCtrl = &FastLED.addLeds<WS2812B, STRIP_PIN, GRB>(stripLeds, STRIP_NUM_LEDS);
    gStripCtrl->setBrightness(STRIP_BRIGHTNESS);
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
    FastLED.show();
}

// ── Idle: Rainbow Cycle ─────────────────────────────────────────────────────
void ledsIdle() {
    unsigned long now = millis();
    if (now - lastUpdate < 20) return;
    lastUpdate = now;

    fill_rainbow(stripLeds, STRIP_NUM_LEDS, hueOffset, 7);
    hueOffset++;
    FastLED.show();
}

// ── Countdown: Progressive Fill ─────────────────────────────────────────────
void ledsCountdown(int step) {
    // step 3 = 1/4 fill white, 2 = 1/2, 1 = 3/4, 0 = full green
    int fillCount;
    CRGB color;

    if (step > 0) {
        fillCount = map(4 - step, 1, 3, 1, STRIP_NUM_LEDS - 1);
        color = CRGB::White;
    } else {
        fillCount = STRIP_NUM_LEDS;
        color = CRGB::Green;
    }

    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
    fill_solid(stripLeds, fillCount, color);
    FastLED.show();
}

// ── Playing: Slow Green Breathing Pulse ─────────────────────────────────────
void ledsPlaying() {
    unsigned long now = millis();
    if (now - lastUpdate < 30) return;
    lastUpdate = now;

    uint8_t brightness = beatsin8(20, 60, 200);   // BPM=20, min=60, max=200
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Green);
    gStripCtrl->setBrightness(brightness);
    FastLED.show();
    gStripCtrl->setBrightness(STRIP_BRIGHTNESS);  // restore for other callers
}

// ── Fail: Red Strobe Flash x3 ──────────────────────────────────────────────
void ledsFail() {
    unsigned long now = millis();

    // Reset strobe state on first call (strobeCount will be 0 after reset)
    if (strobeCount == 0 && !strobeOn) {
        strobeTimer = now;
        strobeOn = true;
        strobeCount = 1;
        fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
        FastLED.show();
        return;
    }

    if (now - strobeTimer < 150) return;   // 150ms per strobe phase
    strobeTimer = now;

    if (strobeOn) {
        fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
        strobeOn = false;
    } else {
        if (strobeCount < 3) {
            fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
            strobeOn = true;
            strobeCount++;
        }
        // After 3 flashes, stay dark
    }
    FastLED.show();
}

// ── Win: Confetti / Sparkle ─────────────────────────────────────────────────
void ledsWin() {
    unsigned long now = millis();
    if (now - lastUpdate < 15) return;
    lastUpdate = now;

    fadeToBlackBy(stripLeds, STRIP_NUM_LEDS, 20);
    int pos = random16(STRIP_NUM_LEDS);
    stripLeds[pos] += CHSV(random8(), 200, 255);
    FastLED.show();
}

// ── Pro Mode: Fast Green Pulse ──────────────────────────────────────────────
void ledsProGreen() {
    unsigned long now = millis();
    if (now - lastUpdate < 20) return;
    lastUpdate = now;

    uint8_t brightness = beatsin8(60, 80, 255);    // BPM=60, faster pulse
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Green);
    gStripCtrl->setBrightness(brightness);
    FastLED.show();
    gStripCtrl->setBrightness(STRIP_BRIGHTNESS);
}

// ── Pro Mode: Steady Red Solid ──────────────────────────────────────────────
void ledsProRed() {
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
    FastLED.show();
}

// ── Clear: All Off ──────────────────────────────────────────────────────────
void ledsClear() {
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
    FastLED.show();
    // Reset strobe state
    strobeCount = 0;
    strobeOn = false;
}
