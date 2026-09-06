/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/buttons.cpp
 *
 *  Debounced button input.
 *
 *  Debounce strategy (classic, simple, works for tactile switches):
 *    1. Read the raw pin.
 *    2. If raw != lastStable, wait AURORA_BTN_DEBOUNCE_MS and re-read.
 *    3. If the new reading still differs, accept the change.
 *    4. On a 0->1 transition (release), call it a "press" event.
 */

#include "buttons.h"

void AuroraButtons::begin() {
    pinMode(_touch.pin, INPUT_PULLUP);
    _touch.lastRaw = digitalRead(_touch.pin);
    _touch.lastStable = _touch.lastRaw;
    _touch.lastChangeMs = millis();

    DBG_PRINTLN(F("[BTN] begin() OK — Single Touch Button on GPIO0"));
}

bool AuroraButtons::checkBtn(BtnState& b) {
    // Helper that updates button's debounce state and returns true if
    // a fresh press event (0->1 release transition) was detected.
    bool raw = (digitalRead(b.pin) == LOW);  // LOW = pressed (active-low)
    bool pressed = false;

    if (raw != b.lastRaw) {
        b.lastChangeMs = millis();
        b.lastRaw = raw;
    }

    if ((millis() - b.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (raw != b.lastStable) {
            // State changed after debounce window — emit a press on 0->1 (release).
            if (b.lastStable == false && raw == true) {
                pressed = true;
            }
            b.lastStable = raw;
        }
    }
    return pressed;
}

uint8_t AuroraButtons::update() {
    uint8_t events = BTN_NONE;
    if (checkBtn(_touch)) events |= BTN_TOUCH;
    return events;
}
