/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/buttons.cpp
 *
 *  Dual-button handler with software debouncing & multi-tier hold timing:
 *    - Button 1 (GPIO0): Warm Touch sensor (fires BTN_TOUCH)
 *    - Button 2 (GPIO2): Multi-function push button
 *        - Short press (< 3s): BTN_MODE_CYCLE (cycles display modes)
 *        - Long press (>= 3s): BTN_WIFI_TOGGLE (toggles WiFi SoftAP)
 *        - Very long press (>= 5s): BTN_STANDBY (enters Deep Sleep standby)
 *    - Hardware Switch (GPIO10): Slide OFF (LOW) -> BTN_STANDBY
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

    DBG_PRINTLN(F("[BTN] begin() OK — GPIO0: Touch, GPIO2: Multi/Hold/Standby, GPIO10: Sleep Switch"));
}

uint8_t AuroraButtons::update() {
    uint8_t events = BTN_NONE;
    uint32_t now = millis();

    // ------------------------------------------------------------------------
    // 1. Button 1 (GPIO0): Dedicated Warm Touch Sensor
    // ------------------------------------------------------------------------
    bool touchRaw = (digitalRead(_touch.pin) == LOW); // LOW = pressed
    if (touchRaw != _touch.lastRaw) {
        _touch.lastChangeMs = now;
        _touch.lastRaw = touchRaw;
    }
    if ((now - _touch.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (touchRaw != _touch.lastStable) {
            if (!_touch.lastStable && touchRaw) {
                // Press edge detected
                events |= BTN_TOUCH;
            }
            _touch.lastStable = touchRaw;
        }
    }

    // ------------------------------------------------------------------------
    // 2. Button 2 (GPIO2): Multi-Function Button (Mode / WiFi / Standby)
    // ------------------------------------------------------------------------
    bool multiRaw = (digitalRead(_multi.pin) == LOW); // LOW = pressed
    if (multiRaw != _multi.lastRaw) {
        _multi.lastChangeMs = now;
        _multi.lastRaw = multiRaw;
    }
    if ((now - _multi.lastChangeMs) >= (uint32_t)AURORA_BTN_DEBOUNCE_MS) {
        if (multiRaw != _multi.lastStable) {
            if (!_multi.lastStable && multiRaw) {
                // Fresh press started
                _multi.pressStartMs = now;
                _multi.wifiTriggered = false;
                _multi.standbyTriggered = false;
            } else if (_multi.lastStable && !multiRaw) {
                // Released! If neither long-press hold fired, it was a short press
                if (!_multi.wifiTriggered && !_multi.standbyTriggered) {
                    events |= BTN_MODE_CYCLE;
                }
            }
            _multi.lastStable = multiRaw;
        }
    }

    // While holding Button 2 down, check hold duration thresholds
    if (_multi.lastStable) {
        uint32_t holdTime = now - _multi.pressStartMs;
        if (holdTime >= (uint32_t)AURORA_BTN_STANDBY_HOLD_MS && !_multi.standbyTriggered) {
            _multi.standbyTriggered = true;
            events |= BTN_STANDBY; // 5 seconds hold -> Deep Sleep
        } else if (holdTime >= (uint32_t)AURORA_BTN_WIFI_HOLD_MS && !_multi.wifiTriggered && !_multi.standbyTriggered) {
            _multi.wifiTriggered = true;
            events |= BTN_WIFI_TOGGLE; // 3 seconds hold -> WiFi Toggle
        }
    }

    // ------------------------------------------------------------------------
    // 3. Optional Hardware Slide Switch (GPIO10)
    // ------------------------------------------------------------------------
#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        bool swRaw = (digitalRead(AURORA_SLEEP_SWITCH_PIN) == LOW); // LOW = switch turned OFF to GND
        if (swRaw != _switchLastRaw) {
            _switchChangeMs = now;
            _switchLastRaw = swRaw;
        }
        if ((now - _switchChangeMs) >= 100) { // 100ms stable filter
            if (swRaw != _switchLastStable) {
                if (!_switchLastStable && swRaw) {
                    // Switch flipped to OFF position
                    events |= BTN_STANDBY;
                }
                _switchLastStable = swRaw;
            }
        }
    }
#endif

    return events;
}
