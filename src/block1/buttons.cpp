/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/buttons.cpp
 *
 *  Button and Power Switch input handlers:
 *    - Button 0 (GPIO0): Dedicated Warm Touch sensor (single tap = warm touch only)
 *    - Button 2 (GPIO2): Mode cycle (< 3s) & WiFi SoftAP toggle (>= 3s)
 *    - Switch (GPIO10): Hardware Power Switch (toggled OFF -> Goodnight & Sleep with RTC)
 */

#include "buttons.h"

void AuroraButtons::begin() {
    pinMode(_touch.pin, INPUT_PULLUP);
    _touch.lastRaw = digitalRead(_touch.pin);
    _touch.lastStable = _touch.lastRaw;
    _touch.lastChangeMs = millis();

    pinMode(_multi.pin, INPUT_PULLUP);
    _multi.lastRaw = digitalRead(_multi.pin);
    _multi.lastStable = _multi.lastRaw;
    _multi.lastChangeMs = millis();

#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        pinMode(AURORA_SLEEP_SWITCH_PIN, INPUT_PULLUP);
        _switchLastRaw = digitalRead(AURORA_SLEEP_SWITCH_PIN);
        _switchLastStable = _switchLastRaw;
        _switchChangeMs = millis();
    }
#endif

    DBG_PRINTLN(F("[BTN] begin() OK — GPIO0: Warm Touch ONLY, GPIO2: Mode (<3s) & WiFi (>=3s), GPIO10: Power Switch"));
}

bool AuroraButtons::isPowerSwitchOff() const {
#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        return (digitalRead(AURORA_SLEEP_SWITCH_PIN) == LOW); // LOW = switched OFF to GND
    }
#endif
    return false;
}

uint8_t AuroraButtons::update() {
    uint8_t events = BTN_NONE;
    uint32_t now = millis();

    // ------------------------------------------------------------------------
    // 1. Button 0 (GPIO0): Dedicated Warm Touch Sensor (SINGLE TAP = WARM TOUCH ONLY)
    // ------------------------------------------------------------------------
    bool touchRaw = (digitalRead(_touch.pin) == LOW); // LOW = pressed
    if (touchRaw != _touch.lastRaw) {
        _touch.lastChangeMs = now;
        _touch.lastRaw = touchRaw;
    }
    if ((now - _touch.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (touchRaw != _touch.lastStable) {
            if (!_touch.lastStable && touchRaw) {
                // Single tapping on Button 0: records Warm Touch ONLY
                events |= BTN_TOUCH;
            }
            _touch.lastStable = touchRaw;
        }
    }

    // ------------------------------------------------------------------------
    // 2. Button 2 (GPIO2): Mode Cycle (< 3s) & WiFi SoftAP Toggle (>= 3s)
    // ------------------------------------------------------------------------
    bool multiRaw = (digitalRead(_multi.pin) == LOW); // LOW = pressed
    if (multiRaw != _multi.lastRaw) {
        _multi.lastChangeMs = now;
        _multi.lastRaw = multiRaw;
    }
    if ((now - _multi.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (multiRaw != _multi.lastStable) {
            if (!_multi.lastStable && multiRaw) {
                // Press started
                _multi.pressStartMs = now;
                _multi.longPressTriggered = false;
            } else if (_multi.lastStable && !multiRaw) {
                // Released! If held less than 3s, cycle display modes
                if (!_multi.longPressTriggered) {
                    events |= BTN_MODE_CYCLE;
                }
            }
            _multi.lastStable = multiRaw;
        }
    }

    // Check while Button 2 is held down
    if (_multi.lastStable && !_multi.longPressTriggered) {
        if ((now - _multi.pressStartMs) >= (uint32_t)AURORA_BTN_WIFI_HOLD_MS) {
            _multi.longPressTriggered = true;
            events |= BTN_WIFI_TOGGLE; // Held >= 3 seconds: toggle WiFi SoftAP
        }
    }

    // ------------------------------------------------------------------------
    // 3. Hardware Power Toggle Switch (GPIO10)
    // ------------------------------------------------------------------------
#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        bool swRaw = (digitalRead(AURORA_SLEEP_SWITCH_PIN) == LOW); // LOW = switched OFF
        if (swRaw != _switchLastRaw) {
            _switchChangeMs = now;
            _switchLastRaw = swRaw;
        }
        if ((now - _switchChangeMs) >= 80) { // 80ms debounce filter
            if (swRaw != _switchLastStable) {
                if (!_switchLastStable && swRaw) {
                    // Switch flipped to OFF position!
                    events |= BTN_STANDBY;
                }
                _switchLastStable = swRaw;
            }
        }
    }
#endif

    return events;
}
