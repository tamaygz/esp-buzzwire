# Getting Started

Step-by-step guide to building, flashing, and running the ESP Buzzwire Game for the first time.

---

## Prerequisites

### Hardware
- An assembled buzzwire game (see [docs/hardware.md](hardware.md) and [docs/wiring.md](wiring.md))
- ESP8266 board connected via USB

### Software (choose one)

#### Option A: PlatformIO (Recommended)
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
3. PlatformIO will automatically install the ESP8266 platform and FastLED library

#### Option B: Arduino IDE
1. Install [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x)
2. Add ESP8266 board support:
   - Go to **File → Preferences → Additional Board Manager URLs**
   - Add: `https://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Go to **Tools → Board → Board Manager** → search "ESP8266" → Install
3. Install FastLED library:
   - Go to **Sketch → Include Library → Manage Libraries**
   - Search "FastLED" → Install version 3.6.0+
4. Select your board:
   - **Tools → Board → ESP8266 Boards → LOLIN(WEMOS) D1 mini** (or **NodeMCU 1.0**)

### USB Drivers
- **CH340** driver (most D1 Mini clones): [Download](https://www.wch-ic.com/downloads/CH341SER_EXE.html)
- **CP2102** driver (some NodeMCU boards): [Download](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

---

## Step-by-Step: Clone → Configure → Flash

### 1. Clone the Repository

```bash
git clone https://github.com/tamaygz/esp-buzzwire.git
cd esp-buzzwire
```

### 2. Configure `src/config.h`

Open `src/config.h` and adjust for your hardware:

```cpp
// Match your LED strip length:
#define STRIP_NUM_LEDS     30       // Change to your actual LED count

// Enable/disable Pro Mode:
#define PRO_MODE_ENABLED   true     // Set false if no IR/PIR sensors

// Choose Pro Mode sensor:
#define PRO_MODE_SENSOR    SENSOR_IR   // SENSOR_IR, SENSOR_PIR, or SENSOR_BOTH
```

See [docs/software.md](software.md) for a full configuration walkthrough.

### 3. Flash the Firmware

#### With PlatformIO (CLI)

```bash
# Build for Wemos D1 Mini
pio run -e d1_mini

# Upload
pio run -e d1_mini -t upload

# Open serial monitor
pio device monitor -b 115200
```

#### With PlatformIO (VS Code)

1. Open the project folder in VS Code
2. Click the **PlatformIO: Upload** button (→ arrow icon in the bottom toolbar)
3. Click the **PlatformIO: Serial Monitor** button (plug icon)

#### With Arduino IDE

1. Open `src/main.cpp`
2. Select your board and port under **Tools**
3. Click **Upload** (→ arrow button)
4. Open **Tools → Serial Monitor** at 115200 baud

---

## Serial Monitor Output

When the firmware boots, you'll see:

```
=== ESP Buzzwire Game ===
[IR] Calibrated baseline: 512
[INIT] Pro Mode: ENABLED
[INIT] Ready — touch the START pad!
```

During gameplay:

```
[GAME] Start touched — countdown!
[GAME] Countdown: 3
[GAME] Countdown: 2
[GAME] Countdown: 1
[GAME] GO!
[GAME] Playing — timer started
[GAME] FAIL #1
[GAME] Returning to IDLE — go back to start
```

On a win:

```
[GAME] WIN! Time: 23.4s  Fails: 2
[GAME] Returning to IDLE
```

---

## First Run Checklist

After flashing, verify each feature works:

- [ ] **Serial Monitor** shows boot messages at 115200 baud
- [ ] **LED strip** shows rainbow idle animation
- [ ] **Matrix** scrolls "TOUCH START"
- [ ] **Touch start pad** → countdown begins (3-2-1-GO on matrix)
- [ ] **Touch game wire** (with ring) → buzzer sounds, strip flashes red, matrix shows "FAIL"
- [ ] **Touch finish pad** → win animation plays
- [ ] **Pro Mode LEDs** alternate red/green (if PRO_MODE_ENABLED)
- [ ] **IR sensor** baseline prints to Serial on game start
- [ ] **PIR sensor** triggers fail when moving during red (if using PIR)

If anything doesn't work, see [docs/troubleshooting.md](troubleshooting.md).
