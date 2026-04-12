#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── LED Strip ───────────────────────────────────────────────────────────────
#define STRIP_PIN          2        // D4 / GPIO2
#define STRIP_NUM_LEDS     30
#define STRIP_BRIGHTNESS   180

// ── LED Matrix (8x8 WS2812B) ───────────────────────────────────────────────
#define MATRIX_PIN         0        // D3 / GPIO0
#define MATRIX_WIDTH       8
#define MATRIX_HEIGHT      8
#define MATRIX_NUM_LEDS    (MATRIX_WIDTH * MATRIX_HEIGHT)
#define MATRIX_BRIGHTNESS  80
#define MATRIX_SERPENTINE  true     // true = zigzag / serpentine wiring

// ── Game Pins ───────────────────────────────────────────────────────────────
#define WIRE_PIN           4        // D2 / GPIO4  — buzzwire contact
#define START_PIN          5        // D1 / GPIO5  — start pad
#define FINISH_PIN         12       // D6 / GPIO12 — finish pad
#define BUZZER_PIN         14       // D5 / GPIO14 — active buzzer

// ── Pro Mode ────────────────────────────────────────────────────────────────
#define PRO_MODE_ENABLED   true     // Master toggle

// Sensor type constants
#define SENSOR_IR          0
#define SENSOR_PIR         1
#define SENSOR_BOTH        2

#define PRO_MODE_SENSOR    SENSOR_IR   // 0=IR, 1=PIR, 2=BOTH

#define PRO_GREEN_DURATION 3000     // ms player may move
#define PRO_RED_DURATION   2000     // ms player must freeze

#define IR_PIN             A0       // Only analog pin on ESP8266
#define IR_MOVE_THRESHOLD  150      // ADC delta to count as movement

#define PIR_PIN            13       // D7 / GPIO13

// ── Pro Mode Traffic Light LEDs ─────────────────────────────────────────────
#define PRO_RED_LED_PIN    15       // D8 / GPIO15
#define PRO_GREEN_LED_PIN  16       // D0 / GPIO16

// ── Timing / Tuning ────────────────────────────────────────────────────────
#define DEBOUNCE_MS        50       // Debounce interval for contact sensors
#define FAIL_DISPLAY_MS    2000     // Time to show fail animation
#define WIN_DISPLAY_MS     5000     // Time to show win animation
#define COUNTDOWN_STEP_MS  1000     // Duration of each countdown step

#endif // CONFIG_H
