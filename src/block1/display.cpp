/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/display.cpp
 */

#include "display.h"

bool AuroraDisplay::begin() {
    // Initialize the I2C bus with our custom pins + fast-mode frequency.
    Wire.begin(AURORA_OLED_SDA_PIN, AURORA_OLED_SCL_PIN, AURORA_OLED_I2C_FREQ);

    _oled = Adafruit_SSD1306(AURORA_OLED_WIDTH, AURORA_OLED_HEIGHT,
                             &Wire, AURORA_OLED_RESET);

    if (!_oled.begin(SSD1306_SWITCHCAPVCC, AURORA_OLED_ADDR)) {
        DBG_PRINTLN(F("[OLED] begin() FAILED — check wiring and I2C address"));
        return false;
    }

    _oled.clearDisplay();
    _oled.display();
    _popupActive = false;
    _popupEndsAtMs = 0;

    DBG_PRINTF("[OLED] begin() OK — %dx%d at 0x%02X, %d kHz\n",
               AURORA_OLED_WIDTH, AURORA_OLED_HEIGHT,
               AURORA_OLED_ADDR, AURORA_OLED_I2C_FREQ / 1000);
    return true;
}

void AuroraDisplay::showBootScreen() {
    _oled.clearDisplay();
    _oled.setTextSize(2);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setCursor(20, 5);
    _oled.print(F("Aurora"));

    _oled.setTextSize(1);
    _oled.setCursor(10, 30);
    _oled.print(AURORA_VERSION);

    _oled.setCursor(10, 44);
    _oled.print(AURORA_BUILD_BLOCK);

    _oled.display();
}

void AuroraDisplay::showStatusScreen(const char* line1, const char* line2, const char* line3) {
    // If a popup is active, don't overwrite it.
    if (_popupActive && millis() < _popupEndsAtMs) return;

    _oled.clearDisplay();
    _oled.setTextSize(1);
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setCursor(0, 0);
    if (line1) _oled.print(line1);
    _oled.setCursor(0, 18);
    if (line2) _oled.print(line2);
    _oled.setCursor(0, 36);
    if (line3) _oled.print(line3);
    _oled.display();
}

void AuroraDisplay::showCenteredText(const char* text, int16_t y, uint8_t textSize) {
    _oled.setTextSize(textSize);
    _oled.setTextColor(SSD1306_WHITE);

    int16_t x1, y1;
    uint16_t w, h;
    _oled.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int16_t x = (AURORA_OLED_WIDTH - (int)w) / 2;
    if (x < 0) x = 0;
    _oled.setCursor(x, y);
    _oled.print(text);
}

void AuroraDisplay::popup(const char* text, uint32_t durationMs) {
    _oled.clearDisplay();

    // Inverted (white text on black) header bar for the popup frame
    _oled.fillRect(0, 0, AURORA_OLED_WIDTH, 12, SSD1306_WHITE);
    _oled.setTextSize(1);
    _oled.setTextColor(SSD1306_BLACK);   // black on the white header
    _oled.setCursor(2, 2);
    _oled.print(F("! Notice"));

    // Main message in the body
    _oled.setTextColor(SSD1306_WHITE);
    _oled.setTextSize(2);

    // Center the text vertically in the body area
    int16_t x1, y1;
    uint16_t w, h;
    _oled.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (AURORA_OLED_WIDTH - (int)w) / 2;
    int16_t y = 24;
    if (x < 0) x = 0;
    _oled.setCursor(x, y);
    _oled.print(text);

    _oled.display();

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
    _oled.clearDisplay();
}

void AuroraDisplay::display() {
    _oled.display();
}
