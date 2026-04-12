# Hardware Guide

This document lists every component needed to build the ESP Buzzwire Game, along with power requirements and sourcing notes.

---

## Component List

| # | Component | Qty | Purpose | Example Part / Link |
|---|-----------|-----|---------|---------------------|
| 1 | **ESP8266 Dev Board** (Wemos D1 Mini or NodeMCU v2) | 1 | Main controller | [Wemos D1 Mini](https://www.wemos.cc/en/latest/d1/d1_mini.html) / any NodeMCU v2 clone |
| 2 | **WS2812B LED Strip** (60 LEDs/m, 5 V) | 1 strip (30–60 LEDs) | Light effects along the wire path | AliExpress / Amazon — search "WS2812B 60/m IP30" |
| 3 | **8×8 WS2812B LED Matrix** | 1 | Countdown, messages, score, Pro Mode indicator | Search "8x8 WS2812B matrix" — 64 RGB LEDs, single data line |
| 4 | **Active Buzzer Module** (3.3–5 V) | 1 | Fail / win sound feedback | KY-012 or any 5 V active buzzer |
| 5 | **Copper / Brass Wire** (2–3 mm diameter) | ~1 m | The game wire path | Hardware store or craft wire |
| 6 | **Metal Ring** (~15–20 mm inner Ø) | 1 | Player's wand ring | Brass ring, washer, or 3D-printed holder with metal insert |
| 7 | **Wooden / 3D-Printed Handle** | 1 | Grip for the ring + wire to GND | Any comfortable grip — route a flexible wire through it |
| 8 | **Flexible Silicone Wire** (22–26 AWG) | ~1 m | Connects ring (GND) through handle to ESP GND | Silicone-insulated hook-up wire |
| 9 | **Start Pad** (bare copper plate / bolt head) | 1 | Player touches to start game | Copper tape on wood, or an M6 bolt head flush-mounted |
| 10 | **Finish Pad** (bare copper plate / bolt head) | 1 | Player touches to win | Same as start pad |
| 11 | **IR Proximity Sensor** (analog, e.g., TCRT5000 or Sharp GP2Y0A21) | 1 | Pro Mode — movement detection via distance delta | TCRT5000 module (~$1) or Sharp GP2Y0A21 |
| 12 | **PIR Motion Sensor** (e.g., HC-SR501) | 1 | Pro Mode — gross body/hand movement detection | HC-SR501 (~$1) |
| 13 | **Red 5 mm LED** + 330 Ω resistor | 1 set | Pro Mode traffic light — RED | Standard through-hole LED |
| 14 | **Green 5 mm LED** + 330 Ω resistor | 1 set | Pro Mode traffic light — GREEN | Standard through-hole LED |
| 15 | **5 V 2 A+ Power Supply** | 1 | Powers LEDs, matrix, ESP8266 | Any 5 V / 2–3 A barrel-jack or USB PSU |
| 16 | **100 µF Electrolytic Capacitor** | 1 | Decoupling across LED strip power input | 16 V or higher rated |
| 17 | **10 kΩ Resistors** (optional) | 2–3 | External pull-ups or voltage dividers as needed | 1/4 W through-hole |
| 18 | **Breadboard + Jumper Wires** | 1 set | Prototyping | Standard 830-point breadboard |
| 19 | **Wooden / Acrylic Base** | 1 | Mounting frame for wire path, LEDs, electronics | ~30×15 cm, 10 mm thick |

**Estimated total cost:** ~$15–30 (excluding base frame)

---

## Power Requirements

| Component | Max Current (est.) |
|---|---|
| ESP8266 | ~80 mA |
| WS2812B Strip (30 LEDs, full white) | ~450 mA |
| WS2812B Matrix (64 LEDs, full white) | ~960 mA |
| Active Buzzer | ~30 mA |
| PIR + IR sensors | ~20 mA |
| Discrete LEDs (red + green) | ~40 mA |
| **Total (worst case)** | **~1.6 A** |

### PSU Recommendation
- Use a **5 V / 2 A** (or higher) regulated power supply.
- Power the LED strip and matrix **directly** from the PSU 5 V rail — **do NOT draw LED power through the ESP8266 board**.
- Connect the ESP8266 5 V/VIN pin to the same PSU (the on-board regulator will provide 3.3 V to the MCU).
- **All grounds must be common** — ESP GND, LED GND, sensor GND, and PSU GND tied together.

---

## Level Shifting Note

The ESP8266 GPIO outputs 3.3 V logic, but WS2812B LEDs are specified for 5 V logic. In practice:

- **Most WS2812B strips work fine at 3.3 V data** when the data wire is short (< 30 cm) and the first LED is powered at 5 V.
- If you experience flickering or data corruption, add a **logic level shifter** (e.g., 74HCT125, SN74LVC1T45) or a **single-diode trick** (drop VCC of the first LED to ~4.3 V so 3.3 V is above the 0.7×VCC threshold).
- Place a **330 Ω resistor** in series on the data line close to the first LED to reduce ringing.

---

## Where to Source

| Source | Good for |
|---|---|
| AliExpress / Banggood | WS2812B strips, matrices, ESP8266 boards, sensor modules — cheapest |
| Amazon | Faster shipping, slightly higher prices |
| Adafruit / SparkFun | High-quality breakout boards, excellent documentation |
| Local electronics store | Resistors, capacitors, wire, buzzer, LEDs |
| Hardware store | Copper wire (game path), wood base, bolts (start/finish pads) |
