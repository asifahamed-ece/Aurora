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
    BTN_NONE   = 0b0000,
    BTN_TOUCH  = 0b0001   // Physical touch button
};

class AuroraButtons {
public:
    /**
     * Initialize GPIO pin and pull-up. MUST be called in setup().
     */
    void begin();

    /**
     * Poll touch button and emit BTN_TOUCH on fresh press/release.
     */
    uint8_t update();

private:
    struct BtnState {
        uint8_t  pin;
        bool     lastStable;        // last debounced state (true = pressed)
        bool     lastRaw;           // last raw reading
        uint32_t lastChangeMs;      // for debounce timing
    };

    BtnState _touch { AURORA_BTN_TOUCH_PIN, false, false, 0 };

    bool checkBtn(BtnState& b);
};

#endif // AURORA_BUTTONS_H
