/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/buttons.h
 *
 *  Button input with software debouncing.
 *
 *  Block 1 scope:
 *    - 4 buttons (Up, Select, Down, WiFi) with internal pull-ups
 *    - Edge detection (only fires on press, not release or hold)
 *    - Non-blocking: call update() in loop()
 *
 *  Design notes:
 *    - Pressing a button pulls the pin to GND (active LOW).
 *    - Debounce uses a 50ms lockout after a press is registered.
 *    - Long-press detection is NOT in Block 1 (added in Block 3 for
 *      the WiFi setup long-press). The WiFi button is a simple press
 *      that triggers an "AP activation" event.
 *
 *  Press detection logic:
 *    We fire on the 0->1 transition (release), not the 1->0 (press).
 *    Rationale: tactile button mechanical bounce is worse on the press
 *    edge than the release edge, and the user perception of "I pressed
 *    a button" maps well to the release event after a clean press.
 */

#ifndef AURORA_BUTTONS_H
#define AURORA_BUTTONS_H

#include <Arduino.h>
#include "config.h"

// Event types emitted by the button manager.
enum ButtonEvent : uint8_t {
    BTN_NONE        = 0b0000,
    BTN_TOUCH       = 0b0001,  // Button 1 (GPIO0): Warm Touch sensor
    BTN_MODE_CYCLE  = 0b0010,  // Button 2 (GPIO2): Short press (< 3s) -> cycle display mode
    BTN_WIFI_TOGGLE = 0b0100,  // Button 2 (GPIO2): Long press (>= 3s) -> toggle WiFi AP
    BTN_STANDBY     = 0b1000   // Button 2 (GPIO2) hold >= 5s OR slide switch turned OFF
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

private:
    struct BtnState {
        uint8_t  pin;
        bool     lastStable;        // last debounced state (true = pressed)
        bool     lastRaw;           // last raw reading
        uint32_t lastChangeMs;      // for debounce timing
        uint32_t pressStartMs;      // when press began
        bool     wifiTriggered;     // whether 3s WiFi toggle fired
        bool     standbyTriggered;  // whether 5s standby fired
    };

    BtnState _touch { AURORA_BTN_TOUCH_PIN, false, false, 0, 0, false, false };
    BtnState _multi { AURORA_BTN_MULTI_PIN, false, false, 0, 0, false, false };
    
    // Optional slide switch on GPIO10
    bool     _switchLastStable { false };
    bool     _switchLastRaw { false };
    uint32_t _switchChangeMs { 0 };
};

#endif // AURORA_BUTTONS_H
