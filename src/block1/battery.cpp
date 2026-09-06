/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/battery.cpp
 */

#include "battery.h"

void AuroraBattery::begin() {
    analogReadResolution(12);   // ESP32-C3 ADC is 12-bit (0-4095)
    _voltageMv = 0;
    _status = BatStatus::BatStatus_OK;
    _initialized = false;

    DBG_PRINTF("[BAT] begin() OK — ADC on GPIO%d, divider ratio %.1f\n",
               AURORA_BAT_ADC_PIN, AURORA_BAT_DIVIDER_RATIO);
}

void AuroraBattery::update() {
    // Average N samples for noise reduction.
    uint32_t sum = 0;
    for (uint8_t i = 0; i < AURORA_BAT_SAMPLES; i++) {
        sum += analogRead(AURORA_BAT_ADC_PIN);
    }
    uint16_t rawAvg = (uint16_t)(sum / AURORA_BAT_SAMPLES);

    // Convert raw 12-bit (0-4095) to millivolts at the ADC pin.
    // Then multiply by the divider ratio to get battery voltage.
    float adcMv = (float)rawAvg * (float)AURORA_BAT_ADC_MAX_MV / 4095.0f;
    float batMv = adcMv * AURORA_BAT_DIVIDER_RATIO;

    _voltageMv = (uint16_t)batMv;
    _initialized = true;

    // Classify the status based on thresholds.
    if (_voltageMv >= AURORA_BAT_OK_MV) {
        _status = BatStatus::BatStatus_OK;
    } else if (_voltageMv >= AURORA_BAT_LOW_MV) {
        _status = BatStatus::BatStatus_LOW;
    } else if (_voltageMv >= AURORA_BAT_CRITICAL_MV) {
        _status = BatStatus::BatStatus_CRITICAL;
    } else {
        _status = BatStatus::BatStatus_DEAD;
    }
}

const char* AuroraBattery::statusString() const {
    if (!_initialized) return "N/A";
    switch (_status) {
        case BatStatus::BatStatus_OK:       return "OK";
        case BatStatus::BatStatus_LOW:      return "LOW";
        case BatStatus::BatStatus_CRITICAL: return "CRIT";
        case BatStatus::BatStatus_DEAD:     return "DEAD";
    }
    return "?";
}
