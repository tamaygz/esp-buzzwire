# Troubleshooting

Common issues and how to fix them. Always check the Serial Monitor (115200 baud) first — most problems show obvious symptoms there.

---

## LED Strip Not Lighting Up

### Symptoms
- Strip stays completely dark
- Only the first LED lights up (usually green or white flicker)

### Fixes

| Check | Action |
|---|---|
| **Power** | Verify 5 V PSU is connected to strip VCC/GND. Measure with multimeter. |
| **Data pin** | Confirm the data wire goes to **D4 (GPIO2)**. Check `STRIP_PIN` in `config.h`. |
| **Data direction** | WS2812B strips have a direction — data flows from DIN to DOUT. Check the arrows printed on the strip. |
| **GND** | ESP8266 GND and strip GND must be the same (common ground). |
| **330 Ω resistor** | A resistor on the data line (close to the first LED) prevents signal ringing. Add one if missing. |
| **Number of LEDs** | `STRIP_NUM_LEDS` in `config.h` must match your actual strip length. |
| **Brightness** | If `STRIP_BRIGHTNESS` is 0 or very low, LEDs may appear off. |
| **Dead first LED** | If the first LED is damaged, the entire strip won't work. Try cutting off the first LED and soldering to the second. |

---

## Matrix Showing Garbage / Wrong Layout

### Symptoms
- Characters appear mirrored or scrambled
- Pixels light up in random-looking patterns

### Fixes

| Check | Action |
|---|---|
| **Serpentine flag** | If your matrix is wired in zigzag (odd rows reversed), set `MATRIX_SERPENTINE true`. If all rows go the same direction, set it to `false`. |
| **Data pin** | Confirm data goes to **D3 (GPIO0)**. Check `MATRIX_PIN` in `config.h`. |
| **Matrix size** | Ensure `MATRIX_WIDTH` and `MATRIX_HEIGHT` are both `8` (or match your actual matrix). |
| **Colour order** | The code uses `GRB` colour order (most common for WS2812B). If colours are wrong (e.g., red shows as green), the colour order may be `RGB` — change in `matrix.cpp` where `FastLED.addLeds` is called. |
| **Data direction** | Check that data goes to DIN (input), not DOUT (output) of the matrix. |
| **Power** | Matrix needs sufficient current — 64 LEDs at full white = ~960 mA. Use PSU, not ESP pins. |

### How to Test Serpentine

If you're unsure about your matrix layout, change the firmware temporarily to light up LEDs 0–7 in a single colour. If they form a straight line across the top row, your wiring matches the code. If they zigzag, toggle `MATRIX_SERPENTINE`.

---

## Wire Contact Not Detected / Always Triggered

### Not Detected (wire touch doesn't register)

| Check | Action |
|---|---|
| **Wiring** | Game wire → D2 (GPIO4). Ring/handle → GND via flexible wire. |
| **Continuity** | Use a multimeter to check continuity from the ring to ESP GND, and from the game wire to GPIO4. |
| **Oxidation** | Copper oxidises over time. Sand or clean the wire and ring contact surfaces. |
| **Loose solder** | Re-solder any connections that may have come loose. |
| **Pull-up** | The code uses `INPUT_PULLUP`. Verify D2 reads HIGH when no contact (use Serial to print `digitalRead(WIRE_PIN)`). |

### Always Triggered (game immediately fails on play)

| Check | Action |
|---|---|
| **Short circuit** | The ring may be permanently touching the game wire. Ensure physical separation. |
| **Wire to GND short** | The game wire may be accidentally connected to GND somewhere. Disconnect and test. |
| **Debounce** | If `DEBOUNCE_MS` is too low, noise may register as contact. Increase to 80–100. |
| **Pin floating** | If the pull-up is not working, add an external 10 kΩ pull-up resistor from D2 to 3.3 V. |

---

## PIR Triggering Too Often / Not At All

### Too Often (false positives)

| Check | Action |
|---|---|
| **Sensitivity** | Turn the sensitivity pot **CCW** (counter-clockwise) to reduce range. |
| **Heat sources** | Heaters, sunlight, hot electronics near the sensor cause false triggers. Move or shield them. |
| **Warm-up** | PIR needs 30–60 seconds after power-on to stabilise. Don't start the game immediately. |
| **Detection zone** | Mask part of the Fresnel lens with electrical tape to narrow the field of view. |

### Not Triggering

| Check | Action |
|---|---|
| **Power** | HC-SR501 needs **5 V** (not 3.3 V). Check VCC is connected to 5 V rail. |
| **Line of sight** | Ensure nothing blocks the sensor's view of the player's hand/arm. |
| **Sensitivity** | Turn the sensitivity pot **CW** (clockwise) to increase range. |
| **Time delay** | Turn the time delay pot **CCW** to minimum for fastest re-trigger. |
| **Pin** | Verify output goes to **D7 (GPIO13)**. Check with `Serial.println(digitalRead(PIR_PIN))`. |

---

## IR Sensor Too Sensitive / Not Sensitive Enough

### Too Sensitive (triggers during red when player is still)

| Check | Action |
|---|---|
| **Threshold** | Increase `IR_MOVE_THRESHOLD` in `config.h` (try 200, 300, or higher). |
| **Ambient light** | Fluorescent lights and sunlight can cause ADC fluctuations. Shield the sensor. |
| **Electrical noise** | Add a 100 nF capacitor between A0 and GND. |
| **Baseline drift** | If the baseline is captured when conditions differ from play conditions, recalibrate. |

### Not Sensitive Enough (can move freely without trigger)

| Check | Action |
|---|---|
| **Threshold** | Decrease `IR_MOVE_THRESHOLD` (try 50–80). |
| **Distance** | Move the sensor closer to the hand path (3–5 cm optimal for TCRT5000). |
| **Sensor type** | Some IR sensors have very short range. A Sharp GP2Y0A21 works at 10–80 cm; TCRT5000 works at 1–5 cm. |
| **Pin** | Verify the sensor analog output is connected to **A0**. Test with `Serial.println(analogRead(A0))`. |

---

## Game Stuck in a State

### Stuck in IDLE
- Start pad not detected. Check start pad wiring (D1/GPIO5 to pad, pad to GND).
- Print `digitalRead(START_PIN)` — should be HIGH normally, LOW when touched.

### Stuck in COUNTDOWN
- This should not happen — countdown is time-based. If it occurs, there may be a `millis()` overflow issue (unlikely) or a crash. Check Serial for error messages.

### Stuck in PLAYING
- Finish pad not detected. Check finish pad wiring (D6/GPIO12 to pad, pad to GND).
- Wire contact not detected. Check wire/ring wiring.

### Stuck in FAIL or WIN
- These states have a fixed duration (`FAIL_DISPLAY_MS` / `WIN_DISPLAY_MS`). If the game seems stuck, verify these values aren't set extremely high.
- Check if `millis()` is running (Serial should show ongoing messages).

---

## Serial Monitor Debug Tips

### Enable Verbose Logging

Add temporary print statements in `game.cpp`:

```cpp
// In main.cpp loop(), before gameLoop():
static unsigned long lastDebugPrint = 0;
if (millis() - lastDebugPrint > 1000) {
    Serial.print(F("State: "));
    Serial.print(gameGetState());
    Serial.print(F("  Wire: "));
    Serial.print(digitalRead(WIRE_PIN));
    Serial.print(F("  Start: "));
    Serial.print(digitalRead(START_PIN));
    Serial.print(F("  Finish: "));
    Serial.println(digitalRead(FINISH_PIN));
    lastDebugPrint = millis();
}
```

### Common Serial Messages

| Message | Meaning |
|---|---|
| `=== ESP Buzzwire Game ===` | Boot successful |
| `[INIT] Pro Mode: ENABLED` | Pro Mode is active |
| `[IR] Calibrated baseline: NNN` | IR sensor baseline captured |
| `[GAME] Start touched` | Start pad contact detected |
| `[GAME] FAIL #N` | Wire contact or movement detected |
| `[GAME] WIN! Time: Xs Fails: N` | Player reached finish |
| No output at all | Check baud rate (115200), USB connection, correct COM port |
| Garbled text | Wrong baud rate in Serial Monitor |
