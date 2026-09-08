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
 *  a single, well-defined event for each pin. GPIO2 additionally fires
 *  BTN_WIFI_TOGGLE on a 3 second hold (guarded by `longFired` so it only
 *  triggers once per press).
 */

#include "buttons.h"

void AuroraButtons::begin() {
    pinMode(_touch.pin, INPUT_PULLUP);
    _touch.lastRaw    = (digitalRead(_touch.pin) == LOW);
    _touch.lastStable = _touch.lastRaw;
    _touch.lastChangeMs = millis();
    _touch.pressStartMs = 0;
    _touch.longFired = false;

    pinMode(_multi.pin, INPUT_PULLUP);
    _multi.lastRaw    = (digitalRead(_multi.pin) == LOW);
    _multi.lastStable = _multi.lastRaw;
    _multi.lastChangeMs = millis();
    _multi.pressStartMs = 0;
    _multi.longFired = false;

    DBG_PRINTF("[BTN] begin() OK — Touch on GPIO%d (BTN_TOUCH), Multi on GPIO%d (short=MODE_CYCLE, 3s=WIFI_TOGGLE), debounce %ums\n",
               AURORA_BTN_TOUCH_PIN, AURORA_BTN_MULTI_PIN,
               (unsigned)AURORA_BTN_DEBOUNCE_MS);
}

uint8_t AuroraButtons::checkTouch(BtnState& b) {
    bool raw = (digitalRead(b.pin) == LOW);  // LOW = pressed (active-low)
    uint8_t event = BTN_NONE;
    uint32_t now = millis();

    if (raw != b.lastRaw) {
        b.lastChangeMs = now;
        b.lastRaw = raw;
    }

    if ((now - b.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (raw != b.lastStable) {
            // Emit on press edge (unpressed -> pressed)
            if (!b.lastStable && raw) {
                event = b.eventOnPress;
            }
            b.lastStable = raw;
        }
    }
    return event;
}

uint8_t AuroraButtons::checkMulti(BtnState& b) {
    bool raw = (digitalRead(b.pin) == LOW);  // LOW = pressed (active-low)
    uint8_t event = BTN_NONE;
    uint32_t now = millis();

    if (raw != b.lastRaw) {
        b.lastChangeMs = now;
        b.lastRaw = raw;
    }

    if ((now - b.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (raw != b.lastStable) {
            if (!b.lastStable && raw) {
                // Just pressed
                b.pressStartMs = now;
                b.longFired = false;
            } else if (b.lastStable && !raw) {
                // Just released
                if (!b.longFired && (now - b.pressStartMs < 3000)) {
                    // Clean short press
                    event = BTN_MODE_CYCLE;
                }
                b.longFired = false;
            }
            b.lastStable = raw;
        }
    }

    // Check for 3-second long press while button is held down
    if (b.lastStable && !b.longFired) {
        if ((now - b.pressStartMs) >= 3000) {
            b.longFired = true;
            event = BTN_WIFI_TOGGLE;
        }
    }

    return event;
}

uint8_t AuroraButtons::update() {
    uint8_t events = BTN_NONE;
    events |= checkTouch(_touch);
    events |= checkMulti(_multi);
    return events;
}

