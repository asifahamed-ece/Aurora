/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/battery.h
 *
 *  Battery voltage monitor with averaged ADC reads and a simple state machine.
 *
 *  Wiring:
 *    Battery+ ──[R1: 100kΩ]──┬──[R2: 100kΩ]── GND
 *                              └──► GPIO3 (ADC1_CH3)
 *
 *  Theory:
 *    The divider halves the battery voltage before it reaches the ADC.
 *    A fully charged LiPo is ~4.2V → ADC sees ~2.1V (well under 3.3V).
 *    A dead LiPo is ~3.0V → ADC sees ~1.5V.
 *
 *  ESP32-C3 ADC quirks:
 *    - The ADC is non-linear at the very low and very high ends.
 *    - For our 1.5V-2.1V range (battery 3.0V-4.2V), it's reasonably linear.
 *    - We average 8 samples to reduce noise.
 *    - If you want higher accuracy, use a calibrated eFuse value via
 *      esp_adc_cal (we don't, to keep the Arduino framework happy).
 */

#ifndef AURORA_BATTERY_H
#define AURORA_BATTERY_H

#include <Arduino.h>
#include "config.h"

class AuroraBattery {
public:
    /**
     * Initialize the ADC pin. MUST be called in setup().
     */
    void begin();

    /**
     * Take a fresh ADC reading and update internal state.
     * Call this from loop() — it's non-blocking but does take a few µs.
     * For best results, call it no more than once per second.
     */
    void update();

    /**
     * Get the most recent battery voltage in millivolts.
     * Returns 0 if update() has not been called yet.
     */
    uint16_t voltageMillivolts() const { return _voltageMv; }

    /**
     * Get the battery status (OK / LOW / CRITICAL / DEAD) based on the
     * most recent reading. DEAD is only returned here if the voltage is
     * actually below the dead threshold — boot-time check is separate.
     */
    BatStatus status() const { return _status; }

    /**
     * Human-readable status string (for OLED or Serial): "OK" / "LOW" /
     * "CRIT" / "DEAD" / "N/A".
     */
    const char* statusString() const;

    /**
     * Returns true if the most recent reading is below AURORA_BAT_DEAD_MV.
     * Caller should put the device into deep sleep or refuse to boot.
     */
    bool isDead() const { return _voltageMv > 0 && _voltageMv < AURORA_BAT_DEAD_MV; }

private:
    uint16_t  _voltageMv;
    BatStatus _status;
    bool      _initialized;
};

#endif // AURORA_BATTERY_H
