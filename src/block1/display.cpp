/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block1/display.cpp
 *
 *  OLED display driver and multi-mode engine:
 *  - Mode 1: Animated Deskmate Companion ("Aurora")
 *  - Mode 2: Clock, Date & Contextual Greeting
 *  - Mode 3: Daily Thought / Affirmation Card
 *  - Mode 4: Heartbeat Keepsake & ECG Pulse
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

// Romantic rotating reactions for Warm Touches
static const char* const kTouchReactions[] = {
    "Warm Touch felt!",
    "Aurora loves you!",
    "You're my favorite",
    "Stay cozy, Chandni",
    "So warm and sweet <3",
    "Made with love for you",
    "Hehehe, thank you! <3",
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

    // Initialize Modes & Deskmate State
    _screenMode = ScreenMode::DESKMATE;
    _modeBadgeUntilMs = 0;

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
    DBG_PRINTF("[OLED] begin() OK — %s %dx%d (Multi-mode Engine Ready)\n",
               driver, AURORA_OLED_WIDTH, AURORA_OLED_HEIGHT);
    return true;
}

void AuroraDisplay::setScreenMode(ScreenMode mode) {
    _screenMode = mode;
    _modeBadgeUntilMs = millis() + 1200; // Display toast badge for 1.2s
    DBG_PRINTF("[OLED] Screen Mode changed to %d (%s)\n", (int)_screenMode, screenModeName());
}

void AuroraDisplay::cycleScreenMode() {
    uint8_t next = ((uint8_t)_screenMode + 1) % 4;
    setScreenMode((ScreenMode)next);
}

const char* AuroraDisplay::screenModeName() const {
    switch (_screenMode) {
        case ScreenMode::DESKMATE:           return "Aurora Deskmate";
        case ScreenMode::CLOCK_DATE:         return "Clock & Greeting";
        case ScreenMode::DAILY_QUOTE:        return "Daily Thought";
        case ScreenMode::HEARTBEAT_KEEPSAKE: return "Keepsake Pulse";
    }
    return "Aurora";
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

    // Spawn an instant celebratory burst of floating hearts
    spawnHeart(24,  56, random(6, 10), 2200);
    spawnHeart(64,  58, random(8, 12), 2600);
    spawnHeart(104, 56, random(6, 10), 2400);
    spawnHeart(40,  62, random(7, 11), 2000);
    spawnHeart(88,  62, random(7, 11), 2100);

    DBG_PRINTLN(F("[TOUCH] Warm Touch triggered! Love mood active <3"));
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

void AuroraDisplay::update() {
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
        if (random(0, 5) == 0) {
            _nextBlinkMs = now + 180; // Rapid double blink!
        } else {
            _nextBlinkMs = now + random(2500, 5500);
        }
    }

    _u8g2.clearBuffer();
    _u8g2.setDrawColor(1);

    // If Midnight Reminder is active, render the midnight envelope screen
    if (_midnightReminder && _mood != DeskmateMood::LOVE_TOUCHED) {
        drawMidnightScreen(now);
        _u8g2.sendBuffer();
        return;
    }

    // Render the active screen mode
    switch (_screenMode) {
        case ScreenMode::CLOCK_DATE:
            renderClockDate(now);
            break;
        case ScreenMode::DAILY_QUOTE:
            renderDailyQuote(now);
            break;
        case ScreenMode::HEARTBEAT_KEEPSAKE:
            renderHeartbeat(now);
            break;
        case ScreenMode::DESKMATE:
        default:
            renderDeskmate(now);
            break;
    }

    // Overlay mode transition badge if active
    if (now < _modeBadgeUntilMs) {
        renderModeBadge(now);
    }

    _u8g2.sendBuffer();
}

void AuroraDisplay::renderModeBadge(uint32_t now) {
    const char* name = screenModeName();
    _u8g2.setFont(u8g2_font_5x7_tf);
    int w = _u8g2.getStrWidth(name);
    int x = (AURORA_OLED_WIDTH - (w + 12)) / 2;

    _u8g2.setDrawColor(0);
    _u8g2.drawRBox(x, 1, w + 12, 11, 2);
    _u8g2.setDrawColor(1);
    _u8g2.drawRFrame(x, 1, w + 12, 11, 2);
    _u8g2.drawStr(x + 6, 9, name);
}

void AuroraDisplay::renderClockDate(uint32_t now) {
    uint32_t epoch = AuroraState::instance().epoch();
    uint32_t h = (epoch / 3600) % 24;
    uint32_t m = (epoch / 60) % 60;

    // 1. Contextual Greeting Header (Top)
    const char* greeting;
    if (aurora_clock::isBirthday(epoch)) {
        greeting = "* Happy Birthday Chandni! *";
    } else if (h >= 5 && h < 12) {
        greeting = "Good morning, Chandni <3";
    } else if (h >= 12 && h < 17) {
        greeting = "Good afternoon, Chandni <3";
    } else if (h >= 17 && h < 22) {
        greeting = "Good evening, Chandni <3";
    } else {
        greeting = "Sweet dreams, Moon <3";
    }

    _u8g2.setFont(u8g2_font_6x10_tr);
    int gw = _u8g2.getStrWidth(greeting);
    int gx = (AURORA_OLED_WIDTH - gw) / 2;
    if (gx < 2) gx = 2;
    _u8g2.drawStr(gx, 11, greeting);
    _u8g2.drawHLine(6, 14, 116);

    // 2. Large Curvy Digital Clock (Center)
    char timeBuf[8];
    // Blinking colon every 500ms
    snprintf(timeBuf, sizeof(timeBuf), "%02lu%c%02lu",
             (unsigned long)h, (now % 1000 < 500 ? ':' : ' '), (unsigned long)m);
    _u8g2.setFont(u8g2_font_logisoso24_tn);
    int tw = _u8g2.getStrWidth(timeBuf);
    _u8g2.drawStr((AURORA_OLED_WIDTH - tw) / 2, 43, timeBuf);

    // Decorative corner hearts
    drawHeart(12, 31, 5);
    drawHeart(116, 31, 5);

    // 3. Date at Bottom
    char dateBuf[24];
    aurora_clock::formatDate(dateBuf, sizeof(dateBuf), epoch);
    _u8g2.setFont(u8g2_font_6x10_tr);
    int dw = _u8g2.getStrWidth(dateBuf);
    _u8g2.drawStr((AURORA_OLED_WIDTH - dw) / 2, 59, dateBuf);

    // Update floating hearts if active
    updateAndDrawHearts(now);
}

void AuroraDisplay::renderDailyQuote(uint32_t now) {
    uint32_t epoch = AuroraState::instance().epoch();
    uint16_t doy = aurora_clock::dayOfYear(epoch);
    uint8_t msgIdx = aurora_messages::indexForDoy(doy);
    const char* msg = aurora_messages::kMessages[msgIdx];

    // Header
    _u8g2.setFont(u8g2_font_6x10_tr);
    const char* header = "[ Today's Thought <3 ]";
    int hw = _u8g2.getStrWidth(header);
    _u8g2.drawStr((AURORA_OLED_WIDTH - hw) / 2, 10, header);
    _u8g2.drawHLine(4, 13, 120);

    // Word-wrapped quote text
    drawWrappedText(msg, 24, 4, 10);

    // Floating hearts
    updateAndDrawHearts(now);
}

void AuroraDisplay::drawPulseWave(int startX, int endX, int centerY) {
    int curX = startX;

    // Flat baseline
    _u8g2.drawLine(curX, centerY, curX + 16, centerY);
    curX += 16;

    // Small P wave
    _u8g2.drawLine(curX, centerY, curX + 4, centerY - 3);
    _u8g2.drawLine(curX + 4, centerY - 3, curX + 8, centerY);
    curX += 8;

    // Flat segment
    _u8g2.drawLine(curX, centerY, curX + 4, centerY);
    curX += 4;

    // Q dip
    _u8g2.drawLine(curX, centerY, curX + 2, centerY + 2);
    curX += 2;

    // R sharp spike up
    _u8g2.drawLine(curX, centerY + 2, curX + 6, centerY - 15);
    curX += 6;

    // S sharp spike down
    _u8g2.drawLine(curX, centerY - 15, curX + 6, centerY + 9);
    curX += 6;

    // Return to baseline
    _u8g2.drawLine(curX, centerY + 9, curX + 4, centerY);
    curX += 4;

    // T wave
    _u8g2.drawLine(curX, centerY, curX + 6, centerY - 4);
    _u8g2.drawLine(curX + 6, centerY - 4, curX + 12, centerY);
    curX += 12;

    // Return flat line to endX
    if (curX < endX) {
        _u8g2.drawLine(curX, centerY, endX, centerY);
    }
}

void AuroraDisplay::renderHeartbeat(uint32_t now) {
    // 1. Pulsing Beating Heart on the left
    float beat = sinf((float)(now % 800) / 800.0f * 6.28318f);
    int heartSize = 16 + (int)(beat * 3.5f);
    drawHeart(26, 26, heartSize);

    // 2. ECG pulse line on the right
    drawPulseWave(50, 122, 26);

    _u8g2.setFont(u8g2_font_5x7_tf);
    _u8g2.drawStr(98, 12, "72 bpm");

    _u8g2.drawHLine(6, 44, 116);

    // 3. Lifetime Keepsake Touches Counter at Bottom
    char buf[32];
    snprintf(buf, sizeof(buf), "<3 %lu Warm Touches <3",
             (unsigned long)AuroraState::instance().touches());
    _u8g2.setFont(u8g2_font_6x12_tr);
    int bw = _u8g2.getStrWidth(buf);
    int bx = (AURORA_OLED_WIDTH - bw) / 2;
    if (bx < 2) bx = 2;
    _u8g2.drawStr(bx, 58, buf);

    // Floating hearts
    updateAndDrawHearts(now);
}

void AuroraDisplay::drawWrappedText(const char* text, int startY, int maxLines, int lineHeight) {
    _u8g2.setFont(u8g2_font_5x7_tf);

    const int maxLineWidth = 122;
    int line = 0;
    int y = startY;

    const char* ptr = text;
    char lineBuf[32];
    int lineBufLen = 0;

    while (*ptr && line < maxLines) {
        // Skip leading space on new line
        if (lineBufLen == 0 && *ptr == ' ') {
            ptr++;
            continue;
        }

        // Find next word
        const char* wordStart = ptr;
        while (*ptr && *ptr != ' ') ptr++;
        int wordLen = ptr - wordStart;

        // Check if word fits on current line
        char testBuf[32];
        if (lineBufLen == 0) {
            snprintf(testBuf, sizeof(testBuf), "%.*s", wordLen, wordStart);
        } else {
            snprintf(testBuf, sizeof(testBuf), "%s %.*s", lineBuf, wordLen, wordStart);
        }

        if (_u8g2.getStrWidth(testBuf) <= maxLineWidth) {
            // Fits on current line
            strncpy(lineBuf, testBuf, sizeof(lineBuf) - 1);
            lineBuf[sizeof(lineBuf) - 1] = '\0';
            lineBufLen = strlen(lineBuf);
            if (*ptr == ' ') ptr++;
        } else {
            // Line full, print lineBuf
            if (lineBufLen > 0) {
                int lw = _u8g2.getStrWidth(lineBuf);
                _u8g2.drawStr((AURORA_OLED_WIDTH - lw) / 2, y, lineBuf);
                line++;
                y += lineHeight;
                lineBufLen = 0;
                lineBuf[0] = '\0';
            } else {
                // Single word exceeds line width, force print
                strncpy(lineBuf, testBuf, sizeof(lineBuf) - 1);
                lineBuf[sizeof(lineBuf) - 1] = '\0';
                _u8g2.drawStr(2, y, lineBuf);
                line++;
                y += lineHeight;
                lineBufLen = 0;
                lineBuf[0] = '\0';
                if (*ptr == ' ') ptr++;
            }
        }
    }

    // Print remaining line if space exists
    if (lineBufLen > 0 && line < maxLines) {
        int lw = _u8g2.getStrWidth(lineBuf);
        _u8g2.drawStr((AURORA_OLED_WIDTH - lw) / 2, y, lineBuf);
    }
}

void AuroraDisplay::renderDeskmate(uint32_t now) {
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

            // Centered romantic reaction banner at bottom
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
}

void AuroraDisplay::drawNormalEyes(int cx1, int cx2, int cy, int w, int h, uint8_t blinkPct) {
    int curH = ((100 - blinkPct) * h) / 100;
    if (curH < 2) curH = 2;

    if (curH <= 3) {
        _u8g2.drawHLine(cx1 - w / 2, cy, w);
        _u8g2.drawHLine(cx2 - w / 2, cy, w);
    } else {
        int r = curH > 8 ? 6 : (curH / 2);
        _u8g2.drawRBox(cx1 - w / 2, cy - curH / 2, w, curH, r);
        _u8g2.drawRBox(cx2 - w / 2, cy - curH / 2, w, curH, r);

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
    for (int dy = 0; dy <= 2; dy++) {
        _u8g2.drawCircle(cx1, cy + 4 + dy, 12, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        _u8g2.drawCircle(cx2, cy + 4 + dy, 12, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    }
}

void AuroraDisplay::drawHeartEyes(int cx1, int cx2, int cy, uint32_t now) {
    float beat = sinf((float)(now % 800) / 800.0f * 6.28318f);
    int heartSize = 16 + (int)(beat * 3.0f);
    drawHeart(cx1, cy, heartSize);
    drawHeart(cx2, cy, heartSize);
}

void AuroraDisplay::drawSadEyes(int cx1, int cx2, int cy, uint32_t now) {
    _u8g2.drawRBox(cx1 - 12, cy - 6, 24, 18, 4);
    _u8g2.drawRBox(cx2 - 12, cy - 6, 24, 18, 4);

    _u8g2.setDrawColor(0);
    _u8g2.drawTriangle(cx1 - 14, cy - 8, cx1 + 14, cy - 8, cx1 + 14, cy + 2);
    _u8g2.drawTriangle(cx2 + 14, cy - 8, cx2 - 14, cy - 8, cx2 - 14, cy + 2);

    _u8g2.drawDisc(cx1, cy + 2, 2, U8G2_DRAW_ALL);
    _u8g2.drawDisc(cx2, cy + 2, 2, U8G2_DRAW_ALL);
    _u8g2.setDrawColor(1);

    uint32_t t = (now - _tearStartMs) % 2200;
    float prog = (float)t / 2200.0f;
    int tearY = (cy + 2) + (int)(prog * 22);
    if (prog < 0.9f) {
        _u8g2.drawDisc(cx2 + 12, tearY, 2, U8G2_DRAW_ALL);
        _u8g2.drawPixel(cx2 + 12, tearY - 2);
    }
}

void AuroraDisplay::drawSleepingEyes(int cx1, int cx2, int cy, uint32_t now) {
    for (int dy = 0; dy <= 2; dy++) {
        _u8g2.drawCircle(cx1, cy - 2 + dy, 11, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
        _u8g2.drawCircle(cx2, cy - 2 + dy, 11, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    }

    _u8g2.setFont(u8g2_font_5x7_tf);
    uint32_t phase = (now / 30) % 60;
    _u8g2.drawStr(102 + (phase / 10), 22 - (phase / 4), "z");
    _u8g2.drawStr(112 + (phase / 8), 14 - (phase / 4), "Z");
}

void AuroraDisplay::drawCheeks(int cx1, int cx2, int cy) {
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
            _u8g2.drawDisc(cx, cy - 1, 4, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
            break;

        case DeskmateMood::LONELY_SAD:
            _u8g2.drawCircle(cx, cy + 3, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
            break;

        case DeskmateMood::SLEEPING:
            _u8g2.drawHLine(cx - 3, cy, 6);
            break;

        case DeskmateMood::IDLE_NORMAL:
        default:
            _u8g2.drawCircle(cx - 3, cy - 1, 3, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
            _u8g2.drawCircle(cx + 3, cy - 1, 3, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
            break;
    }
}

void AuroraDisplay::drawMidnightScreen(uint32_t now) {
    _u8g2.setFont(u8g2_font_7x14_tr);
    const char* title = "12:00 Midnight!";
    int tw = _u8g2.getStrWidth(title);
    _u8g2.drawStr((AURORA_OLED_WIDTH - tw) / 2, 13, title);

    int envX = 48;
    int envY = 18;
    int envW = 32;
    int envH = 20;

    _u8g2.drawFrame(envX, envY, envW, envH);
    _u8g2.drawLine(envX, envY, envX + envW / 2, envY + 11);
    _u8g2.drawLine(envX + envW, envY, envX + envW / 2, envY + 11);

    float beat = sinf((float)(now % 900) / 900.0f * 6.28318f);
    int sealSize = 6 + (int)(beat * 1.5f);
    drawHeart(envX + envW / 2, envY + 10, sealSize);

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
