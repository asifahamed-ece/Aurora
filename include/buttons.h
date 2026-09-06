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
// Bit flags so the caller can check multiple events in one call if needed.
enum ButtonEvent : uint8_t {
    BTN_NONE   = 0b0000,
    BTN_UP     = 0b0001,
    BTN_SELECT = 0b0010,
    BTN_DOWN   = 0b0100,
    BTN_WIFI   = 0b1000
};

class AuroraButtons {
public:
    /**
     * Initialize GPIO pins and pull-ups. MUST be called in setup().
     */
    void begin();

    /**
     * Poll each button and emit an event on a fresh press (after debounce).
     * Call this every loop(). Returns a bitmask of ButtonEvent values.
     * If multiple buttons were pressed since the last call, multiple bits
     * will be set.
     *
     * Note: at typical loop speeds, this is non-blocking.
     */
    uint8_t update();

private:
    // Per-button state for debounce.
    struct BtnState {
        uint8_t  pin;
        bool     lastStable;        // last debounced state (true = pressed)
        bool     lastRaw;           // last raw reading
        uint32_t lastChangeMs;      // for debounce timing
    };

    BtnState _up     { AURORA_BTN_UP_PIN,     false, false, 0 };
    BtnState _select { AURORA_BTN_SELECT_PIN, false, false, 0 };
    BtnState _down   { AURORA_BTN_DOWN_PIN,   false, false, 0 };
    BtnState _wifi   { AURORA_BTN_WIFI_PIN,   false, false, 0 };

    // Helper that updates one button's debounce state and returns true if
    // a fresh press event was detected.
    bool checkBtn(BtnState& b);
};

#endif // AURORA_BUTTONS_H
