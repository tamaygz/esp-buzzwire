# Software Architecture

Technical overview of the ESP Buzzwire firmware: modules, dependencies, state machine, and configuration.

---

## Module Dependency Diagram

```
                    ┌──────────┐
                    │ main.cpp │
                    └────┬─────┘
                         │ calls setup() / loop()
                         ▼
                    ┌──────────┐
                    │ game.cpp │  ◄── Game state machine
                    └────┬─────┘
                         │ uses
            ┌────────────┼────────────┬──────────────┐
            ▼            ▼            ▼              ▼
       ┌─────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐
       │ leds.cpp│ │matrix.cpp│ │sensors.cpp│ │promode.cpp│
       └────┬────┘ └────┬─────┘ └────┬─────┘ └─────┬─────┘
            │            │            │              │
            └────────────┴────────────┴──────────────┘
                         │
                         ▼
                    ┌──────────┐
                    │ config.h │  ◄── Pins, flags, constants
                    └──────────┘
```

All `.cpp` files include their own `.h` header and `config.h`. The `game.cpp` module orchestrates everything — it is the only module called from `main.cpp`.

---

## Source File Descriptions

### `src/config.h`
Centralised configuration header. Contains all:
- **Pin definitions** (LED strip, matrix, game pins, sensors, buzzer, Pro Mode LEDs)
- **Feature flags** (`PRO_MODE_ENABLED`, `PRO_MODE_SENSOR`)
- **Tuning constants** (debounce, display durations, countdown timing)
- **LED parameters** (num LEDs, brightness, serpentine flag)

No `.cpp` counterpart — header-only, included by every module.

### `src/main.cpp`
Minimal entry point:
- `setup()` — calls `sensorsSetup()`, `ledsSetup()`, `matrixSetup()`, `promodeSetup()`, `gameSetup()`, and initialises Serial.
- `loop()` — calls `gameLoop()`. All logic lives in the game module.

### `src/game.h` / `src/game.cpp`
The **game state machine** — the brain of the firmware.
- Defines `GameState` enum: `STATE_IDLE`, `STATE_COUNTDOWN`, `STATE_PLAYING`, `STATE_FAIL`, `STATE_WIN`.
- Tracks: current state, timer start, fail count, elapsed time, countdown step.
- Each state has a dedicated handler function:
  - `handleIdle()` — rainbow LEDs, scroll "TOUCH START", wait for start pad.
  - `handleCountdown()` — 3→2→1→GO on matrix, progressive fill on strip.
  - `handlePlaying()` — check wire contact, check finish, update matrix timer, Pro Mode integration.
  - `handleFail()` — buzz, red strobe, "FAIL"/"GO BACK" on matrix, return to IDLE.
  - `handleWin()` — triple beep, confetti, scroll "WIN! Xs Yf", return to IDLE.

### `src/leds.h` / `src/leds.cpp`
All **LED strip effects** using FastLED. Every effect is **non-blocking** (uses `millis()`).
- `ledsSetup()` — initialise FastLED for the strip.
- `ledsIdle()` — smooth rainbow cycle.
- `ledsCountdown(step)` — progressive white→green fill based on countdown step.
- `ledsPlaying()` — slow green breathing pulse (uses `beatsin8`).
- `ledsFail()` — red strobe flash ×3.
- `ledsWin()` — multi-colour confetti sparkle.
- `ledsProGreen()` — fast green pulse.
- `ledsProRed()` — steady solid red.
- `ledsClear()` — all off; resets strobe state.

### `src/matrix.h` / `src/matrix.cpp`
**8×8 WS2812B matrix** rendering engine.
- `matrixSetup()` — initialise FastLED for the matrix.
- `matrixClear()` — blank the matrix.
- `matrixDrawChar(x, y, c, color)` — render a single character using the built-in `font8x8_basic` bitmap font (PROGMEM, covers printable ASCII 0x20–0x7E).
- `matrixScrollText(text, color, delayMs)` — non-blocking horizontal scrolling text.
- `matrixShowLetter(c, color)` — display a single large character (used for countdown digits, Pro Mode R/G).
- `matrixShowNumber(n, color)` — display a 1–2 digit number.
- `XY(x, y)` — maps (x, y) coordinates to a linear LED index, respecting the serpentine layout flag.

### `src/sensors.h` / `src/sensors.cpp`
All **hardware input reading**.
- `sensorsSetup()` — `pinMode` for wire, start, finish, PIR pins.
- `isWireTouched()` / `isStartTouched()` / `isFinishTouched()` — debounced digital reads (INPUT_PULLUP, LOW = contact).
- `irCalibrate()` — captures baseline ADC reading (average of 32 samples).
- `irIsMoving()` — returns `true` if current ADC reading deviates from baseline by > `IR_MOVE_THRESHOLD`.
- `pirIsMoving()` — returns `true` if PIR pin is HIGH.

### `src/promode.h` / `src/promode.cpp`
**Pro Mode** traffic light cycling and movement detection.
- `promodeSetup()` — configures red/green LED pins.
- `promodeUpdate()` — called every tick during PLAYING; cycles between GREEN and RED phases based on `PRO_GREEN_DURATION` / `PRO_RED_DURATION`. Updates discrete LEDs and the matrix.
- `promodeIsGreen()` / `promodeIsRed()` — phase queries.
- `promodeMovementDetected()` — checks the configured sensor(s) using `#if` preprocessor guards based on `PRO_MODE_SENSOR`.
- `promodeReset()` — resets the phase timer (called at game start).

---

## Game State Machine Diagram

```
                        ┌──────────┐
                        │   IDLE   │◄──────────────────────────┐
                        └────┬─────┘                           │
                             │ Start pad touched               │
                             ▼                                 │
                        ┌──────────────┐                       │
                        │  COUNTDOWN   │                       │
                        │  3 → 2 → 1  │                       │
                        │     GO!      │                       │
                        └──────┬───────┘                       │
                               │ Countdown complete            │
                               ▼                               │
                        ┌──────────────┐                       │
                ┌──────►│   PLAYING    │◄──┐                   │
                │       └──┬───┬───┬───┘   │                   │
                │          │   │   │       │                   │
                │          │   │   │       │                   │
                │  Wire    │   │   │  Pro Mode:                │
                │  touch   │   │   │  movement during RED      │
                │          │   │   │       │                   │
                │          ▼   │   ▼       │                   │
                │       ┌──────────────┐   │                   │
                │       │    FAIL      │───┘                   │
                │       │  Buzz, red   │                       │
                │       │  strobe,     │──────────────────────►│
                │       │  "GO BACK"   │   after FAIL_DISPLAY_MS
                │       └──────────────┘                       │
                │              │                               │
                │     Finish pad touched                       │
                │              ▼                               │
                │       ┌──────────────┐                       │
                │       │     WIN      │                       │
                │       │  Beep x3,    │──────────────────────►│
                │       │  confetti,   │   after WIN_DISPLAY_MS
                │       │  "WIN! Xs Yf"│                       │
                │       └──────────────┘                       │
                │                                              │
                └──────────────────────────────────────────────┘
```

---

## Configuration Walkthrough (`config.h`)

### LED Strip
```cpp
#define STRIP_PIN          2        // D4 / GPIO2 — data pin
#define STRIP_NUM_LEDS     30       // Number of LEDs on the strip
#define STRIP_BRIGHTNESS   180      // 0–255, controls max brightness
```
Adjust `STRIP_NUM_LEDS` to match your actual strip length.

### LED Matrix
```cpp
#define MATRIX_PIN         0        // D3 / GPIO0
#define MATRIX_WIDTH       8
#define MATRIX_HEIGHT      8
#define MATRIX_BRIGHTNESS  80
#define MATRIX_SERPENTINE  true     // Set false if rows all go left→right
```
Set `MATRIX_SERPENTINE` to `true` for zigzag-wired matrices (most common), `false` for straight-row wiring.

### Game Pins
```cpp
#define WIRE_PIN           4        // D2 / GPIO4
#define START_PIN          5        // D1 / GPIO5
#define FINISH_PIN         12       // D6 / GPIO12
#define BUZZER_PIN         14       // D5 / GPIO14
```
All sensor pins use `INPUT_PULLUP` — LOW means contact/touch.

### Pro Mode
```cpp
#define PRO_MODE_ENABLED   true     // false to disable entirely
#define SENSOR_IR          0
#define SENSOR_PIR         1
#define SENSOR_BOTH        2
#define PRO_MODE_SENSOR    SENSOR_IR   // Choose sensor type
#define PRO_GREEN_DURATION 3000     // Green phase duration (ms)
#define PRO_RED_DURATION   2000     // Red phase duration (ms)
#define IR_MOVE_THRESHOLD  150      // ADC delta threshold
```

### Timing
```cpp
#define DEBOUNCE_MS        50       // Contact debounce window
#define FAIL_DISPLAY_MS    2000     // How long to show fail animation
#define WIN_DISPLAY_MS     5000     // How long to show win animation
#define COUNTDOWN_STEP_MS  1000     // Duration of each countdown step
```

---

## Library Dependencies

| Library | Version | Purpose |
|---|---|---|
| **FastLED** | ^3.6.0 | Drives WS2812B LED strip and 8×8 matrix. Provides `fill_rainbow`, `beatsin8`, `fadeToBlackBy`, CHSV/CRGB colour types, and hardware-accelerated output. |
| **Arduino Core for ESP8266** | (bundled) | Provides `millis()`, `digitalRead()`, `analogRead()`, `Serial`, GPIO management. Installed automatically by PlatformIO when targeting `espressif8266`. |

No WiFi, MQTT, or display libraries are required — the entire project runs standalone with just FastLED.

---

## How Pro Mode Sensor Selection Works

The `PRO_MODE_SENSOR` define in `config.h` is a numeric constant:

```cpp
#define SENSOR_IR    0
#define SENSOR_PIR   1
#define SENSOR_BOTH  2
```

In `promode.cpp`, preprocessor `#if` directives select the correct code path at **compile time**:

```cpp
bool promodeMovementDetected() {
#if PRO_MODE_SENSOR == SENSOR_IR
    return irIsMoving();
#elif PRO_MODE_SENSOR == SENSOR_PIR
    return pirIsMoving();
#elif PRO_MODE_SENSOR == SENSOR_BOTH
    return irIsMoving() || pirIsMoving();
#else
    return false;
#endif
}
```

This means unused sensor code is **not compiled** into the binary, saving flash and RAM on the resource-constrained ESP8266.
