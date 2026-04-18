# 🎮 ESP Buzzwire Game

A modern, ESP8266-powered buzzwire skill game with individually addressable RGB LED effects, an 8×8 LED matrix for countdown and messages, and an optional Pro Mode with IR/PIR movement detection.

---

## ✨ Features

- 🌈 **FastLED Strip Effects** — rainbow idle, countdown fill, green pulse, red strobe fail, confetti win
- 🔢 **8×8 RGB Matrix** — countdown digits, scrolling messages, elapsed timer, Pro Mode indicator
- ⏱️ **Full Game State Machine** — IDLE → COUNTDOWN → PLAYING → FAIL/WIN → IDLE
- 🔴🟢 **Pro Mode** — traffic-light challenge with configurable IR and/or PIR movement detection
- 🔊 **Buzzer Feedback** — fail tone and win beeps
- ⚡ **Non-Blocking Gameplay** — all game effects use `millis()`, no delays in the main loop; IR sensor calibration uses a short ~64 ms blocking average at game start
- 🛠️ **Highly Configurable** — all pins, thresholds, and timings in a single `config.h`

---

## 🧰 Hardware Requirements

| Component | Purpose |
|---|---|
| ESP8266 (D1 Mini / NodeMCU) | Main controller |
| WS2812B LED Strip (30–60 LEDs) | Light effects along wire path |
| 8×8 WS2812B LED Matrix | Countdown, messages, score |
| Active Buzzer | Sound feedback |
| Copper wire (2–3 mm) | Game wire path |
| Metal ring + handle | Player's wand |
| Start & finish pads | Touch detection |
| IR sensor (TCRT5000) | Pro Mode movement detection (optional) |
| PIR sensor (HC-SR501) | Pro Mode movement detection (optional) |
| 5 V 2 A+ PSU | Powers everything |

See [docs/hardware.md](docs/hardware.md) for the full component list with part numbers and sourcing notes.

---

## 🚀 Quick Start

### 1. Clone
```bash
git clone https://github.com/tamaygz/esp-buzzwire.git
cd esp-buzzwire
```

### 2. Configure
Edit `src/config.h` to match your hardware (LED count, Pro Mode sensor, etc.).

### 3. Flash
```bash
# PlatformIO (recommended)
pio run -e d1_mini -t upload
pio device monitor -b 115200
```

See [docs/getting-started.md](docs/getting-started.md) for detailed instructions including Arduino IDE setup.

---

## 📖 Documentation

| Document | Description |
|---|---|
| [docs/hardware.md](docs/hardware.md) | Component list, power budget, level shifting |
| [docs/wiring.md](docs/wiring.md) | Pin connections, ASCII wiring diagram, pad construction |
| [docs/software.md](docs/software.md) | Architecture, state machine, module descriptions |
| [docs/getting-started.md](docs/getting-started.md) | Prerequisites, step-by-step flash guide |
| [docs/pro-mode.md](docs/pro-mode.md) | Pro Mode setup, IR/PIR configuration, timing |
| [docs/calibration.md](docs/calibration.md) | Sensor tuning, Serial Monitor tips |
| [docs/troubleshooting.md](docs/troubleshooting.md) | Common issues and fixes |
| [prd.md](prd.md) | Product requirements document |

---

## 🔌 Wiring Overview

```
ESP8266 Pin Map
────────────────────────────────────
D4 (GPIO2)  → WS2812B Strip DIN
D3 (GPIO0)  → 8×8 Matrix DIN
D2 (GPIO4)  → Buzzwire contact
D1 (GPIO5)  → Start pad
D6 (GPIO12) → Finish pad
D5 (GPIO14) → Red LED (Pro Mode)
D7 (GPIO13) → PIR sensor
A0          → IR sensor
D8 (GPIO15) → Buzzer
D0 (GPIO16) → Green LED (Pro Mode)
────────────────────────────────────
```

<!-- TODO: Add wiring photo/diagram image -->
<!-- ![Wiring Diagram](docs/images/wiring.png) -->

---

## 🎯 Game Flow

```
[IDLE] → rainbow LEDs, "TOUCH START" on matrix
   │
   ▼ (touch start pad)
[COUNTDOWN] → 3-2-1-GO on matrix
   │
   ▼
[PLAYING] → timer running, Pro Mode active
   │
   ├──► Wire touch → [FAIL] → buzz, red flash → back to [IDLE]
   ├──► Move during RED → [FAIL] → same penalty
   └──► Finish pad → [WIN] → confetti, beeps → back to [IDLE]
```

---

## 📁 Project Structure

```
esp-buzzwire/
├── src/
│   ├── main.cpp          # Entry point
│   ├── config.h          # All pins & tuning constants
│   ├── game.h/.cpp       # Game state machine
│   ├── leds.h/.cpp       # LED strip effects (FastLED)
│   ├── matrix.h/.cpp     # 8×8 matrix rendering & font
│   ├── sensors.h/.cpp    # Wire, start, finish, IR, PIR reads
│   └── promode.h/.cpp    # Pro Mode traffic light & detection
├── docs/                 # Full documentation
├── platformio.ini        # PlatformIO build config
├── prd.md                # Product requirements
└── README.md
```

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
