# Calibration Guide

How to tune sensors and timing for optimal gameplay. Use the Serial Monitor (115200 baud) throughout this process.

---

## Wire Contact Detection (Debounce Tuning)

### How It Works
The game wire is connected to a GPIO pin configured as `INPUT_PULLUP`. Normally the pin reads HIGH. When the grounded ring touches the wire, the pin is pulled LOW.

### The Problem
Mechanical contact can cause **bouncing** — rapid HIGH/LOW transitions as the metal surfaces make and break contact. Without debouncing, a single touch might register as multiple contacts.

### Tuning `DEBOUNCE_MS`

In `src/config.h`:
```cpp
#define DEBOUNCE_MS        50       // Milliseconds
```

- **Too low** (< 20 ms): May register false double-contacts.
- **Too high** (> 100 ms): May miss quick touches or feel unresponsive.
- **Default (50 ms)**: Good starting point for most setups.

### Testing
1. Flash the firmware and open Serial Monitor.
2. During PLAYING state, quickly tap the ring against the wire.
3. Each tap should produce exactly one `[GAME] FAIL #N` message.
4. If you see multiple FAILs per tap, **increase** DEBOUNCE_MS.
5. If taps are occasionally missed, **decrease** DEBOUNCE_MS.

---

## IR Sensor Calibration

### Baseline Capture (`irCalibrate`)

At the start of each game (after countdown), the firmware reads the IR sensor 32 times and averages the result. This becomes the **baseline**.

Serial output:
```
[IR] Calibrated baseline: 512
```

### Finding the Right Threshold

In `src/config.h`:
```cpp
#define IR_MOVE_THRESHOLD  150      // ADC units (0–1023 range)
```

#### Step-by-Step

1. **Enable debug output** — set `DEBUG_LOGGING=1` in `config.h` or flash the debug build:
   ```bash
   pio run -e d1_mini_debug -t upload
   ```

2. **Observe live IR values** in the Serial Monitor during PLAYING state:
   ```
   [IR] val=520 delta=8      ← hand still
   [IR] val=531 delta=19
   [IR] val=710 delta=198    ← hand moved
   ```

3. **Set threshold** between the still-delta and moving-delta:
   - Still delta ~30, moving delta ~200 → set threshold to 100–150.

4. **Disable debug output** when done (`DEBUG_LOGGING=0`) to restore production performance.

### Mounting Tips
- Mount the sensor at 3–8 cm from the typical hand position.
- Ensure no external light sources (sunlight, lamps) directly hit the sensor — this causes drift.
- Use a small cardboard shroud around the sensor to block ambient IR.

---

## PIR Sensor Tuning

### Hardware Adjustment

The HC-SR501 has two potentiometers:

```
    ┌──────────────────────┐
    │  [Sensitivity]  [Time]│
    │      pot         pot  │
    │                       │
    │   ┌──────────────┐    │
    │   │  Fresnel Lens │    │
    │   └──────────────┘    │
    └──────────────────────┘
```

| Pot | Turn CW | Turn CCW |
|---|---|---|
| **Sensitivity** | Longer range (~7 m) | Shorter range (~3 m) |
| **Time delay** | Longer re-trigger (~300 s) | Shorter re-trigger (~3 s) |

**Recommended settings for the game:**
- Sensitivity: Turn **CCW** until only nearby movement triggers it (2–3 m range).
- Time delay: Turn **CCW** to minimum (fastest re-trigger).

### Retrigger Mode

Most HC-SR501 modules have a jumper:
- **H (repeatable trigger)**: Stays HIGH as long as motion continues. Usually the default.
- **L (single trigger)**: Goes HIGH once, then LOW after the time delay, regardless of continued motion.

**For the game, H mode is recommended** — it keeps the output HIGH as long as the player is moving, giving a more accurate reading.

### Testing

1. Flash with `PRO_MODE_SENSOR` set to `SENSOR_PIR`.
2. Start a game and enter PLAYING state.
3. During RED phase:
   - Stay still → no fail.
   - Wave your hand → should trigger fail.
4. If the PIR triggers too easily (false positives):
   - Reduce sensitivity (turn pot CCW).
   - Ensure no heat sources (heaters, sunlight) are in the sensor's field of view.
   - Try masking part of the Fresnel lens with tape to narrow the detection zone.
5. If the PIR doesn't trigger:
   - Increase sensitivity (turn pot CW).
   - Ensure the sensor has a clear line-of-sight to the player.
   - Wait 30–60 seconds after power-up for the sensor to stabilise.

---

## Using Serial Monitor for Live Sensor Values

### Connecting
```bash
# PlatformIO
pio device monitor -b 115200

# Arduino IDE
# Tools → Serial Monitor → 115200 baud
```

### What You'll See

| Message | Meaning |
|---|---|
| `╔══ ESP Buzzwire v1.0.0 ╗` | Boot banner — Serial is working |
| `[SENSORS] Setup complete` | Sensor pins configured |
| `[LEDS] Setup complete` | LED strip initialised |
| `[MATRIX] Setup complete` | Matrix initialised |
| `[INIT] Pro Mode: ENABLED  sensor=IR` | Pro Mode active, IR selected |
| `[IR] Calibrated baseline: 512` | IR baseline captured at game start |
| `[PROMODE] → RED phase (freeze!)` | Red phase started |
| `[PROMODE] → GREEN phase (safe to move)` | Green phase started |
| `[GAME] IDLE → COUNTDOWN` | Start pad touched |
| `[GAME] Countdown: 3` | Countdown digit displayed |
| `[GAME] COUNTDOWN → PLAYING` | Game timer started |
| `[GAME] Wire touched — FAIL #1` | Wire contact detected |
| `[GAME] Pro Mode movement during RED — FAIL #2` | Pro Mode movement fail |
| `[GAME] Finish touched — WIN! time=15.2s fails=1` | Player reached finish |
| `[GAME] PLAYING → WIN` | Entering win state |
| `[GAME] WIN → IDLE` | Returning to idle |

### Enabling Verbose Debug Output

Set `DEBUG_LOGGING` in `src/config.h` to enable detailed sensor and LED logging:

```cpp
// src/config.h
#define DEBUG_LOGGING      1    // Change from 0 to 1
```

Or use the dedicated PlatformIO debug environment (no file edit needed):
```bash
pio run -e d1_mini_debug -t upload
```

When enabled you will also see (example):
```
[SENSORS] Wire TOUCHED
[IR] val=687 delta=175
[PIR] Motion detected
[MATRIX] Scroll reset
[MATRIX] Scroll started: TOUCH START
[LEDS] Cleared
```

> **Note:** Debug output adds overhead. Disable it (`DEBUG_LOGGING=0`) for production builds.

### IR Sensor — Live Value Monitoring

With `DEBUG_LOGGING=1`, `irIsMoving()` prints every ADC reading to Serial. Use this to determine the right `IR_MOVE_THRESHOLD`:

1. Flash the debug build (`pio run -e d1_mini_debug -t upload`).
2. Start a game, complete the countdown (which runs `irCalibrate()`).
3. During PLAYING, watch the Serial output for lines like:
   ```
   [IR] val=520 delta=8
   [IR] val=531 delta=19
   [IR] val=710 delta=198    ← hand moved
   ```
4. Set `IR_MOVE_THRESHOLD` between the still-delta and moving-delta values.



---

## Common Calibration Pitfalls

| Problem | Cause | Fix |
|---|---|---|
| IR baseline drifts over time | Temperature change, ambient light | Re-calibrate (restart game), shield sensor from light |
| PIR always HIGH after power-on | Warm-up period (~30–60s) | Wait for sensor to stabilise before starting |
| Wire contact not detected | Loose connection, oxidised wire | Clean contacts, check GND continuity from ring to ESP |
| Start/finish pad unreliable | Dry skin, poor contact area | Use larger pads, add a bit of conductive gel, or use a direct GND-to-GPIO bridge approach |
| Double-fail on single wire touch | Debounce too low | Increase `DEBOUNCE_MS` to 80–100 |
| Game feels unresponsive | Debounce too high | Decrease `DEBOUNCE_MS` to 30–40 |
| IR too sensitive | Threshold too low | Increase `IR_MOVE_THRESHOLD` (try 200–300) |
| IR not sensitive enough | Threshold too high or sensor too far | Decrease threshold (try 50–80), move sensor closer |
