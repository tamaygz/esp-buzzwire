# ESP Buzzwire Game — Product Requirements Document

## Overview

A modern, ESP8266-based buzzwire skill game with individually addressable RGB LED strip effects (FastLED), an 8x8 RGB LED matrix for countdown/messages, configurable IR/PIR-based Pro Mode movement detection, and a full game state machine (wait → playing → fail → win).

---

## Hardware

### Core Controller
- **MCU:** ESP8266 (NodeMCU or Wemos D1 Mini recommended)
- **Framework:** Arduino (PlatformIO or Arduino IDE)

### LED Output
- **LED Strip:** WS2812B individually addressable RGB strip
  - Library: FastLED
  - Data pin: D4 (GPIO2) — configurable via `config.h`
  - Typical length: 30–60 LEDs (configurable)
- **8x8 RGB LED Matrix:** WS2812B-based 8x8 matrix (64 LEDs)
  - Used for: countdown, scrolling messages, score, game state icons
  - Data pin: D3 (GPIO0) — configurable via `config.h`
  - Library: FastLED + custom font renderer (font8x8)

### Game Sensors
- **Buzzwire contact detection:** Digital GPIO with INPUT_PULLUP
  - Game wire → GND
  - Ring/wand handle → GPIO (pulled HIGH; contact = LOW)
  - Pin: D2 (GPIO4) — configurable
- **Start pad:** Metal touch pad connected to GPIO with INPUT_PULLUP
  - Pin: D1 (GPIO5) — configurable
- **Finish pad:** Metal touch pad connected to GPIO with INPUT_PULLUP
  - Pin: D6 (GPIO12) — configurable

### Pro Mode Sensors (configurable, select one or both)
- **PIR Motion Sensor** (e.g., HC-SR501)
  - Digital output → GPIO
  - Pin: D7 (GPIO13) — configurable
  - Detects gross body/hand movement in zone
- **IR Proximity Sensor** (e.g., TCRT5000 or Sharp GP2Y0A)
  - Analog output → A0
  - Detects ring/handle movement via distance delta
  - Pin: A0 — fixed (only analog pin on ESP8266)

### Audio
- **Active buzzer:** Digital GPIO
  - Pin: D8 (GPIO15) — configurable

### Pro Mode Traffic Light (optional discrete LEDs)
- **Red LED** → GPIO with 330Ω resistor, Pin: D5 (GPIO14)
- **Green LED** → GPIO with 330Ω resistor, Pin: D0 (GPIO16)

---

## Pin Summary

| Function              | ESP8266 Pin | GPIO  | Notes                        |
|-----------------------|-------------|-------|------------------------------|
| LED Strip (FastLED)   | D4          | GPIO2 | WS2812B data                 |
| LED Matrix (FastLED)  | D3          | GPIO0 | 8x8 WS2812B data             |
| Buzzwire contact      | D2          | GPIO4 | INPUT_PULLUP, LOW = contact  |
| Start pad             | D1          | GPIO5 | INPUT_PULLUP, LOW = touched  |
| Finish pad            | D6          | GPIO12| INPUT_PULLUP, LOW = touched  |
| Buzzer                | D8          | GPIO15| Active buzzer                |
| PIR sensor            | D7          | GPIO13| Digital HIGH = motion        |
| IR proximity (analog) | A0          | A0    | Analog, delta detection      |
| Red LED (Pro Mode)    | D5          | GPIO14| 330Ω resistor                |
| Green LED (Pro Mode)  | D0          | GPIO16| 330Ω resistor                |

---

## Software Architecture

### File Structure

```
esp-buzzwire/
├── src/
│   ├── main.cpp              # Main game loop & state machine
│   ├── config.h              # All pin & tuning constants
│   ├── game.h / game.cpp     # Game state machine logic
│   ├── leds.h / leds.cpp     # FastLED strip effects
│   ├── matrix.h / matrix.cpp # 8x8 matrix rendering & font
│   ├── sensors.h / sensors.cpp # Wire, start, finish, PIR, IR reads
│   └── promode.h / promode.cpp # Pro Mode traffic light + detection
├── platformio.ini            # PlatformIO build config
├── README.md
└── prd.md                    # This file
```

### Libraries Required

| Library              | Purpose                          |
|----------------------|----------------------------------|
| FastLED              | WS2812B strip + matrix control   |
| ESP8266WiFi          | (optional) WiFi/OTA support      |
| Arduino core ESP8266 | Base framework                   |

---

## Game States

```
IDLE
 └─► [Player touches START pad]
       └─► COUNTDOWN (3-2-1 on matrix)
             └─► PLAYING
                   ├─► [Wire touched]      → FAIL → IDLE
                   ├─► [Pro: moved on RED] → FAIL → IDLE
                   └─► [Finish pad touched] → WIN → IDLE
```

### State Descriptions

#### IDLE
- LED strip: slow rainbow cycle
- Matrix: scrolling "TOUCH START" message
- Waiting for start pad contact

#### COUNTDOWN
- Matrix: displays "3", "2", "1", "GO!" with 1s delay each, in large color digits
- LED strip: chase/fill animation building up to GO
- Game variables reset (timer, fail count)

#### PLAYING
- Timer starts on GO
- LED strip: steady green glow or slow pulse
- Matrix: shows elapsed time in seconds (updated every second)
- Pro Mode: traffic light cycles RED/GREEN (configurable durations)
  - During GREEN: player may move freely
  - During RED: any detected movement = instant FAIL
  - Matrix shows large R (red) or G (green) during Pro Mode
  - Red/Green discrete LEDs mirror the matrix state

#### FAIL
- Buzzer: 500ms tone
- LED strip: red flash strobe (3×)
- Matrix: "FAIL" + fail count, then "GO BACK"
- Timer RESETS
- Fail counter increments
- Returns to IDLE after 2s (player must return to start pad and touch again)

#### WIN
- Buzzer: 3 short beeps
- LED strip: fireworks / confetti animation (5s)
- Matrix: scrolling "WIN!" + elapsed time + fail count
- Returns to IDLE after 5s

---

## Pro Mode

### Overview
Pro Mode adds a traffic-light challenge: a red/green LED cycle controls when the player is allowed to move. Moving during RED = instant fail (same penalty as touching the wire).

### Configuration (`config.h`)
```cpp
#define PRO_MODE_ENABLED     true   // Master toggle
#define PRO_MODE_SENSOR      IR     // IR, PIR, or BOTH
#define PRO_GREEN_MIN        2000   // ms min player may move
#define PRO_GREEN_MAX        4000   // ms max player may move
#define PRO_RED_MIN          1500   // ms min player must freeze
#define PRO_RED_MAX          3000   // ms max player must freeze
#define IR_MOVE_THRESHOLD    150    // ADC delta to count as movement
#define PIR_PIN              D7     // PIR digital input pin
```

### Sensor Options

#### IR Proximity (TCRT5000 / Sharp analog)
- At game start, baseline ADC reading is captured
- During RED phase, ADC is sampled continuously
- If `abs(current - baseline) > IR_MOVE_THRESHOLD` → movement detected → FAIL
- **Mount:** Aim at the path where the player's hand travels along the wire

#### PIR (HC-SR501)
- Digital HIGH = motion in zone
- During RED phase, if PIR pin goes HIGH → FAIL
- **Mount:** Point at player's arm/hand area above the wire
- **Tune:** Adjust sensitivity pot on HC-SR501 to minimum needed range

#### BOTH mode
- Either sensor triggering during RED = FAIL (OR logic)

---

## LED Strip Effects (FastLED)

| State      | Effect                          | Color(s)          |
|------------|---------------------------------|-------------------|
| IDLE       | Rainbow cycle                   | Full spectrum     |
| COUNTDOWN  | Progressive fill left→right     | White → Green     |
| PLAYING    | Slow breathing pulse            | Green             |
| PRO GREEN  | Fast pulse                      | Green             |
| PRO RED    | Steady solid                    | Red               |
| FAIL       | Strobe flash x3                 | Red               |
| WIN        | Confetti / sparkle              | Multi-color       |

---

## 8x8 Matrix Display

### Font
- Custom 5×7 or 8×8 bitmap font stored in PROGMEM
- Supports: A–Z, 0–9, basic symbols (!, ?, ., space)
- Characters rendered via `drawChar(x, y, c, color)` helper

### Matrix Layout
- Serpentine (zigzag) WS2812B matrix
- XY → LED index mapping configurable for row direction and start corner

### Displayed Content

| State      | Matrix Content                        |
|------------|---------------------------------------|
| IDLE       | Scrolling "TOUCH START"               |
| COUNTDOWN  | Large "3" → "2" → "1" → "GO!"        |
| PLAYING    | Elapsed time (e.g., "12s")            |
| PRO Mode   | Large "G" (green) or "R" (red)        |
| FAIL       | "FAIL" flash → "GO BACK"             |
| WIN        | Scrolling "WIN! Xs Yf" (time, fails)  |

---

## Configuration (`config.h`)

All hardware pins, thresholds, and feature flags are centralised in `config.h`:

```cpp
// ── LED Strip ────────────────────────────────────────
#define STRIP_PIN          D4
#define STRIP_NUM_LEDS     30
#define STRIP_BRIGHTNESS   180

// ── LED Matrix ───────────────────────────────────────
#define MATRIX_PIN         D3
#define MATRIX_WIDTH       8
#define MATRIX_HEIGHT      8
#define MATRIX_BRIGHTNESS  80
#define MATRIX_SERPENTINE  true   // true = zigzag wiring

// ── Game Pins ────────────────────────────────────────
#define WIRE_PIN           D2     // Buzzwire contact
#define START_PIN          D1     // Start pad
#define FINISH_PIN         D6     // Finish pad
#define BUZZER_PIN         D8

// ── Pro Mode ─────────────────────────────────────────
#define PRO_MODE_ENABLED   true
#define PRO_MODE_SENSOR    IR     // IR | PIR | BOTH
#define PRO_GREEN_MIN      2000
#define PRO_GREEN_MAX      4000
#define PRO_RED_MIN        1500
#define PRO_RED_MAX        3000
#define IR_PIN             A0
#define IR_MOVE_THRESHOLD  150
#define PIR_PIN            D7

// ── Pro Mode Traffic Light LEDs ──────────────────────
#define PRO_RED_LED_PIN    D5
#define PRO_GREEN_LED_PIN  D0

// ── Tuning ───────────────────────────────────────────
#define DEBOUNCE_MS        50
#define FAIL_DISPLAY_MS    2000
#define WIN_DISPLAY_MS     5000
#define COUNTDOWN_STEP_MS  1000
```

---

## Wiring Diagram (Textual)

```
ESP8266 NodeMCU / Wemos D1 Mini
─────────────────────────────────────────────────────────
                    ┌──────────────┐
     5V PSU ───────►│ 5V      GND  │◄─── GND (common)
                    │              │
    WS2812B strip──►│ D4           │
    WS2812B matrix─►│ D3           │
    Buzzwire wire──►│ D2  (PULLUP) │◄─── wire → GND
    Start pad ─────►│ D1  (PULLUP) │◄─── pad → GND
    Finish pad ────►│ D6  (PULLUP) │◄─── pad → GND
    Buzzer (+) ────►│ D8           │
    PIR OUT ───────►│ D7           │
    Red LED (+) ───►│ D5           │──► 330Ω ──► LED ──► GND
    Green LED (+) ─►│ D0           │──► 330Ω ──► LED ──► GND
    IR sensor OUT ─►│ A0           │
                    └──────────────┘

Ring/wand handle ──── long flexible wire ──── GND
Game wire path   ──── D2 (INPUT_PULLUP)
```

---

## Power Budget

| Component          | Current (est.) |
|--------------------|---------------|
| ESP8266            | ~80 mA        |
| WS2812B strip (30) | ~450 mA max   |
| WS2812B matrix (64)| ~960 mA max   |
| Buzzer             | ~30 mA        |
| PIR / IR sensors   | ~10 mA        |
| **Total (max)**    | **~1.5 A**    |

> Use a 5V 2A+ PSU. Power the LED strip and matrix **directly from the PSU**, not through the ESP8266 3.3V/5V pins. Share GND.

---

## Future / Optional Extensions

- [ ] WiFi web dashboard (live timer, leaderboard via ESP8266 AP mode)
- [ ] SPIFFS/LittleFS high-score storage (top 5 times)
- [ ] OTA firmware updates
- [ ] Microphone secondary movement detector (sound spike during red)
- [ ] Bluetooth RSSI player registration
- [ ] Difficulty selector (button cycles through Easy/Normal/Pro)
- [ ] Lives system (3 fails = game over instead of per-fail reset)
- [ ] NFC tag player registration
