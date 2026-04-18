# Wiring Guide

Complete wiring reference for the ESP Buzzwire Game. Read [docs/hardware.md](hardware.md) first for the component list.

---

## Pin-by-Pin Connection Table

| ESP8266 Pin | GPIO | Function | Connection |
|-------------|------|----------|------------|
| D4 | GPIO2 | LED Strip Data | → 330 Ω → WS2812B strip DIN |
| D3 | GPIO0 | LED Matrix Data | → 330 Ω → 8×8 matrix DIN |
| D2 | GPIO4 | Buzzwire Contact | → Game wire (other end of wire to GND via ring) |
| D1 | GPIO5 | Start Pad | → Metal start pad (pad also wired to GND) |
| D6 | GPIO12 | Finish Pad | → Metal finish pad (pad also wired to GND) |
| D5 | GPIO14 | Red LED (Pro Mode) | → 330 Ω → Red LED anode → GND |
| D7 | GPIO13 | PIR Sensor | → PIR module digital OUT |
| A0 | A0 | IR Sensor | → IR sensor analog OUT |
| D8 | GPIO15 | Buzzer | → Active buzzer (+) terminal |
| D0 | GPIO16 | Green LED (Pro Mode) | → 330 Ω → Green LED anode → GND |
| 5V / VIN | — | Power In | → PSU 5 V + |
| GND | — | Common Ground | → PSU GND, all component GNDs |

---

## ASCII Wiring Diagram

```
                         ┌──────────────────────────┐
                         │   ESP8266 (D1 Mini)       │
                         │                           │
   5V PSU (+) ──────────►│ 5V                   GND  │◄─── PSU GND (common)
                         │                           │
   WS2812B Strip DIN ◄──│ D4 (GPIO2)                │
   (via 330Ω resistor)  │                           │
                         │                           │
   8x8 Matrix DIN ◄─────│ D3 (GPIO0)                │
   (via 330Ω resistor)  │                           │
                         │                           │
   Game Wire ───────────►│ D2 (GPIO4)  INPUT_PULLUP  │
                         │                           │
   Start Pad ───────────►│ D1 (GPIO5)  INPUT_PULLUP  │
                         │                           │
   Finish Pad ──────────►│ D6 (GPIO12) INPUT_PULLUP  │
                         │                           │
   Buzzer (+) ◄─────────│ D8 (GPIO15)               │
                         │                           │
   PIR Sensor OUT ──────►│ D7 (GPIO13)               │
                         │                           │
   IR Sensor OUT ───────►│ A0                        │
                         │                           │
   Red LED ◄────────────│ D5 (GPIO14) → 330Ω → LED  │
                         │                           │
   Green LED ◄──────────│ D0 (GPIO16) → 330Ω → LED  │
                         └──────────────────────────┘

   Ring/Wand Handle ──── flexible wire ──── GND (common)
```

---

## Game Wire & Ring/Wand Handle

### Ring / Wand

1. Thread a **flexible silicone wire** through the handle interior.
2. One end connects to the **metal ring** (solder or crimp).
3. The other end connects to **ESP GND** (common ground rail).
4. The ring slides along the game wire without an electrical connection — until the player **touches** the game wire, completing the circuit.

### Game Wire

1. Shape 2–3 mm **copper or brass wire** into curves and mount the ends in a wooden or acrylic base.
2. Connect one end of the game wire to **GPIO4 (D2)** on the ESP8266.
3. The firmware configures D2 as `INPUT_PULLUP` — the pin reads HIGH normally.
4. When the grounded ring touches the game wire, the pin is pulled LOW → **contact detected**.

```
    GPIO4 (D2) ─── INPUT_PULLUP (reads HIGH)
         │
    [Game Wire shaped into curves]
         │
    (Ring touches wire → pulls pin LOW)
         │
    [Ring] ──── flexible wire ──── GND
```

---

## Start & Finish Pads

Each pad is a **bare metal surface** connected to **GND**. The corresponding GPIO pin uses `INPUT_PULLUP`.

### Construction Options

- **Bolt head:** Flush-mount an M6 bolt so the head is exposed. Wire the bolt to GND.
- **Copper tape:** Stick copper tape to the base surface. Solder a wire to it and connect to GND.
- **Bare copper plate:** Small PCB pad or copper disc.

### Wiring

```
    GPIO5 (D1)  ─── INPUT_PULLUP
         │
    [Start Pad — bare metal]
         │
    GND (when player touches with finger, skin bridges the gap or
         pad is wired directly to GND; player's body provides path
         through the ring handle wire which also connects to GND)
```

> **Tip:** The simplest approach is to wire the pad directly to GND. The player holds the wand (whose handle wire is GND). When they touch the metal pad with their other hand, their body closes the circuit from GND (pad) → skin → body → skin → GND (handle). However, this may not be reliable. A more robust approach: wire the pad to the GPIO pin and add a separate GND plate next to it, so the player's finger bridges both.

---

## Pro Mode Sensor Wiring

### IR Proximity Sensor (TCRT5000 / Sharp GP2Y0A21)

```
    IR Sensor Module
    ┌─────────────┐
    │ VCC  ───────│──── 3.3V or 5V (check module)
    │ GND  ───────│──── GND
    │ OUT (analog)│──── A0
    └─────────────┘
```

- Mount pointing at the wire path where the player's hand travels.
- The A0 pin reads an analog value (0–1023) proportional to distance.
- At game start, a baseline reading is captured; any significant delta during RED phase = movement.

### PIR Motion Sensor (HC-SR501)

```
    HC-SR501
    ┌─────────────────────┐
    │ VCC ────────────────│──── 5V (HC-SR501 needs 5V!)
    │ GND ────────────────│──── GND
    │ OUT (digital) ──────│──── D7 (GPIO13)
    └─────────────────────┘
    Sensitivity pot: turn CCW to reduce range
    Retrigger jumper: set to "H" (repeatable trigger)
```

- Mount above or beside the wire path, pointing at the player's arm/hand area.
- **Important:** HC-SR501 operates at 5 V but its digital output is typically 3.3 V compatible. Verify with a multimeter.
- Adjust the **sensitivity potentiometer** to cover just the immediate play area.
- Set the **retrigger jumper** to repeatable-trigger mode (**"H" position**) — this keeps the output HIGH as long as motion continues, giving accurate readings during the RED phase. The "L" (single-trigger) mode causes the output to drop briefly even when the player is still moving, leading to missed detections.

---

## Power Distribution

```
                    ┌──────────────┐
    PSU 5V (+) ────►│              │
                    │  Power Rail  ├───► ESP8266 5V/VIN
                    │  (common)    ├───► WS2812B Strip VCC
                    │              ├───► WS2812B Matrix VCC
                    │              ├───► HC-SR501 VCC
                    │              ├───► Buzzer VCC (if 5V type)
                    └──────────────┘
    PSU GND (−) ────► Common GND rail
                       ├── ESP8266 GND
                       ├── Strip GND
                       ├── Matrix GND
                       ├── All sensors GND
                       ├── Buzzer GND
                       ├── LEDs cathode
                       └── Ring/handle wire
```

> **Never** power the LED strip or matrix through the ESP8266's 3.3 V pin — it cannot supply enough current and will cause brownouts or damage.

---

## Decoupling Capacitors

| Capacitor | Location | Purpose |
|---|---|---|
| **100 µF electrolytic** | Across WS2812B strip VCC/GND at the first LED | Absorbs inrush current on power-up |
| **100 µF electrolytic** | Across matrix VCC/GND at the input | Same — protects matrix LEDs |
| **100 nF ceramic** (optional) | Close to ESP8266 VIN/GND | Filters high-frequency noise |

---

## Quick Checklist

- [ ] All GNDs connected together (PSU, ESP, strip, matrix, sensors, ring handle)
- [ ] 330 Ω resistors on LED data lines (D4, D3) close to the first LED
- [ ] 330 Ω resistors on discrete LED anodes (D5, D0)
- [ ] 100 µF capacitors across LED strip and matrix power inputs
- [ ] Game wire connected to D2 (GPIO4)
- [ ] Start pad connected to D1 (GPIO5), pad surface to GND
- [ ] Finish pad connected to D6 (GPIO12), pad surface to GND
- [ ] Ring handle wire connected to GND
- [ ] IR sensor analog out → A0
- [ ] PIR sensor digital out → D7 (GPIO13)
- [ ] Buzzer (+) → D8 (GPIO15), (−) → GND
