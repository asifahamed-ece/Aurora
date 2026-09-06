/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/display.cpp
 *
 *  OLED display driver and Animated Deskmate ("Aurora") Character Engine.
 *  Supports both SSD1306 (0.96") and SH1106 (1.3") displays with U8g2.
 */

#include "display.h"
#include "state.h"
#include "clock.h"
#include "messages.h"
#include <U8g2lib.h>
#include <math.h>

// Create the U8g2 display object based on driver selection
#if AURORA_OLED_DRIVER == 1
// SH1106 128x64 noname F-HW_I2C constructor
U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2(U8G2_R0);
#else
// SSD1306 128x64 noname F-HW_I2C constructor
U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2(U8G2_R0);
#endif

// Romantic rotating reactions for Warm Touches (Clean typography, no <3)
static const char* const kTouchReactions[] = {
    "Warm Touch felt!",
    "Aurora loves you!",
    "You're my favorite",
    "Stay cozy, Chandni",
    "So warm and sweet",
    "Made with love for you",
    "Hehehe, thank you!",
    "You light up my world"
};
static constexpr uint8_t NUM_TOUCH_REACTIONS = sizeof(kTouchReactions) / sizeof(kTouchReactions[0]);

bool AuroraDisplay::begin() {
    Wire.begin(AURORA_OLED_SDA_PIN, AURORA_OLED_SCL_PIN);

    DBG_PRINTLN(F("[OLED] I2C Scanner - looking for devices..."));
    for (byte address = 0; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            DBG_PRINTF("[OLED] I2C device found at 0x%02X\n", address);
        }
    }

    _u8g2.setBusClock(400000);  // 400kHz I2C
    _u8g2.begin();

    _popupActive = false;
    _popupEndsAtMs = 0;

    // Initialize Deskmate State
    _mood = DeskmateMood::IDLE_NORMAL;
    _moodStartMs = millis();
    _lastFrameMs = 0;
    _loveUntilMs = 0;
    _midnightReminder = false;
    _touchReactionIdx = 0;

    _gazeX = 0;
    _gazeY = 0;
    _targetGazeX = 0;
    _targetGazeY = 0;
    _nextGazeChangeMs = millis() + 2500;

    _isBlinking = false;
    _blinkStartMs = 0;
    _blinkDurationMs = 160;
    _nextBlinkMs = millis() + 3000;

    _tearStartMs = millis();

    initHearts();

    const char* driver = AURORA_OLED_DRIVER == 1 ? "SH1106" : "SSD1306";
    DBG_PRINTF("[OLED] begin() OK — %s %dx%d (Deskmate Engine Ready)\n",
               driver, AURORA_OLED_WIDTH, AURORA_OLED_HEIGHT);
    return true;
}

void AuroraDisplay::initHearts() {
    for (uint8_t i = 0; i < MAX_HEARTS; i++) {
        _hearts[i].active = false;
    }
}

void AuroraDisplay::spawnHeart(int16_t x, int16_t y, uint8_t size, uint16_t lifetimeMs) {
    for (uint8_t i = 0; i < MAX_HEARTS; i++) {
        if (!_hearts[i].active) {
            _hearts[i].x = x;
            _hearts[i].y = y;
            _hearts[i].size = size;
            _hearts[i].startMs = millis();
            _hearts[i].lifetimeMs = lifetimeMs;
            _hearts[i].phase = (float)random(0, 628) / 100.0f;
            _hearts[i].active = true;
            break;
        }
    }
}

void AuroraDisplay::updateAndDrawHearts(uint32_t now) {
    for (uint8_t i = 0; i < MAX_HEARTS; i++) {
        if (!_hearts[i].active) continue;
        uint32_t elapsed = now - _hearts[i].startMs;
        if (elapsed >= _hearts[i].lifetimeMs || _hearts[i].y < -12) {
            _hearts[i].active = false;
            continue;
        }
        float progress = (float)elapsed / (float)_hearts[i].lifetimeMs;
        int16_t curY = _hearts[i].y - (int16_t)(progress * 42.0f);
        int16_t curX = _hearts[i].x + (int16_t)(sinf(_hearts[i].phase + progress * 6.28f) * 6.0f);
        drawHeart(curX, curY, _hearts[i].size);
    }
}

void AuroraDisplay::drawHeart(int cx, int cy, int size) {
    if (size <= 0) return;
    int r = (size * 3) / 10;
    if (r < 1) r = 1;
    int dx = (size * 3) / 10;
    int dy = (size * 2) / 10;
    _u8g2.drawDisc(cx - dx, cy - dy, r, U8G2_DRAW_ALL);
    _u8g2.drawDisc(cx + dx, cy - dy, r, U8G2_DRAW_ALL);
    _u8g2.drawTriangle(
        cx - (size * 6) / 10, cy - dy / 2,
        cx + (size * 6) / 10, cy - dy / 2,
        cx, cy + (size * 6) / 10
    );
}

void AuroraDisplay::triggerWarmTouch() {
    _mood = DeskmateMood::LOVE_TOUCHED;
    _moodStartMs = millis();
    _loveUntilMs = millis() + 7000; // Love reaction lasts 7 seconds
    _midnightReminder = false;      // Touch clears midnight reminder

    _touchReactionIdx = (_touchReactionIdx + 1) % NUM_TOUCH_REACTIONS;

    // Spawn an instant celebratory burst of floating hearts!
    spawnHeart(24,  56, random(6, 10), 2200);
    spawnHeart(64,  58, random(8, 12), 2600);
    spawnHeart(104, 56, random(6, 10), 2400);
    spawnHeart(40,  62, random(7, 11), 2000);
    spawnHeart(88,  62, random(7, 11), 2100);

    DBG_PRINTLN(F("[DESKMATE] Warm Touch triggered! Love mood active <3"));
}

void AuroraDisplay::setMood(DeskmateMood mood) {
    if (_mood != mood) {
        _mood = mood;
        _moodStartMs = millis();
        if (mood == DeskmateMood::LONELY_SAD) {
            _tearStartMs = millis();
        }
    }
}

void AuroraDisplay::setMidnightReminder(bool active) {
    _midnightReminder = active;
}

void AuroraDisplay::updateDeskmate() {
    uint32_t now = millis();

    // Check popup auto-dismiss
    if (_popupActive) {
        if (now >= _popupEndsAtMs) {
            _popupActive = false;
        } else {
            return; // Popup wins
        }
    }

    // Frame rate throttle (~20 FPS)
    if ((now - _lastFrameMs) < AURORA_DESKMATE_FRAME_MS) return;
    _lastFrameMs = now;

    // Handle LOVE_TOUCHED timeout -> transition to HAPPY for a brief time then IDLE
    if (_mood == DeskmateMood::LOVE_TOUCHED && now >= _loveUntilMs) {
        _mood = DeskmateMood::HAPPY;
        _moodStartMs = now;
    } else if (_mood == DeskmateMood::HAPPY && (now - _moodStartMs) > 4000) {
        _mood = DeskmateMood::IDLE_NORMAL;
        _moodStartMs = now;
    }

    // Gaze / Saccades update
    if (now >= _nextGazeChangeMs) {
        _nextGazeChangeMs = now + random(2500, 5000);
        uint8_t roll = random(0, 10);
        if (roll < 4) {
            _targetGazeX = 0;   // Center
            _targetGazeY = 0;
        } else if (roll < 6) {
            _targetGazeX = -7;  // Look Left
            _targetGazeY = 0;
        } else if (roll < 8) {
            _targetGazeX = 7;   // Look Right
            _targetGazeY = 0;
        } else if (roll < 9) {
            _targetGazeX = 0;   // Shyly look up
            _targetGazeY = -3;
        } else {
            _targetGazeX = 0;
            _targetGazeY = 2;
        }
    }

    // Smooth gaze interpolation
    if (_gazeX < _targetGazeX) _gazeX++;
    else if (_gazeX > _targetGazeX) _gazeX--;
    if (_gazeY < _targetGazeY) _gazeY++;
    else if (_gazeY > _targetGazeY) _gazeY--;

    // Blinking state machine
    if (!_isBlinking && now >= _nextBlinkMs) {
        _isBlinking = true;
        _blinkStartMs = now;
        _blinkDurationMs = random(140, 190);
    } else if (_isBlinking && (now - _blinkStartMs) >= _blinkDurationMs) {
        _isBlinking = false;
        // Next blink in 2.5 to 5.5 seconds, with occasional rapid double blink
        if (random(0, 5) == 0) {
            _nextBlinkMs = now + 180; // Rapid double blink!
        } else {
            _nextBlinkMs = now + random(2500, 5500);
        }
    }

    switch (_screenMode) {
        case ScreenMode::CLOCK_DATE:
            renderClockDate(now);
            break;
        case ScreenMode::DAILY_THOUGHT:
            renderDailyThought(now);
            break;
        case ScreenMode::PULSE_METER:
            renderPulseMeter(now);
            break;
        case ScreenMode::DESKMATE:
        default:
            renderDeskmate(now);
            break;
    }
}

void AuroraDisplay::renderDeskmate(uint32_t now) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // If Midnight Reminder is active, render the midnight envelope screen
    if (_midnightReminder && _mood != DeskmateMood::LOVE_TOUCHED) {
        drawMidnightScreen(now);
        _u8g2.sendBuffer();
        return;
    }

    int cx1 = 40 + _gazeX;
    int cx2 = 88 + _gazeX;
    int cy  = 26 + _gazeY;

    switch (_mood) {
        case DeskmateMood::LOVE_TOUCHED: {
            drawHeartEyes(cx1, cx2, cy, now);
            drawCheeks(cx1, cx2, cy);
            drawMouth(64, 46, DeskmateMood::LOVE_TOUCHED);

            // Continuously spawn hearts during love mood
            if (random(0, 6) == 0) {
                spawnHeart(random(16, 112), 62, random(5, 9), 2000);
            }
            updateAndDrawHearts(now);

            // Centered romantic reaction banner at the bottom (strictly bounds-checked)
            _u8g2.setFont(u8g2_font_6x12_tr);
            const char* quote = kTouchReactions[_touchReactionIdx];
            int w = _u8g2.getStrWidth(quote);
            int x = (AURORA_OLED_WIDTH - w) / 2;
            if (x < 2) x = 2;
            _u8g2.drawStr(x, 62, quote);
            break;
        }

        case DeskmateMood::HAPPY: {
            drawHappyEyes(cx1, cx2, cy);
            drawCheeks(cx1, cx2, cy);
            drawMouth(64, 45, DeskmateMood::HAPPY);

            if (random(0, 15) == 0) {
                spawnHeart(random(24, 104), 60, random(4, 7), 2200);
            }
            updateAndDrawHearts(now);
            break;
        }

        case DeskmateMood::LONELY_SAD: {
            drawSadEyes(cx1, cx2, cy, now);
            drawMouth(64, 47, DeskmateMood::LONELY_SAD);

            // Gentle attention banner at bottom (guaranteed not clipped)
            _u8g2.setFont(u8g2_font_6x12_tr);
            const char* sadPrompt = "Miss you... Touch me?";
            int w = _u8g2.getStrWidth(sadPrompt);
            int x = (AURORA_OLED_WIDTH - w) / 2;
            if (x < 2) x = 2;
            _u8g2.drawStr(x, 62, sadPrompt);
            break;
        }

        case DeskmateMood::SLEEPING: {
            drawSleepingEyes(cx1, cx2, cy, now);
            drawMouth(64, 46, DeskmateMood::SLEEPING);
            break;
        }

        case DeskmateMood::IDLE_NORMAL:
        default: {
            uint8_t blinkPct = 0;
            if (_isBlinking) {
                uint32_t bElapsed = now - _blinkStartMs;
                uint16_t half = _blinkDurationMs / 2;
                if (bElapsed < half) {
                    blinkPct = (bElapsed * 100) / half;
                } else {
                    blinkPct = ((_blinkDurationMs - bElapsed) * 100) / half;
                }
            }
            drawNormalEyes(cx1, cx2, cy, 26, 24, blinkPct);
            drawMouth(64, 46, DeskmateMood::IDLE_NORMAL);
            break;
        }
    }

    _u8g2.sendBuffer();
}

void AuroraDisplay::drawNormalEyes(int cx1, int cx2, int cy, int w, int h, uint8_t blinkPct) {
    int curH = ((100 - blinkPct) * h) / 100;
    if (curH < 2) curH = 2;

    if (curH <= 3) {
        // Closed eyelid slit
        _u8g2.drawHLine(cx1 - w / 2, cy, w);
        _u8g2.drawHLine(cx2 - w / 2, cy, w);
    } else {
        int r = curH > 8 ? 6 : (curH / 2);
        // Rounded glowing eye boxes
        _u8g2.drawRBox(cx1 - w / 2, cy - curH / 2, w, curH, r);
        _u8g2.drawRBox(cx2 - w / 2, cy - curH / 2, w, curH, r);

        // Soulful pupil highlights (cutouts in black)
        if (curH > 10) {
            _u8g2.setDrawColor(0);
            _u8g2.drawDisc(cx1 - 4, cy - curH / 5, 3, U8G2_DRAW_ALL);
            _u8g2.drawDisc(cx1 + 4, cy + curH / 5, 2, U8G2_DRAW_ALL);
            _u8g2.drawDisc(cx2 - 4, cy - curH / 5, 3, U8G2_DRAW_ALL);
            _u8g2.drawDisc(cx2 + 4, cy + curH / 5, 2, U8G2_DRAW_ALL);
            _u8g2.setDrawColor(1);
        }
    }
}

void AuroraDisplay::drawHappyEyes(int cx1, int cx2, int cy) {
    // Cute arched anime smile eyes: ^  ^
    for (int dy = 0; dy <= 2; dy++) {
        _u8g2.drawCircle(cx1, cy + 4 + dy, 12, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        _u8g2.drawCircle(cx2, cy + 4 + dy, 12, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    }
}

void AuroraDisplay::drawHeartEyes(int cx1, int cx2, int cy, uint32_t now) {
    // Pulsing heart eyes: <3  <3
    float beat = sinf((float)(now % 800) / 800.0f * 6.28318f);
    int heartSize = 16 + (int)(beat * 3.0f);
    drawHeart(cx1, cy, heartSize);
    drawHeart(cx2, cy, heartSize);
}

void AuroraDisplay::drawSadEyes(int cx1, int cx2, int cy, uint32_t now) {
    // Droopy rounded eyes
    _u8g2.drawRBox(cx1 - 12, cy - 6, 24, 18, 4);
    _u8g2.drawRBox(cx2 - 12, cy - 6, 24, 18, 4);

    // Slanted upper eyelid cutouts for sad expression
    _u8g2.setDrawColor(0);
    _u8g2.drawTriangle(cx1 - 14, cy - 8, cx1 + 14, cy - 8, cx1 + 14, cy + 2);
    _u8g2.drawTriangle(cx2 + 14, cy - 8, cx2 - 14, cy - 8, cx2 - 14, cy + 2);

    // Sad pupil shines
    _u8g2.drawDisc(cx1, cy + 2, 2, U8G2_DRAW_ALL);
    _u8g2.drawDisc(cx2, cy + 2, 2, U8G2_DRAW_ALL);
    _u8g2.setDrawColor(1);

    // Animated sliding teardrop on right cheek
    uint32_t t = (now - _tearStartMs) % 2200;
    float prog = (float)t / 2200.0f;
    int tearY = (cy + 2) + (int)(prog * 22);
    if (prog < 0.9f) {
        _u8g2.drawDisc(cx2 + 12, tearY, 2, U8G2_DRAW_ALL);
        _u8g2.drawPixel(cx2 + 12, tearY - 2);
    }
}

void AuroraDisplay::drawSleepingEyes(int cx1, int cx2, int cy, uint32_t now) {
    // Curved resting eyes: ( -  - )
    for (int dy = 0; dy <= 2; dy++) {
        _u8g2.drawCircle(cx1, cy - 2 + dy, 11, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
        _u8g2.drawCircle(cx2, cy - 2 + dy, 11, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    }

    // Drifting "z Z Z" bubbles
    _u8g2.setFont(u8g2_font_5x7_tf);
    uint32_t phase = (now / 30) % 60;
    _u8g2.drawStr(102 + (phase / 10), 22 - (phase / 4), "z");
    _u8g2.drawStr(112 + (phase / 8), 14 - (phase / 4), "Z");
}

void AuroraDisplay::drawCheeks(int cx1, int cx2, int cy) {
    // Rosy blushing cheek slashes: ///
    int y = cy + 14;
    for (int i = -6; i <= 2; i += 4) {
        _u8g2.drawLine(cx1 + i - 2, y + 3, cx1 + i + 2, y - 3);
        _u8g2.drawLine(cx2 + i - 2, y + 3, cx2 + i + 2, y - 3);
    }
}

void AuroraDisplay::drawMouth(int cx, int cy, DeskmateMood mood) {
    switch (mood) {
        case DeskmateMood::LOVE_TOUCHED:
        case DeskmateMood::HAPPY:
            // Cute open smile: \_/
            _u8g2.drawDisc(cx, cy - 1, 4, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
            break;

        case DeskmateMood::LONELY_SAD:
            // Sad downward arc: n
            _u8g2.drawCircle(cx, cy + 3, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
            break;

        case DeskmateMood::SLEEPING:
            // Peaceful small line
            _u8g2.drawHLine(cx - 3, cy, 6);
            break;

        case DeskmateMood::IDLE_NORMAL:
        default:
            // Cute cat mouth: w
            _u8g2.drawCircle(cx - 3, cy - 1, 3, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
            _u8g2.drawCircle(cx + 3, cy - 1, 3, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
            break;
    }
}

void AuroraDisplay::drawMidnightScreen(uint32_t now) {
    // Title
    _u8g2.setFont(u8g2_font_7x14_tr);
    const char* title = "12:00 Midnight!";
    int tw = _u8g2.getStrWidth(title);
    _u8g2.drawStr((AURORA_OLED_WIDTH - tw) / 2, 13, title);

    // Animated love envelope in the center
    int envX = 48;
    int envY = 18;
    int envW = 32;
    int envH = 20;

    _u8g2.drawFrame(envX, envY, envW, envH);
    _u8g2.drawLine(envX, envY, envX + envW / 2, envY + 11);
    _u8g2.drawLine(envX + envW, envY, envX + envW / 2, envY + 11);

    // Heart wax seal in center
    float beat = sinf((float)(now % 900) / 900.0f * 6.28318f);
    int sealSize = 6 + (int)(beat * 1.5f);
    drawHeart(envX + envW / 2, envY + 10, sealSize);

    // Glimmer sparkles around envelope
    if ((now / 200) % 2 == 0) {
        _u8g2.drawPixel(34, 22);
        _u8g2.drawPixel(34, 24);
        _u8g2.drawPixel(33, 23);
        _u8g2.drawPixel(35, 23);

        _u8g2.drawPixel(94, 30);
        _u8g2.drawPixel(94, 32);
        _u8g2.drawPixel(93, 31);
        _u8g2.drawPixel(95, 31);
    }

    // Call-to-action text: clean, centered, no clipping
    _u8g2.setFont(u8g2_font_6x12_tr);
    const char* sub = "Daily Message Ready!";
    int sw = _u8g2.getStrWidth(sub);
    _u8g2.drawStr((AURORA_OLED_WIDTH - sw) / 2, 49, sub);

    _u8g2.setFont(u8g2_font_5x7_tf);
    const char* prompt = "< Touch to open >";
    int pw = _u8g2.getStrWidth(prompt);
    _u8g2.drawStr((AURORA_OLED_WIDTH - pw) / 2, 60, prompt);
}

void AuroraDisplay::showBootScreen() {
    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_7x14_tr);
    _u8g2.setDrawColor(1);

    const char* title = "Aurora Deskmate";
    int titleWidth = _u8g2.getStrWidth(title);
    int titleX = (AURORA_OLED_WIDTH - titleWidth) / 2;
    _u8g2.drawStr(titleX, 22, title);

    drawHeart(64, 36, 12);

    _u8g2.setFont(u8g2_font_6x12_tr);
    const char* sub = "For Chandni <3";
    int sw = _u8g2.getStrWidth(sub);
    _u8g2.drawStr((AURORA_OLED_WIDTH - sw) / 2, 58, sub);

    _u8g2.sendBuffer();
}

void AuroraDisplay::showStatusScreen(const char* line1, const char* line2, const char* line3) {
    if (_popupActive && millis() < _popupEndsAtMs) return;

    _u8g2.clearBuffer();
    _u8g2.setFont(u8g2_font_6x12_tr);
    _u8g2.setDrawColor(1);

    if (line1) _u8g2.drawStr(2, 14, line1);
    if (line2) _u8g2.drawStr(2, 34, line2);
    if (line3) _u8g2.drawStr(2, 54, line3);

    _u8g2.sendBuffer();
}

void AuroraDisplay::showCenteredText(const char* text, int16_t y, uint8_t textSize) {
    if (textSize >= 2) {
        _u8g2.setFont(u8g2_font_7x14_tr);
    } else {
        _u8g2.setFont(u8g2_font_6x12_tr);
    }

    int textWidth = _u8g2.getStrWidth(text);
    int x = (AURORA_OLED_WIDTH - textWidth) / 2;
    if (x < 2) x = 2;
    _u8g2.drawStr(x, y + 8, text);
}

void AuroraDisplay::popup(const char* text, uint32_t durationMs) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // Header bar (inverted - white background)
    _u8g2.drawBox(0, 0, AURORA_OLED_WIDTH, 12);

    // Header text (black on white)
    _u8g2.setDrawColor(0);  // black
    _u8g2.setFont(u8g2_font_6x10_tr);
    _u8g2.drawStr(4, 9, "! Notice");

    // Main message (white text)
    _u8g2.setDrawColor(1);  // white

    // Dynamically choose font based on text width to ensure NO clipping on RHS!
    _u8g2.setFont(u8g2_font_7x14_tr);
    int textWidth = _u8g2.getStrWidth(text);
    if (textWidth > 122) {
        _u8g2.setFont(u8g2_font_6x12_tr);
        textWidth = _u8g2.getStrWidth(text);
        if (textWidth > 122) {
            _u8g2.setFont(u8g2_font_5x7_tf);
            textWidth = _u8g2.getStrWidth(text);
        }
    }
    int x = (AURORA_OLED_WIDTH - textWidth) / 2;
    if (x < 2) x = 2;
    _u8g2.drawStr(x, 38, text);

    _u8g2.sendBuffer();

    _popupActive = true;
    _popupEndsAtMs = millis() + durationMs;
}

bool AuroraDisplay::isPopupActive() const {
    if (_popupActive && millis() >= _popupEndsAtMs) {
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

void AuroraDisplay::sleep() {
    _u8g2.clearBuffer();
    _u8g2.sendBuffer();
    _u8g2.setPowerSave(1); // Turn off OLED display panel & internal charge pump (0 uA)
}

void AuroraDisplay::wake() {
    _u8g2.setPowerSave(0); // Power on OLED panel
}

void AuroraDisplay::cycleScreenMode() {
    uint8_t next = ((uint8_t)_screenMode + 1) % (uint8_t)ScreenMode::MODE_COUNT;
    _screenMode = (ScreenMode)next;
    _modeBadgeUntilMs = millis() + 900;
}

void AuroraDisplay::setScreenMode(ScreenMode mode) {
    _screenMode = mode;
    _modeBadgeUntilMs = millis() + 900;
}

void AuroraDisplay::drawWrappedText(int x, int y, const char* text, int maxW, int lineH) {
    if (!text || *text == '\0') return;
    char word[32];
    char line[64] = {0};
    const char* p = text;
    int curY = y;

    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        int wlen = 0;
        while (*p && *p != ' ' && *p != '\n' && wlen < 31) {
            word[wlen++] = *p++;
        }
        word[wlen] = '\0';

        char testLine[64];
        if (line[0] == '\0') {
            strncpy(testLine, word, sizeof(testLine));
        } else {
            snprintf(testLine, sizeof(testLine), "%s %s", line, word);
        }

        if (_u8g2.getStrWidth(testLine) <= maxW) {
            strncpy(line, testLine, sizeof(line));
        } else {
            if (line[0] != '\0') {
                _u8g2.drawStr(x, curY, line);
                curY += lineH;
            }
            strncpy(line, word, sizeof(line));
        }

        if (*p == '\n') {
            if (line[0] != '\0') {
                _u8g2.drawStr(x, curY, line);
                curY += lineH;
                line[0] = '\0';
            }
            p++;
        }
    }

    if (line[0] != '\0') {
        _u8g2.drawStr(x, curY, line);
    }
}

void AuroraDisplay::renderClockDate(uint32_t now) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    uint32_t localEp = AuroraState::instance().localEpoch();
    char timeStr[16] = "--:--";
    char dateStr[24] = "Connecting time...";
    
    if (localEp > 0) {
        aurora_clock::formatTime(timeStr, sizeof(timeStr), localEp);
        aurora_clock::formatDate(dateStr, sizeof(dateStr), localEp);
    }

    // 1. Dynamic Contextual Greeting Banner
    _u8g2.setFont(u8g2_font_6x12_tr);
    const char* greeting = "Sweet dreams, Moon";
    if (localEp > 0) {
        uint32_t h = (localEp / 3600) % 24;
        if (aurora_clock::isBirthday(localEp)) {
            greeting = "Happy Birthday Chandni!";
        } else if (h >= 5 && h < 12) {
            greeting = "Good morning, Chandni";
        } else if (h >= 12 && h < 17) {
            greeting = "Good afternoon, Chandni";
        } else if (h >= 17 && h < 22) {
            greeting = "Good evening, Chandni";
        }
    }
    int gw = _u8g2.getStrWidth(greeting);
    int gx = (AURORA_OLED_WIDTH - gw) / 2;
    if (gx < 8) gx = 8;
    _u8g2.drawStr(gx, 12, greeting);
    drawHeart(gx - 6, 8, 3);
    drawHeart(gx + gw + 6, 8, 3);

    // 2. Curvy Digital Clock
    if ((now / 500) % 2 != 0 && strlen(timeStr) >= 5) {
        timeStr[2] = ' '; // blink colon every 500ms
    }
    _u8g2.setFont(u8g2_font_fur20_tn);
    int tw = _u8g2.getStrWidth(timeStr);
    int tx = (AURORA_OLED_WIDTH - tw) / 2;
    _u8g2.drawStr(tx, 40, timeStr);

    // 3. Formatted Calendar Date
    _u8g2.setFont(u8g2_font_6x10_tr);
    int dw = _u8g2.getStrWidth(dateStr);
    int dx = (AURORA_OLED_WIDTH - dw) / 2;
    if (dx < 4) dx = 4;
    _u8g2.drawStr(dx, 58, dateStr);

    updateAndDrawHearts(now);
    _u8g2.sendBuffer();
}

void AuroraDisplay::renderDailyThought(uint32_t now) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // Header: [ Today's Thought ] flanked by geometric hearts
    _u8g2.setFont(u8g2_font_6x12_tr);
    const char* header = "[ Today's Thought ]";
    int hw = _u8g2.getStrWidth(header);
    int hx = (AURORA_OLED_WIDTH - hw) / 2;
    _u8g2.drawStr(hx, 11, header);
    drawHeart(hx - 6, 7, 3);
    drawHeart(hx + hw + 6, 7, 3);

    _u8g2.drawHLine(8, 14, 112);

    // Body text wrapped within 112px safe margins
    _u8g2.setFont(u8g2_font_5x7_tf);
    uint32_t ep = AuroraState::instance().localEpoch();
    uint16_t doy = aurora_clock::dayOfYear(ep > 0 ? ep : 0);
    uint8_t msgIdx = aurora_messages::indexForDoy(doy);
    const char* msg = aurora_messages::kMessages[msgIdx];
    drawWrappedText(8, 25, msg, 112, 9);

    updateAndDrawHearts(now);
    _u8g2.sendBuffer();
}

void AuroraDisplay::renderPulseMeter(uint32_t now) {
    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // Header
    _u8g2.setFont(u8g2_font_6x10_tr);
    const char* title = "Heartbeat Keepsake";
    int tw = _u8g2.getStrWidth(title);
    int tx = (AURORA_OLED_WIDTH - tw) / 2;
    _u8g2.drawStr(tx, 9, title);

    // Dynamic BPM: 72 steady, accelerates to 118 on touch for 7 seconds
    bool isExcited = (now - AuroraState::instance().lastTouchMs()) < 7000;
    uint8_t bpm = isExcited ? 118 : 72;
    uint8_t heartSize = isExcited ? (5 + (now / 150) % 3) : (4 + (now / 350) % 2);

    // Beating geometric heart
    drawHeart(64, 23, heartSize);

    // Dynamic real-time sweeping ECG line
    int sweepX = (now / 15) % 116;
    for (int x = 6; x < 122; x++) {
        int relX = (x + (now / 12)) % 60;
        int yOffset = 0;
        if (relX >= 20 && relX < 24) yOffset = -2;      // P wave
        else if (relX == 28) yOffset = 3;               // Q
        else if (relX == 30) yOffset = -14;             // R peak
        else if (relX == 32) yOffset = 6;               // S
        else if (relX >= 36 && relX < 42) yOffset = -3; // T wave

        int y = 38 + yOffset;
        if (x != (sweepX + 6)) {
            _u8g2.drawPixel(x, y);
        }
    }
    _u8g2.drawDisc(sweepX + 6, 38, 2); // moving sweep blip

    // Keepsake Warm Touches lifetime counter & BPM
    char buf[36];
    snprintf(buf, sizeof(buf), "%u Touches - %u BPM",
             (unsigned)AuroraState::instance().touches(), bpm);
    _u8g2.setFont(u8g2_font_5x7_tf);
    int bw = _u8g2.getStrWidth(buf);
    int bx = (AURORA_OLED_WIDTH - bw) / 2;
    if (bx < 4) bx = 4;
    _u8g2.drawStr(bx, 58, buf);
    drawHeart(bx - 5, 55, 3);
    drawHeart(bx + bw + 5, 55, 3);

    updateAndDrawHearts(now);
    _u8g2.sendBuffer();
}
