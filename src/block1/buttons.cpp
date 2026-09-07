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
 *    4. On a 0->1 transition (release), emit the per-pin ButtonEvent.
 *
 *  Each BtnState carries its own `eventOnPress` (BTN_TOUCH for GPIO0,
 *  BTN_MODE_CYCLE for GPIO2), so the same release-edge logic produces
 *  a single, well-defined event for each pin. There is intentionally
 *  no long-press / WiFi-toggle path on GPIO2 anymore.
 */

#include "buttons.h"

void AuroraButtons::begin() {
    pinMode(_touch.pin, INPUT_PULLUP);
    _touch.lastRaw    = digitalRead(_touch.pin);
    _touch.lastStable = _touch.lastRaw;
    _touch.lastChangeMs = millis();

    pinMode(_multi.pin, INPUT_PULLUP);
    _multi.lastRaw    = digitalRead(_multi.pin);
    _multi.lastStable = _multi.lastRaw;
    _multi.lastChangeMs = millis();

    DBG_PRINTF("[BTN] begin() OK — Touch on GPIO%d (BTN_TOUCH), Multi on GPIO%d (BTN_MODE_CYCLE), debounce %ums\n",
               AURORA_BTN_TOUCH_PIN, AURORA_BTN_MULTI_PIN,
               (unsigned)AURORA_BTN_DEBOUNCE_MS);
}

uint8_t AuroraButtons::checkBtn(BtnState& b) {
    // Helper that updates a button's debounce state and returns the
    // ButtonEvent for a fresh press, or BTN_NONE.
    bool raw = (digitalRead(b.pin) == LOW);  // LOW = pressed (active-low)
    uint8_t event = BTN_NONE;

    if (raw != b.lastRaw) {
        b.lastChangeMs = millis();
        b.lastRaw = raw;
    }

    if ((millis() - b.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (raw != b.lastStable) {
            // State changed after debounce window — emit on 0->1 (release).
            if (b.lastStable == false && raw == true) {
                event = b.eventOnPress;
            }
            b.lastStable = raw;
        }
    }
    return event;
}

uint8_t AuroraButtons::update() {
    uint8_t events = BTN_NONE;
    events |= checkBtn(_touch);
    events |= checkBtn(_multi);
    return events;
}
