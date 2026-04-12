#include "sensors.h"
#include "config.h"

// ── Internal State ──────────────────────────────────────────────────────────
static int irBaseline = 0;

static unsigned long lastWireDebounce   = 0;
static unsigned long lastStartDebounce  = 0;
static unsigned long lastFinishDebounce = 0;

static bool lastWireState   = false;
static bool lastStartState  = false;
static bool lastFinishState = false;

// ── Debounced Digital Read Helper ───────────────────────────────────────────
static bool debouncedRead(uint8_t pin, unsigned long &lastDebounce, bool &lastState) {
    bool raw = (digitalRead(pin) == LOW);   // INPUT_PULLUP: LOW = contact
    unsigned long now = millis();

    if (raw != lastState) {
        lastDebounce = now;
    }
    lastState = raw;

    if ((now - lastDebounce) >= DEBOUNCE_MS) {
        return raw;
    }
    return false;
}

// ── Setup ───────────────────────────────────────────────────────────────────
void sensorsSetup() {
    pinMode(WIRE_PIN,   INPUT_PULLUP);
    pinMode(START_PIN,  INPUT_PULLUP);
    pinMode(FINISH_PIN, INPUT_PULLUP);
    pinMode(PIR_PIN,    INPUT);
    // A0 needs no pinMode on ESP8266
}

// ── Contact Detection ───────────────────────────────────────────────────────
bool isWireTouched() {
    return debouncedRead(WIRE_PIN, lastWireDebounce, lastWireState);
}

bool isStartTouched() {
    return debouncedRead(START_PIN, lastStartDebounce, lastStartState);
}

bool isFinishTouched() {
    return debouncedRead(FINISH_PIN, lastFinishDebounce, lastFinishState);
}

// ── IR Sensor ───────────────────────────────────────────────────────────────
void irCalibrate() {
    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(IR_PIN);
        delay(2);
    }
    irBaseline = (int)(sum / 32);
    Serial.print(F("[IR] Calibrated baseline: "));
    Serial.println(irBaseline);
}

bool irIsMoving() {
    int current = analogRead(IR_PIN);
    return abs(current - irBaseline) > IR_MOVE_THRESHOLD;
}

// ── PIR Sensor ──────────────────────────────────────────────────────────────
bool pirIsMoving() {
    return digitalRead(PIR_PIN) == HIGH;
}
