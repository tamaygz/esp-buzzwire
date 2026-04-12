# Pro Mode Guide

Pro Mode adds a **traffic-light challenge** to the buzzwire game: a red/green cycle controls when the player is allowed to move. Moving during RED = instant fail (same penalty as touching the wire).

---

## How It Works

1. When the game enters the **PLAYING** state, a traffic light cycle begins.
2. The cycle alternates between:
   - 🟢 **GREEN phase** — player may move freely (default: 3 seconds)
   - 🔴 **RED phase** — player must freeze (default: 2 seconds)
3. During RED, one or both sensors monitor for movement.
4. If movement is detected during RED → **instant FAIL** — same as touching the wire (buzzer, red flash, timer reset, fail count incremented).
5. The traffic light state is shown on:
   - **Discrete LEDs**: red LED on D8, green LED on D0
   - **8×8 matrix**: large "R" (red) or "G" (green) letter

---

## Enabling / Disabling Pro Mode

In `src/config.h`:

```cpp
// Master toggle — set to false to disable Pro Mode entirely
#define PRO_MODE_ENABLED   true
```

When disabled:
- No traffic light cycling occurs
- No movement detection
- Matrix shows elapsed time instead of R/G letters
- LED strip shows normal green breathing pulse

---

## Sensor Selection

```cpp
// Sensor type constants
#define SENSOR_IR          0
#define SENSOR_PIR         1
#define SENSOR_BOTH        2

// Choose which sensor(s) to use during Pro Mode
#define PRO_MODE_SENSOR    SENSOR_IR   // Change to SENSOR_PIR or SENSOR_BOTH
```

### IR Sensor (SENSOR_IR)

**How it works:**
- An analog IR proximity sensor (e.g., TCRT5000, Sharp GP2Y0A21) reads distance/reflectance.
- At game start, a **baseline** ADC reading is captured (average of 32 samples).
- During RED phase, the ADC is read continuously.
- If `|current - baseline| > IR_MOVE_THRESHOLD` → movement detected.

**How to mount:**
- Attach the IR sensor to the wire frame, pointing at the area where the player's hand travels along the wire.
- The sensor should be 2–10 cm from the hand path, depending on sensor type.
- Ensure the sensor is stable and won't vibrate from player movement.

**Calibration:**
- Adjust `IR_MOVE_THRESHOLD` in `config.h` (default: 150).
- Open Serial Monitor and read live ADC values to find the right threshold.
- See [docs/calibration.md](calibration.md) for detailed instructions.

### PIR Sensor (SENSOR_PIR)

**How it works:**
- A PIR (passive infrared) motion sensor (e.g., HC-SR501) detects body heat movement.
- Digital output: HIGH = motion detected, LOW = no motion.
- During RED phase, if the PIR pin reads HIGH → movement detected.

**How to mount:**
- Point the PIR sensor at the player's arm/hand area above the wire.
- The sensor dome should have a clear line-of-sight to the play zone.
- Mount at a height of ~20–30 cm above the wire path.

**Sensitivity adjustment:**
- The HC-SR501 has a **sensitivity potentiometer** — turn counter-clockwise to reduce detection range.
- Set it so only movement within the immediate play area is detected.
- There's also a **retrigger delay potentiometer** — set to minimum for fastest re-detection.

**Retrigger delay note:**
- HC-SR501 has a built-in retrigger delay (typically 2–5 seconds after the last trigger).
- This means if the player moves once during RED, the sensor stays HIGH for a few seconds.
- For the game this is fine — the first movement triggers the fail.
- For more responsive detection, consider the **RCWL-0516** microwave sensor or use IR mode instead.

### BOTH Mode (SENSOR_BOTH)

```cpp
#define PRO_MODE_SENSOR    SENSOR_BOTH
```

- Uses OR logic: either IR or PIR triggering during RED = FAIL.
- This provides the most coverage: IR detects hand movement near the wire; PIR detects gross body movement.
- Both sensors must be wired (IR on A0, PIR on D7).

---

## Traffic Light Timing

```cpp
#define PRO_GREEN_DURATION 3000     // ms — player may move
#define PRO_RED_DURATION   2000     // ms — player must freeze
```

### Tuning Tips

| Setting | Effect |
|---|---|
| Longer green, shorter red | Easier — more time to move, less freeze time |
| Shorter green, longer red | Harder — less time to move, more freeze time |
| Equal green and red | Balanced difficulty |

### Difficulty Presets (suggestions)

| Difficulty | Green | Red |
|---|---|---|
| Easy | 5000 ms | 1000 ms |
| Normal | 3000 ms | 2000 ms |
| Hard | 2000 ms | 3000 ms |
| Insane | 1000 ms | 4000 ms |

---

## How the Phase Cycle Works (Technical)

```
Time ─────────────────────────────────────────────────►

│◄── GREEN ──►│◄─── RED ───►│◄── GREEN ──►│◄─── RED ──►│
│  3000 ms    │  2000 ms     │  3000 ms    │  2000 ms    │
│ May move    │ Must freeze  │ May move    │ Must freeze  │
│ Green LED ON│ Red LED ON   │ Green LED ON│ Red LED ON   │
│ Matrix: "G" │ Matrix: "R"  │ Matrix: "G" │ Matrix: "R"  │
│ Strip: green│ Strip: red   │ Strip: green│ Strip: red   │
│   pulse     │   solid      │   pulse     │   solid      │
```

The phase timer is reset when the game transitions to PLAYING (after countdown). It cycles indefinitely until the game ends (fail or win).
