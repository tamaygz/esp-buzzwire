#include "sensors.h"
#include "config.h"

// ── Internal State ──────────────────────────────────────────────────────────
static int  irBaseline = 0;         // Calibrated ADC baseline; 0 = uncalibrated

static unsigned long lastWireDebounce   = 0;
static unsigned long lastStartDebounce  = 0;
static unsigned long lastFinishDebounce = 0;

static bool lastWireRaw   = false;
static bool lastStartRaw  = false;
static bool lastFinishRaw = false;

// ── Debounced Digital Read Helper ───────────────────────────────────────────
// INPUT_PULLUP convention: LOW = contact, HIGH = open.
// Returns true only after the raw reading has been stable for DEBOUNCE_MS.
static bool debouncedRead(uint8_t pin, unsigned long &lastDebounce, bool &lastRaw) {
    bool raw = (digitalRead(pin) == LOW);
    unsigned long now = millis();

    if (raw != lastRaw) {
        // State changed — restart debounce timer
        lastRaw      = raw;
        lastDebounce = now;
    }

    // Confirm stable state only after quiet period
    if ((now - lastDebounce) >= DEBOUNCE_MS) {
        return raw;
    }
    return false;   // Still within debounce window
}

// ── Setup ───────────────────────────────────────────────────────────────────
void sensorsSetup() {
    pinMode(WIRE_PIN,   INPUT_PULLUP);
    pinMode(START_PIN,  INPUT_PULLUP);
    pinMode(FINISH_PIN, INPUT_PULLUP);
    pinMode(PIR_PIN,    INPUT);
    // A0 / IR_PIN: ESP8266 ADC needs no pinMode call

    Serial.println(F("[SENSORS] Setup complete"));
    DEBUG_LOGLN("[SENSORS] WIRE_PIN, START_PIN, FINISH_PIN → INPUT_PULLUP");
    DEBUG_LOGLN("[SENSORS] PIR_PIN → INPUT");
}

// ── Contact Detection ───────────────────────────────────────────────────────
bool isWireTouched() {
    bool v = debouncedRead(WIRE_PIN, lastWireDebounce, lastWireRaw);
    if (v) { DEBUG_LOGLN("[SENSORS] Wire TOUCHED"); }
    return v;
}

bool isStartTouched() {
    bool v = debouncedRead(START_PIN, lastStartDebounce, lastStartRaw);
    if (v) { DEBUG_LOGLN("[SENSORS] Start TOUCHED"); }
    return v;
}

bool isFinishTouched() {
    bool v = debouncedRead(FINISH_PIN, lastFinishDebounce, lastFinishRaw);
    if (v) { DEBUG_LOGLN("[SENSORS] Finish TOUCHED"); }
    return v;
}

// ── IR Sensor ───────────────────────────────────────────────────────────────
// Blocks for approximately IR_CALIBRATE_SAMPLES × 2 ms (~64 ms at default 32).
void irCalibrate() {
    long sum = 0;
    for (int i = 0; i < IR_CALIBRATE_SAMPLES; i++) {
        sum += analogRead(IR_PIN);
        delay(2);
    }
    irBaseline = (int)(sum / IR_CALIBRATE_SAMPLES);
    Serial.print(F("[IR] Calibrated baseline: "));
    Serial.println(irBaseline);
}

bool irIsMoving() {
    int current = analogRead(IR_PIN);
    int delta   = abs(current - irBaseline);
    bool moving = (delta > IR_MOVE_THRESHOLD);
    DEBUG_LOG_2("[IR] val=", current, " delta=", delta);
    return moving;
}

// Returns the last calibrated ADC baseline (0 if irCalibrate() has not been called).
int irGetBaseline() {
    return irBaseline;
}

// ── PIR Sensor ──────────────────────────────────────────────────────────────
bool pirIsMoving() {
    bool moving = (digitalRead(PIR_PIN) == HIGH);
    if (moving) { DEBUG_LOGLN("[PIR] Motion detected"); }
    return moving;
}
