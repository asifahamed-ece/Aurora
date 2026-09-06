/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/display.cpp
 *
 *  OLED display driver using U8g2 library.
 *  Supports both SSD1306 (0.96") and SH1106 (1.3") displays.
 */

#include "display.h"

// U8g2 library include
#include <U8g2lib.h>

// Create the U8g2 display object based on driver selection
// For SH1106 1.3" 128x64 I2C:
#if AURORA_OLED_DRIVER == 1
// SH1106 128x64 noname F-HW_I2C constructor
// U8G2_SH1106_128X64_NONAME_F_HW_I2C(u8g2_cb_t rotation, uint8_t reset = U8X8_PIN_NONE)
U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2(U8G2_R0);
#else
// SSD1306 128x64 noname F-HW_I2C constructor
U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2(U8G2_R0);
#endif

bool AuroraDisplay::begin() {
    // Initialize the I2C bus with our custom pins.
    Wire.begin(AURORA_OLED_SDA_PIN, AURORA_OLED_SCL_PIN);

    // Scan for I2C devices and report what we find
    DBG_PRINTLN(F("[OLED] I2C Scanner - looking for devices..."));
    for (byte address = 0; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            DBG_PRINTF("[OLED] I2C device found at 0x%02X\n", address);
        }
    }

    // Initialize the U8g2 display
    _u8g2.setBusClock(400000);  // 400kHz I2C
    _u8g2.begin();

    _popupActive = false;
    _popupEndsAtMs = 0;

    const char* driver = AURORA_OLED_DRIVER == 1 ? "SH1106" : "SSD1306";
    DBG_PRINTF("[OLED] begin() OK — %s %dx%d at 0x%02X\n",
               driver, AURORA_OLED_WIDTH, AURORA_OLED_HEIGHT, AURORA_OLED_ADDR);
    return true;
}

void AuroraDisplay::showBootScreen() {
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_ncenB14_tr);
    _u8g2.setDrawColor(1);

    // Center "Aurora" text
    const char* title = "Aurora";
    int titleWidth = _u8g2.getStrWidth(title);
    int titleX = (AURORA_OLED_WIDTH - titleWidth) / 2;
    _u8g2.drawStr(titleX, 20, title);

    // Version and build block
    _u8g2.setFont(u8g2_font_5x7_tf);
    _u8g2.drawStr(10, 36, AURORA_VERSION);
    _u8g2.drawStr(10, 48, AURORA_BUILD_BLOCK);

    _u8g2.sendBuffer();
}

void AuroraDisplay::showStatusScreen(const char* line1, const char* line2, const char* line3) {
    // If a popup is active, don't overwrite it.
    if (_popupActive && millis() < _popupEndsAtMs) return;

    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_5x7_tf);
    _u8g2.setDrawColor(1);

    if (line1) _u8g2.drawStr(0, 8, line1);
    if (line2) _u8g2.drawStr(0, 24, line2);
    if (line3) _u8g2.drawStr(0, 40, line3);

    _u8g2.sendBuffer();
}

void AuroraDisplay::showCenteredText(const char* text, int16_t y, uint8_t textSize) {
    // Select font based on size
    if (textSize >= 2) {
        _u8g2.setFont(u8g2_font_ncenB14_tr);
    } else {
        _u8g2.setFont(u8g2_font_5x7_tf);
    }

    int textWidth = _u8g2.getStrWidth(text);
    int x = (AURORA_OLED_WIDTH - textWidth) / 2;
    if (x < 0) x = 0;
    _u8g2.drawStr(x, y + 8, text);
}

void AuroraDisplay::popup(const char* text, uint32_t durationMs) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // Header bar (inverted - white background)
    _u8g2.drawBox(0, 0, AURORA_OLED_WIDTH, 12);

    // Header text (black on white)
    _u8g2.setDrawColor(0);  // black
    _u8g2.setFont(u8g2_font_5x7_tf);
    _u8g2.drawStr(2, 9, "! Notice");

    // Main message (white text)
    _u8g2.setDrawColor(1);  // white
    _u8g2.setFont(u8g2_font_ncenB14_tr);
    int textWidth = _u8g2.getStrWidth(text);
    int x = (AURORA_OLED_WIDTH - textWidth) / 2;
    if (x < 0) x = 0;
    _u8g2.drawStr(x, 38, text);

    _u8g2.sendBuffer();

    _popupActive = true;
    _popupEndsAtMs = millis() + durationMs;
}

bool AuroraDisplay::isPopupActive() const {
    if (_popupActive && millis() >= _popupEndsAtMs) {
        // Auto-dismiss
        const_cast<AuroraDisplay*>(this)->_popupActive = false;
        return false;
    }
    return _popupActive;
}

void AuroraDisplay::dismissPopup() {
    _popupActive = false;
    _popupEndsAtMs = 0;
}

void AuroraDisplay::clear() {
    _u8g2.clearBuffer();
}

void AuroraDisplay::display() {
    _u8g2.sendBuffer();
}
