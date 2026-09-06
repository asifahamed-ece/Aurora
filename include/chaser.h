/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/chaser.h
 *
 *  Single-LED / LED-strip chaser driver.
 *
 *  Block 1 scope:
 *    - Drive one GPIO pin (AURORA_LED_PIN) with a chaser / "breathing" pattern
 *    - Three animation patterns: solid on, breathing (PWM), and chase
 *    - A single PWM channel — fine for one LED or a MOSFET-driven strip
 *
 *  Why a chaser, not a WS2812B ring?
 *    - Cheaper, simpler wiring, fewer failure modes
 *    - The chaser pattern is a *static repeating pattern* — that's its
 *      personality. The personality lives in the dashboard and the daily
 *      messages, not in fancy LED animations.
 *
 *  Wiring:
 *    GPIO4 ──[220Ω]──►|──► GND         (single LED)
 *    GPIO4 ──[1kΩ]──► MOSFET gate        (MOSFET-driven strip)
 */

#ifndef AURORA_CHASER_H
#define AURORA_CHASER_H

#include <Arduino.h>
#include "config.h"

enum class ChaserPattern : uint8_t {
    SOLID,      // LED on at fixed brightness
    BREATHE,    // LED fades in/out smoothly (sin wave)
    CHASE       // LED blinks on/off in a steady rhythm
};

class AuroraChaser {
public:
    /**
     * Initialize the chaser GPIO. MUST be called in setup().
     * Sets up PWM on the configured pin.
     */
    void begin();

    /**
     * Update the animation. Call every loop() — throttled internally.
     */
    void update();

    /**
     * Force-set the LED on/off state, ignoring the current pattern.
     * Used for transient notifications (e.g., "WiFi activated" — short flash).
     */
    void setOn(bool on);

    /**
     * Set the active pattern. Takes effect on the next update().
     */
    void setPattern(ChaserPattern p) { _pattern = p; }

    /**
     * Set the LED brightness (0-255). For SOLID pattern only;
     * BREATHE and CHASE ignore this and use their own envelope.
     */
    void setBrightness(uint8_t b) { _solidBrightness = b; }

    /**
     * Get the current pattern.
     */
    ChaserPattern pattern() const { return _pattern; }

private:
    ChaserPattern _pattern;
    uint8_t       _solidBrightness;
    uint32_t      _lastUpdateMs;
    uint32_t      _animStartMs;     // for breathing phase calculation
};

#endif // AURORA_CHASER_H
