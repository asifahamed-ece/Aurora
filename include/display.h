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

// Deskmate pet mood states
enum class DeskmateMood : uint8_t {
    IDLE_NORMAL,        // Looking around, organic blinking, cute smile
    HAPPY,              // Bouncy happy arc eyes, blushing
    LOVE_TOUCHED,       // Beating heart eyes, floating hearts, blushing, romantic reaction
    LONELY_SAD,         // No touch for 1 hour: droopy sad eyes, teardrop, needs attention
    MIDNIGHT_REMINDER,  // 12 Midnight: animated envelope & stars, daily message reminder
    SLEEPING            // Late-night calm curved sleeping eyes, drifting z Z Z
};

// OLED screen modes cycled by the multi-function button (short press).
enum class ScreenMode : uint8_t {
    DESKMATE = 0,         // Animated Aurora deskmate (default)
    CLOCK_DATE,           // Large digital clock + contextual greeting + date
    DAILY_QUOTE,          // Today's rotating daily thought
    QR_CODE,              // "Connect Aurora WiFi and Scan" + dashboard URL QR
    HEARTBEAT_KEEPSAKE    // Pulsing heart + ECG pulse + lifetime Warm Touches
};

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
     * Main animation loop update - call frequently in loop().
     * Throttled internally to ~20 FPS. Renders the active ScreenMode.
     */
    void update();

    /**
     * Backwards-compatible alias for update(). Older callers used this name.
     */
    void updateDeskmate() { update(); }

    /**
     * Trigger a Warm Touch reaction on the deskmate!
     * Puts Aurora into LOVE_TOUCHED with heart eyes, floating hearts,
     * blushing cheeks, and romantic reaction text.
     */
    void triggerWarmTouch();

    /**
     * Set the current deskmate mood directly.
     */
    void setMood(DeskmateMood mood);
    DeskmateMood currentMood() const { return _mood; }

    /**
     * Set or clear the midnight daily message reminder alert.
     */
    void setMidnightReminder(bool active);
    bool isMidnightReminderActive() const { return _midnightReminder; }

    /**
     * Switch to a specific ScreenMode and pop a brief badge toast.
     */
    void setScreenMode(ScreenMode mode);

    /**
     * Cycle to the next ScreenMode (wraps around).
     */
    void cycleScreenMode();

    /**
     * Human-readable name of the current ScreenMode (for serial logs).
     */
    const char* screenModeName() const;

    /**
     * Current ScreenMode (used by main loop for serial printouts).
     */
    ScreenMode currentScreenMode() const { return _screenMode; }

    /**
     * Utility: Draw a smooth, filled geometric heart at (cx, cy) with given size.
     */
    void drawHeart(int cx, int cy, int size);

    /**
     * Draw a 3-line status screen (preserved for compatibility/fallback).
     */
    void showStatusScreen(const char* line1, const char* line2, const char* line3);

    /**
     * Draw a centered single line of text at the given y position.
     */
    void showCenteredText(const char* text, int16_t y, uint8_t textSize = 1);

    /**
     * Show a centered popup for `durationMs` milliseconds.
     * Automatically scales font so text is NEVER clipped on the RHS!
     */
    void popup(const char* text, uint32_t durationMs);

    /**
     * Returns true if a popup is currently being shown.
     */
    bool isPopupActive() const;

    /**
     * Force-dismiss the current popup (if any).
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
    // Popup state
    bool     _popupActive;
    uint32_t _popupEndsAtMs;

    // Deskmate mood & state
    DeskmateMood _mood;
    uint32_t _moodStartMs;
    uint32_t _lastFrameMs;
    uint32_t _loveUntilMs;
    bool     _midnightReminder;
    uint8_t  _touchReactionIdx;

    // Screen mode engine
    ScreenMode _screenMode;
    uint32_t   _modeBadgeUntilMs;

    // Eye gaze / saccades
    int8_t   _gazeX;
    int8_t   _gazeY;
    int8_t   _targetGazeX;
    int8_t   _targetGazeY;
    uint32_t _nextGazeChangeMs;

    // Blinking state
    bool     _isBlinking;
    uint32_t _blinkStartMs;
    uint16_t _blinkDurationMs;
    uint32_t _nextBlinkMs;

    // Teardrop animation for LONELY_SAD
    uint32_t _tearStartMs;

    // Floating hearts particle system
    struct HeartParticle {
        int16_t  x;
        int16_t  y;
        float    phase;
        uint8_t  size;
        uint32_t startMs;
        uint16_t lifetimeMs;
        bool     active;
    };
    static constexpr uint8_t MAX_HEARTS = 6;
    HeartParticle _hearts[MAX_HEARTS];

    void initHearts();
    void spawnHeart(int16_t x, int16_t y, uint8_t size, uint16_t lifetimeMs);
    void updateAndDrawHearts(uint32_t now);

    // Sparkle particle system for clock mode
    struct SparkleParticle {
        int16_t  x;
        int16_t  y;
        uint8_t  size;
        uint32_t startMs;
        uint16_t lifetimeMs;
        bool     active;
    };
    static constexpr uint8_t MAX_SPARKLES = 8;
    SparkleParticle _sparkles[MAX_SPARKLES];

    void initSparkles();
    void spawnSparkle(int16_t x, int16_t y, uint8_t size, uint16_t lifetimeMs);
    void updateAndDrawSparkles(uint32_t now);
    void drawSparkle(int16_t x, int16_t y, uint8_t size);
    void drawBatteryIcon(int x, int y, uint8_t pct);

    // Sub-renderers for deskmate states
    void renderDeskmate(uint32_t now);
    void drawNormalEyes(int cx1, int cx2, int cy, int w, int h, uint8_t blinkPct);
    void drawHappyEyes(int cx1, int cx2, int cy);
    void drawHeartEyes(int cx1, int cx2, int cy, uint32_t now);
    void drawSadEyes(int cx1, int cx2, int cy, uint32_t now);
    void drawSleepingEyes(int cx1, int cx2, int cy, uint32_t now);
    void drawCheeks(int cx1, int cx2, int cy);
    void drawMouth(int cx, int cy, DeskmateMood mood);
    void drawMidnightScreen(uint32_t now);

    // Heartbeat state & dynamic pulse
    uint32_t _excitedUntilMs;
    uint16_t getBpm(uint32_t now) const;

    // Sub-renderers for the other screen modes
    void renderModeBadge(uint32_t now);
    void renderClockDate(uint32_t now);
    void renderDailyQuote(uint32_t now);
    void renderQRCode(uint32_t now);
    void renderHeartbeat(uint32_t now);
    void drawPulseWave(int startX, int endX, int centerY, uint32_t now, uint16_t bpm);
    void drawWrappedText(const char* text, int startY, int maxLines, int lineHeight);
};

#endif // AURORA_DISPLAY_H
