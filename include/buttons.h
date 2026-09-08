/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/buttons.h
 *
 *  Button input with software debouncing.
 *
 *  Block 1 scope:
 *    - GPIO0  -> BTN_TOUCH       (Warm Touch on the deskmate)
 *    - GPIO2  -> BTN_MODE_CYCLE  (short press: cycle OLED screen modes)
 *             -> BTN_WIFI_TOGGLE (3-second hold: toggle WiFi softAP)
 *
 *  Design notes:
 *    - Pressing a button pulls the pin to GND (active LOW).
 *    - Debounce uses AURORA_BTN_DEBOUNCE_MS (80 ms) for both pins.
 *    - GPIO2 is dual-purpose: a short press cycles the OLED mode, and a
 *      3-second hold toggles the WiFi AP. The mode cycle only fires on a
 *      clean short release; a held button triggers the WiFi toggle instead.
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

enum ButtonEvent : uint8_t {
    BTN_NONE        = 0b0000,
    BTN_TOUCH       = 0b0001,  // GPIO0 touch button (Warm Touch)
    BTN_MODE_CYCLE  = 0b0010,  // GPIO2 push button short press (cycle OLED mode)
    BTN_WIFI_TOGGLE = 0b0100   // GPIO2 push button long press 3s (toggle WiFi AP)
};

class AuroraButtons {
public:
    /**
     * Initialize GPIO pins and pull-ups. MUST be called in setup().
     */
    void begin();

    /**
     * Poll all buttons and emit fresh press events.
     */
    uint8_t update();

private:
    struct BtnState {
        uint8_t  pin;
        uint8_t  eventOnPress;     // which ButtonEvent to emit
        bool     lastStable;       // last debounced state (true = pressed)
        bool     lastRaw;          // last raw reading
        uint32_t lastChangeMs;     // for debounce timing
        uint32_t pressStartMs;     // when button was first pressed
        bool     longFired;        // whether 3s long press has already triggered
    };

    BtnState _touch { AURORA_BTN_TOUCH_PIN, BTN_TOUCH,      false, false, 0, 0, false };
    BtnState _multi { AURORA_BTN_MULTI_PIN, BTN_MODE_CYCLE, false, false, 0, 0, false };

    uint8_t checkTouch(BtnState& b);
    uint8_t checkMulti(BtnState& b);
};

#endif // AURORA_BUTTONS_H
