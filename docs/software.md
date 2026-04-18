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
- **Debug toggle** (`DEBUG_LOGGING`) and the `DEBUG_LOG*` macro family
- **Pin definitions** (LED strip, matrix, game pins, sensors, buzzer, Pro Mode LEDs)
- **Feature flags** (`PRO_MODE_ENABLED`, `PRO_MODE_SENSOR`)
- **Tuning constants** (debounce, display durations, countdown timing)
- **LED parameters** (num LEDs, brightness, serpentine flag)
- **Effect timing constants** (`IDLE_UPDATE_MS`, `PLAY_UPDATE_MS`, `WIN_UPDATE_MS`, `STROBE_*`)
- **Scroll speed constants** (`SCROLL_IDLE_MS`, `SCROLL_COUNTDOWN_MS`, `SCROLL_FAIL_MS`, `SCROLL_WIN_MS`)

No `.cpp` counterpart — header-only, included by every module.

### `src/main.cpp`
Minimal entry point:
- `setup()` — calls `sensorsSetup()`, `ledsSetup()`, `matrixSetup()`, `promodeSetup()`, `gameSetup()`, initialises Serial (115200 baud), and prints a boot banner including Pro Mode sensor type and debug status.
- `loop()` — calls `gameLoop()`. All logic lives in the game module.

### `src/game.h` / `src/game.cpp`
The **game state machine** — the brain of the firmware.
- Defines `GameState` enum: `STATE_IDLE`, `STATE_COUNTDOWN`, `STATE_PLAYING`, `STATE_FAIL`, `STATE_WIN`.
- Tracks: current state, timer start, fail count, elapsed time, countdown step.
- `enterState(newState)` — internal helper that logs every transition to Serial, resets the per-state timer, and calls `matrixScrollReset()` so the scroll always starts fresh.
- `gameGetStateName(state)` — returns a human-readable state name (e.g. `"PLAYING"`), useful for debug output.
- Each state has a dedicated handler function:
  - `handleIdle()` — rainbow LEDs, scroll "TOUCH START", wait for start pad.
  - `handleCountdown()` — 3→2→1→GO on matrix, progressive fill on strip.
  - `handlePlaying()` — check wire contact, check finish, update matrix timer, Pro Mode integration.
  - `handleFail()` — buzz, red strobe, "FAIL"/"GO BACK" on matrix, return to IDLE.
  - `handleWin()` — triple beep, confetti, scroll "WIN! Xs Yf", return to IDLE.

### `src/leds.h` / `src/leds.cpp`
All **LED strip effects** using FastLED. Every effect is **non-blocking** (uses `millis()`).
- `ledsSetup()` — initialise FastLED for the strip.
- `ledsIdle()` — smooth rainbow cycle (throttled to `IDLE_UPDATE_MS`).
- `ledsCountdown(step)` — progressive white→green fill based on countdown step.
- `ledsPlaying()` — slow green breathing pulse (BPM=20, throttled to `PLAY_UPDATE_MS`).
- `ledsFail()` — red strobe flash ×`STROBE_FLASH_COUNT` (non-blocking, `STROBE_INTERVAL_MS` per flash).
- `ledsWin()` — multi-colour confetti sparkle (throttled to `WIN_UPDATE_MS`).
- `ledsProGreen()` — fast green pulse (BPM=60, throttled to `PLAY_UPDATE_MS`).
- `ledsProRed()` — steady solid red (throttled to `PLAY_UPDATE_MS` to avoid redundant show calls).
- `ledsClear()` — all off; resets strobe state.

**Internal detail:** `ledsPlaying()` and `ledsProGreen()` share a private `stripBreathe(color, bpm, lo, hi)` helper that applies a `beatsin8` brightness sine wave via per-controller brightness (no global brightness clobber).

### `src/matrix.h` / `src/matrix.cpp`
**8×8 WS2812B matrix** rendering engine.
- `matrixSetup()` — initialise FastLED for the matrix.
- `matrixClear()` — blank the matrix.
- `matrixScrollReset()` — force the next `matrixScrollText()` call to restart from the right edge. Called automatically via `enterState()` on every state transition.
- `matrixDrawChar(x, y, c, color)` — render a single character using the built-in `font8x8_basic` bitmap font (PROGMEM, covers printable ASCII 0x20–0x7E). Uses the `FONT_CHAR_WIDTH=8` constant.
- `matrixScrollText(text, color, delayMs)` — non-blocking horizontal scrolling text. Respects both pointer-change detection and the `scrollDirty` flag set by `matrixScrollReset()`.
- `matrixShowLetter(c, color)` — display a single large character (used for countdown digits, Pro Mode R/G).
- `matrixShowNumber(n, color)` — display a 1–2 digit number (clamped 0–99).
- `XY(x, y)` — maps (x, y) coordinates to a linear LED index, respecting `MATRIX_SERPENTINE`.

### `src/sensors.h` / `src/sensors.cpp`
All **hardware input reading**.
- `sensorsSetup()` — `pinMode` for wire, start, finish, PIR pins.
- `isWireTouched()` / `isStartTouched()` / `isFinishTouched()` — debounced digital reads (INPUT_PULLUP, LOW = contact). The internal `debouncedRead()` helper tracks the raw state separately from the debounce timer — a state change always restarts the quiet-period window.
- `irCalibrate()` — captures a baseline ADC reading (average of `IR_CALIBRATE_SAMPLES` = 32 samples, blocks ~64 ms).
- `irGetBaseline()` — returns the last captured ADC baseline for debug inspection.
- `irIsMoving()` — returns `true` if current ADC reading deviates from baseline by > `IR_MOVE_THRESHOLD`.
- `pirIsMoving()` — returns `true` if PIR pin is HIGH.

### `src/promode.h` / `src/promode.cpp`
**Pro Mode** traffic light cycling and movement detection.
- `promodeSetup()` — configures red/green LED pins (both initially LOW).
- `promodeReset()` — resets the phase timer and starts in GREEN phase (call at game start).
- `promodeUpdate()` — called every tick during PLAYING; cycles between GREEN and RED phases with randomized durations (configured via `PRO_GREEN_MIN`/`PRO_GREEN_MAX` and `PRO_RED_MIN`/`PRO_RED_MAX`). Logs phase transitions to Serial. Matrix display is fully owned by `game.cpp`.
- `promodeIsGreen()` / `promodeIsRed()` — phase queries.
- `promodeMovementDetected()` — checks the configured sensor(s) using `#if` preprocessor guards based on `PRO_MODE_SENSOR`.

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

### Debug Logging
```cpp
#define DEBUG_LOGGING      0    // Set to 1 for verbose output; 0 for production
```
When `DEBUG_LOGGING=1` the following macros emit output to Serial:
- `DEBUG_LOG("msg")` — `Serial.print(F("msg"))`
- `DEBUG_LOGLN("msg")` — `Serial.println(F("msg"))`
- `DEBUG_LOG_VAL("label", value)` — prints label then value
- `DEBUG_LOG_2("l1", v1, "l2", v2)` — prints two labelled values on one line

When `DEBUG_LOGGING=0` (default) all macros expand to nothing — zero overhead.

Use the dedicated **debug build** in PlatformIO to enable it without editing `config.h`:
```bash
pio run -e d1_mini_debug -t upload
```

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
#define MATRIX_SERPENTINE  1        // 1 = zigzag wiring, 0 = progressive
```
Set `MATRIX_SERPENTINE` to `1` for zigzag-wired matrices (most common), `0` for straight-row wiring.

### Game Pins
```cpp
#define WIRE_PIN           4        // D2 / GPIO4
#define START_PIN          5        // D1 / GPIO5
#define FINISH_PIN         12       // D6 / GPIO12
#define BUZZER_PIN         15       // D8 / GPIO15
```
All sensor pins use `INPUT_PULLUP` — LOW means contact/touch.

### Pro Mode
```cpp
#define PRO_MODE_ENABLED   1        // 1 = enabled, 0 = disabled
#define SENSOR_IR          0
#define SENSOR_PIR         1
#define SENSOR_BOTH        2
#define PRO_MODE_SENSOR    SENSOR_IR   // Choose sensor type
#define PRO_GREEN_MIN      2000     // Green phase min duration (ms)
#define PRO_GREEN_MAX      4000     // Green phase max duration (ms)
#define PRO_RED_MIN        1500     // Red phase min duration (ms)
#define PRO_RED_MAX        3000     // Red phase max duration (ms)
#define IR_CALIBRATE_SAMPLES 32     // IR baseline averaging samples
#define IR_MOVE_THRESHOLD  150      // ADC delta threshold
```

### Timing
```cpp
#define DEBOUNCE_MS           50    // Contact debounce window
#define FAIL_DISPLAY_MS     2000    // How long to show fail animation
#define WIN_DISPLAY_MS      5000    // How long to show win animation
#define COUNTDOWN_STEP_MS   1000    // Duration of each countdown step
```

### Effect Timing (new)
```cpp
#define IDLE_UPDATE_MS        20    // Rainbow refresh interval
#define PLAY_UPDATE_MS        30    // Playing/pro-green pulse refresh
#define WIN_UPDATE_MS         15    // Confetti refresh interval
#define STROBE_INTERVAL_MS   150    // On/off time per flash
#define STROBE_FLASH_COUNT     3    // Number of red flashes on fail
```

### Scroll Speeds (new)
```cpp
#define SCROLL_IDLE_MS        80    // "TOUCH START" scroll speed
#define SCROLL_COUNTDOWN_MS   30    // "GO!" scroll speed
#define SCROLL_FAIL_MS        60    // "FAIL" / "GO BACK" scroll speed
#define SCROLL_WIN_MS         80    // WIN results scroll speed
```
Smaller value = faster scroll (ms per pixel column advance).

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

