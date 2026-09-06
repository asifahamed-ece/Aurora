/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/buttons.h
 *
 *  Button and Power Switch input with software debouncing:
 *    - Button 0 (GPIO0): Dedicated Warm Touch sensor (single tap = warm touch only)
 *    - Button 2 (GPIO2): Multi-function button:
 *        - Short press (< 3s): Cycles OLED display modes (Clock, Face, Thought, Pulse)
 *        - Long press (>= 3s): Toggles WiFi SoftAP ON / OFF
 *    - Switch (GPIO10): Hardware Power Switch (toggled OFF -> Goodnight & Sleep with RTC)
 */

#ifndef AURORA_BUTTONS_H
#define AURORA_BUTTONS_H

#include <Arduino.h>
#include "config.h"

// Event types emitted by the button manager.
enum ButtonEvent : uint8_t {
    BTN_NONE        = 0b0000,
    BTN_TOUCH       = 0b0001,  // Button 0 (GPIO0): Dedicated Warm Touch ONLY
    BTN_MODE_CYCLE  = 0b0010,  // Button 2 (GPIO2): Short press (< 3s) -> cycle display mode
    BTN_WIFI_TOGGLE = 0b0100,  // Button 2 (GPIO2): Long press (>= 3s) -> toggle WiFi SoftAP
    BTN_STANDBY     = 0b1000   // Switch (GPIO10): Toggled OFF -> Sleep with RTC active
};

class AuroraButtons {
public:
    /**
     * Initialize GPIO pins and pull-ups. MUST be called in setup().
     */
    void begin();

    /**
     * Poll buttons and emit events (debounced).
     */
    uint8_t update();

    /**
     * Check if the hardware power switch is in the OFF state.
     */
    bool isPowerSwitchOff() const;

private:
    struct BtnState {
        uint8_t  pin;
        bool     lastStable;        // last debounced state (true = pressed)
        bool     lastRaw;           // last raw reading
        uint32_t lastChangeMs;      // for debounce timing
        uint32_t pressStartMs;      // when press began
        bool     longPressTriggered;// whether 3s WiFi toggle fired
    };

    BtnState _touch { AURORA_BTN_TOUCH_PIN, false, false, 0, 0, false };
    BtnState _multi { AURORA_BTN_MULTI_PIN, false, false, 0, 0, false };
    
    // Hardware Power Toggle Switch on GPIO10
    bool     _switchLastStable { false };
    bool     _switchLastRaw { false };
    uint32_t _switchChangeMs { 0 };
};

#endif // AURORA_BUTTONS_H
