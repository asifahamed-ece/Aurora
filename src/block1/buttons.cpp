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
    auto setupBtn = [](BtnState& b) {
        pinMode(b.pin, INPUT_PULLUP);
        b.lastRaw = digitalRead(b.pin);
        b.lastStable = b.lastRaw;
        b.lastChangeMs = millis();
    };
    setupBtn(_up);
    setupBtn(_select);
    setupBtn(_down);
    setupBtn(_wifi);

    DBG_PRINTLN(F("[BTN] begin() OK — Up=GPIO0, Sel=GPIO1, Down=GPIO10, WiFi=GPIO2"));
}

bool AuroraButtons::checkBtn(BtnState& b) {
    // Helper that updates one button's debounce state and returns true if
    // a fresh press event (0->1 release transition) was detected.
    bool raw = (digitalRead(b.pin) == LOW);  // LOW = pressed (active-low)
    bool pressed = false;

    if (raw != b.lastRaw) {
        b.lastChangeMs = millis();
        b.lastRaw = raw;
    }

    if ((millis() - b.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (raw != b.lastStable) {
            // State changed after debounce window — emit a press on 0->1.
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

    if (checkBtn(_up))     events |= BTN_UP;
    if (checkBtn(_select)) events |= BTN_SELECT;
    if (checkBtn(_down))   events |= BTN_DOWN;
    if (checkBtn(_wifi))   events |= BTN_WIFI;

    return events;
}
