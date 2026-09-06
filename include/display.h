/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/display.h
 *
 *  OLED display driver wrapper using U8g2 library.
 *  Supports both SSD1306 (0.96") and SH1106 (1.3") displays.
 */

#ifndef AURORA_DISPLAY_H
#define AURORA_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

// U8g2 forward declaration
class U8G2;

class AuroraDisplay {
public:
    /**
     * Initialize the display hardware. Returns true on success.
     * MUST be called once in setup() before any draw call.
     */
    bool begin();

    /**
     * Draw the boot screen with project name + build block.
     */
    void showBootScreen();

    /**
     * Draw a 3-line status screen. Used for Block 1's main view.
     *   line1 / line2 / line3 — null-terminated C strings
     */
    void showStatusScreen(const char* line1, const char* line2, const char* line3);

    /**
     * Draw a centered single line of text at the given y position.
     */
    void showCenteredText(const char* text, int16_t y, uint8_t textSize = 1);

    /**
     * Show a centered popup for `durationMs` milliseconds.
     * While the popup is active, all draw calls are ignored — the popup
     * wins. This makes the popup feel like a system notification.
     */
    void popup(const char* text, uint32_t durationMs);

    /**
     * Returns true if a popup is currently being shown.
     */
    bool isPopupActive() const;

    /**
     * Force-dismiss the current popup (if any). Useful in tests.
     */
    void dismissPopup();

    /**
     * Clear the framebuffer (does not push to display — call display()).
     */
    void clear();

    /**
     * Push the framebuffer to the display.
     */
    void display();

private:
    bool  _popupActive;
    uint32_t _popupEndsAtMs;
};

#endif // AURORA_DISPLAY_H
