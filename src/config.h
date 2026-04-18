#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ── Debug Logging ────────────────────────────────────────────────────────────
// Set to 1 to enable verbose sensor/LED/promode debug output on Serial.
// Leave at 0 for production builds (saves RAM and CPU time).
#define DEBUG_LOGGING      0

#if DEBUG_LOGGING
  #define DEBUG_LOG(msg)            Serial.print(F(msg))
  #define DEBUG_LOGLN(msg)          Serial.println(F(msg))
  #define DEBUG_LOG_VAL(msg, val)   do { Serial.print(F(msg)); Serial.println(val); } while (0)
  #define DEBUG_LOG_2(m1, v1, m2, v2) \
      do { Serial.print(F(m1)); Serial.print(v1); \
           Serial.print(F(m2)); Serial.println(v2); } while (0)
#else
  #define DEBUG_LOG(msg)
  #define DEBUG_LOGLN(msg)
  #define DEBUG_LOG_VAL(msg, val)
  #define DEBUG_LOG_2(m1, v1, m2, v2)
#endif

// ── LED Strip ───────────────────────────────────────────────────────────────
#define STRIP_PIN          2        // D4 / GPIO2
#define STRIP_NUM_LEDS     30
#define STRIP_BRIGHTNESS   180

// ── LED Matrix (8×8 WS2812B) ────────────────────────────────────────────────
#define MATRIX_PIN         0        // D3 / GPIO0
#define MATRIX_WIDTH       8
#define MATRIX_HEIGHT      8
#define MATRIX_NUM_LEDS    (MATRIX_WIDTH * MATRIX_HEIGHT)
#define MATRIX_BRIGHTNESS  80
#define MATRIX_SERPENTINE  1        // 1 = zigzag / serpentine wiring, 0 = progressive

// ── Game Pins ───────────────────────────────────────────────────────────────
#define WIRE_PIN           4        // D2 / GPIO4  — buzzwire contact
#define START_PIN          5        // D1 / GPIO5  — start pad
#define FINISH_PIN         12       // D6 / GPIO12 — finish pad
#define BUZZER_PIN         15       // D8 / GPIO15 — active buzzer

// ── Pro Mode ────────────────────────────────────────────────────────────────
#define PRO_MODE_ENABLED   1        // 1 = enabled, 0 = disabled

// Sensor type constants (used by PRO_MODE_SENSOR)
#define SENSOR_IR          0
#define SENSOR_PIR         1
#define SENSOR_BOTH        2

#define PRO_MODE_SENSOR    SENSOR_IR   // 0=IR, 1=PIR, 2=BOTH

#define PRO_GREEN_MIN      2000     // ms min green phase
#define PRO_GREEN_MAX      10000     // ms max green phase
#define PRO_RED_MIN        1500     // ms min red phase
#define PRO_RED_MAX        4000     // ms max red phase

#define IR_PIN             A0       // Only analog pin on ESP8266
#define IR_MOVE_THRESHOLD  150      // ADC delta to register as movement
#define IR_CALIBRATE_SAMPLES 32     // Number of ADC samples for IR baseline

#define PIR_PIN            13       // D7 / GPIO13

// ── Pro Mode Traffic Light LEDs ─────────────────────────────────────────────
#define PRO_RED_LED_PIN    14       // D5 / GPIO14
#define PRO_GREEN_LED_PIN  16       // D0 / GPIO16

// ── Timing / Tuning ─────────────────────────────────────────────────────────
#define DEBOUNCE_MS           50    // Debounce window for contact sensors (ms)
#define FAIL_DISPLAY_MS     2000    // Duration of fail animation (ms)
#define WIN_DISPLAY_MS      5000    // Duration of win animation (ms)
#define COUNTDOWN_STEP_MS   1000    // Duration of each countdown digit (ms)

// ── LED Strip Effect Timing ──────────────────────────────────────────────────
#define IDLE_UPDATE_MS        20    // Strip refresh interval — idle rainbow
#define PLAY_UPDATE_MS        30    // Strip refresh interval — playing / pro-green pulse
#define WIN_UPDATE_MS         15    // Strip refresh interval — win confetti
#define STROBE_INTERVAL_MS   150    // On/off time per flash in ledsFail()
#define STROBE_FLASH_COUNT     3    // Number of red flashes in ledsFail()

// ── Matrix Scroll Speeds ─────────────────────────────────────────────────────
// Smaller value = faster scroll (ms per pixel column advance)
#define SCROLL_IDLE_MS        80    // IDLE "TOUCH START" scroll
#define SCROLL_COUNTDOWN_MS   30    // COUNTDOWN "GO!" scroll
#define SCROLL_FAIL_MS        60    // FAIL "FAIL" / "GO BACK" scroll
#define SCROLL_WIN_MS         80    // WIN results scroll

#endif // CONFIG_H
