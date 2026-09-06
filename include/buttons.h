/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/buttons.h
 *
 *  Button and Power Switch input with software debouncing:
 *    - Button 0 (GPIO0): Warm Touch & OLED Mode Cycle (Clock, Face, Thought, Pulse)
 *    - Button 2 (GPIO2): WiFi AP Toggle Switch (simple press toggles WiFi AP)
 *    - Switch (GPIO10): Power Toggle Switch (toggled OFF -> Goodnight & Deep Sleep with RTC)
 */

#ifndef AURORA_BUTTONS_H
#define AURORA_BUTTONS_H

#include <Arduino.h>
#include "config.h"

// Event types emitted by the button manager.
enum ButtonEvent : uint8_t {
    BTN_NONE        = 0b0000,
    BTN_TOUCH       = 0b0001,  // Button 0 (GPIO0): Warm Touch & Mode Cycle
    BTN_WIFI_TOGGLE = 0b0010,  // Button 2 (GPIO2): Toggle WiFi SoftAP
    BTN_STANDBY     = 0b0100   // Switch (GPIO10): Toggled OFF -> Sleep with RTC
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
    };

    BtnState _touch { AURORA_BTN_TOUCH_PIN, false, false, 0 };
    BtnState _wifi  { AURORA_BTN_MULTI_PIN, false, false, 0 };
    
    // Hardware Power Toggle Switch on GPIO10
    bool     _switchLastStable { false };
    bool     _switchLastRaw { false };
    uint32_t _switchChangeMs { 0 };
};

#endif // AURORA_BUTTONS_H
