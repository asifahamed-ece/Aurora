/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/buttons.cpp
 *
 *  Button and Power Switch input handlers:
 *    - Button 0 (GPIO0): Warm Touch & OLED Mode Cycle (Clock, Face, Thought, Pulse)
 *    - Button 2 (GPIO2): WiFi AP Toggle Switch (simple press toggles WiFi AP)
 *    - Switch (GPIO10): Power Toggle Switch (toggled OFF -> Goodnight & Sleep with RTC)
 */

#include "buttons.h"

void AuroraButtons::begin() {
    pinMode(_touch.pin, INPUT_PULLUP);
    _touch.lastRaw = digitalRead(_touch.pin);
    _touch.lastStable = _touch.lastRaw;
    _touch.lastChangeMs = millis();

    pinMode(_wifi.pin, INPUT_PULLUP);
    _wifi.lastRaw = digitalRead(_wifi.pin);
    _wifi.lastStable = _wifi.lastRaw;
    _wifi.lastChangeMs = millis();

#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        pinMode(AURORA_SLEEP_SWITCH_PIN, INPUT_PULLUP);
        _switchLastRaw = digitalRead(AURORA_SLEEP_SWITCH_PIN);
        _switchLastStable = _switchLastRaw;
        _switchChangeMs = millis();
    }
#endif

    DBG_PRINTLN(F("[BTN] begin() OK — GPIO0: Touch & Screen Cycle, GPIO2: WiFi Toggle, GPIO10: Power Switch"));
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
    // 1. Button 0 (GPIO0): Dedicated Warm Touch & Screen Mode Cycle
    // ------------------------------------------------------------------------
    bool touchRaw = (digitalRead(_touch.pin) == LOW); // LOW = pressed
    if (touchRaw != _touch.lastRaw) {
        _touch.lastChangeMs = now;
        _touch.lastRaw = touchRaw;
    }
    if ((now - _touch.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (touchRaw != _touch.lastStable) {
            if (!_touch.lastStable && touchRaw) {
                // Fresh press on Button 0: triggers warm touch & mode cycle
                events |= BTN_TOUCH;
            }
            _touch.lastStable = touchRaw;
        }
    }

    // ------------------------------------------------------------------------
    // 2. Button 2 (GPIO2): Dedicated WiFi AP Toggle Switch (Classic simple press)
    // ------------------------------------------------------------------------
    bool wifiRaw = (digitalRead(_wifi.pin) == LOW); // LOW = pressed
    if (wifiRaw != _wifi.lastRaw) {
        _wifi.lastChangeMs = now;
        _wifi.lastRaw = wifiRaw;
    }
    if ((now - _wifi.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (wifiRaw != _wifi.lastStable) {
            if (!_wifi.lastStable && wifiRaw) {
                // Fresh press on Button 2: toggles WiFi SoftAP
                events |= BTN_WIFI_TOGGLE;
            }
            _wifi.lastStable = wifiRaw;
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
