#include "leds.h"
#include "config.h"
#include <FastLED.h>

// ── Strip Array & Controller ────────────────────────────────────────────────
static CRGB stripLeds[STRIP_NUM_LEDS];
static CLEDController* gStripCtrl = nullptr;

// ── Shared Effect Timing ─────────────────────────────────────────────────────
static unsigned long lastUpdate = 0;
static uint8_t hueOffset = 0;

// ── Strobe State (ledsFail) ──────────────────────────────────────────────────
static uint8_t strobeCount = 0;
static bool    strobeOn    = false;
static unsigned long strobeTimer = 0;

// ── Setup ───────────────────────────────────────────────────────────────────
void ledsSetup() {
    gStripCtrl = &FastLED.addLeds<WS2812B, STRIP_PIN, GRB>(stripLeds, STRIP_NUM_LEDS);
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
    Serial.println(F("[LEDS] Setup complete"));
}

// ── Internal Helper: Breathing Brightness Effect ─────────────────────────────
// Applies a sinusoidal brightness pulse to a solid-colour strip.
// Calls showLeds(brightness) directly on the strip controller so only the
// strip is updated with the variable brightness, leaving the matrix unaffected.
// @param color  Solid colour to fill the strip with
// @param bpm    Oscillation speed (beats per minute)
// @param lo     Minimum brightness value (0–255)
// @param hi     Maximum brightness value (0–255)
static void stripBreathe(CRGB color, uint8_t bpm, uint8_t lo, uint8_t hi) {
    uint8_t brightness = beatsin8(bpm, lo, hi);
    fill_solid(stripLeds, STRIP_NUM_LEDS, color);
    gStripCtrl->showLeds(brightness);
}

// ── Idle: Rainbow Cycle ─────────────────────────────────────────────────────
void ledsIdle() {
    unsigned long now = millis();
    if (now - lastUpdate < IDLE_UPDATE_MS) return;
    lastUpdate = now;

    fill_rainbow(stripLeds, STRIP_NUM_LEDS, hueOffset, 7);
    hueOffset++;
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
}

// ── Countdown: Progressive Fill ─────────────────────────────────────────────
// step 3 = ¼ white, step 2 = ½ white, step 1 = ¾ white, step 0 = full green
void ledsCountdown(int step) {
    int  fillCount;
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
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
}

// ── Playing: Slow Green Breathing Pulse (BPM=20) ────────────────────────────
void ledsPlaying() {
    unsigned long now = millis();
    if (now - lastUpdate < PLAY_UPDATE_MS) return;
    lastUpdate = now;
    stripBreathe(CRGB::Green, 20, 60, 200);
}

// ── Fail: Red Strobe Flash × STROBE_FLASH_COUNT ─────────────────────────────
void ledsFail() {
    unsigned long now = millis();

    // First call after ledsClear() resets strobeCount — arm first flash
    if (strobeCount == 0 && !strobeOn) {
        strobeTimer = now;
        strobeOn    = true;
        strobeCount = 1;
        fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
        gStripCtrl->showLeds(STRIP_BRIGHTNESS);
        return;
    }

    if (now - strobeTimer < STROBE_INTERVAL_MS) return;
    strobeTimer = now;

    if (strobeOn) {
        // Turn off
        fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
        strobeOn = false;
    } else {
        // Turn on next flash if quota not reached
        if (strobeCount < STROBE_FLASH_COUNT) {
            fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
            strobeOn = true;
            strobeCount++;
        }
        // After all flashes, strip stays dark until ledsClear() is called
    }
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
}

// ── Win: Confetti / Sparkle ─────────────────────────────────────────────────
void ledsWin() {
    unsigned long now = millis();
    if (now - lastUpdate < WIN_UPDATE_MS) return;
    lastUpdate = now;

    fadeToBlackBy(stripLeds, STRIP_NUM_LEDS, 20);
    int pos = random16(STRIP_NUM_LEDS);
    stripLeds[pos] += CHSV(random8(), 200, 255);
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
}

// ── Pro Mode: Fast Green Pulse (BPM=60) ──────────────────────────────────────
void ledsProGreen() {
    unsigned long now = millis();
    if (now - lastUpdate < PLAY_UPDATE_MS) return;
    lastUpdate = now;
    stripBreathe(CRGB::Green, 60, 80, 255);
}

// ── Pro Mode: Steady Red Solid ───────────────────────────────────────────────
void ledsProRed() {
    unsigned long now = millis();
    if (now - lastUpdate < PLAY_UPDATE_MS) return;
    lastUpdate = now;
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Red);
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
}

// ── Clear: All Off, Reset Strobe State ──────────────────────────────────────
void ledsClear() {
    fill_solid(stripLeds, STRIP_NUM_LEDS, CRGB::Black);
    gStripCtrl->showLeds(STRIP_BRIGHTNESS);
    strobeCount = 0;
    strobeOn    = false;
    DEBUG_LOGLN("[LEDS] Cleared");
}
