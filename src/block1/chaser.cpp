/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/chaser.cpp
 *
 *  LED chaser implementation.
 *  Uses Arduino's analogWrite() (PWM) for brightness control on the
 *  configured GPIO. ESP32-C3 supports PWM on all GPIO pins via LEDC.
 */

#include "chaser.h"

// Map our GPIO to an LEDC channel. ESP32-C3 has 6 LEDC channels (0-5).
// We use channel 0 for the chaser.
#define CHASER_LEDC_CHANNEL  0
#define CHASER_LEDC_FREQ_HZ  5000
#define CHASER_LEDC_RES_BITS 8   // 8-bit resolution (0-255)

void AuroraChaser::begin() {
    _pattern = ChaserPattern::BREATHE;
    _solidBrightness = 128;
    _periodMs = AURORA_LED_CHASER_PERIOD_MS;
    _lastUpdateMs = 0;
    _animStartMs = millis();

    // Setup LEDC PWM channel
    ledcSetup(CHASER_LEDC_CHANNEL, CHASER_LEDC_FREQ_HZ, CHASER_LEDC_RES_BITS);
    ledcAttachPin(AURORA_LED_PIN, CHASER_LEDC_CHANNEL);

    // Start with LED off
    ledcWrite(CHASER_LEDC_CHANNEL, 0);

    DBG_PRINTF("[CHASER] begin() OK — pin GPIO%d, channel %d, %dHz @ %d-bit (period %dms)\n",
               AURORA_LED_PIN, CHASER_LEDC_CHANNEL,
               CHASER_LEDC_FREQ_HZ, CHASER_LEDC_RES_BITS, _periodMs);
}

void AuroraChaser::setOn(bool on) {
    if (on) {
        ledcWrite(CHASER_LEDC_CHANNEL, _solidBrightness);
    } else {
        ledcWrite(CHASER_LEDC_CHANNEL, 0);
    }
}

void AuroraChaser::update() {
    uint32_t now = millis();
    uint16_t period = _periodMs > 0 ? _periodMs : AURORA_LED_CHASER_PERIOD_MS;

    switch (_pattern) {
        case ChaserPattern::SOLID: {
            ledcWrite(CHASER_LEDC_CHANNEL, _solidBrightness);
            break;
        }

        case ChaserPattern::BREATHE: {
            // Smooth breathing using sin().
            uint32_t t = (now - _animStartMs) % (uint32_t)period;
            float phase = (float)t / (float)period;                         // 0.0 - 1.0
            float wave = sinf(phase * 2.0f * 3.14159265f);                  // -1.0 - 1.0
            // Map wave from [-1, 1] to [0, 255] (so it never fully goes off — softer).
            uint8_t brightness = (uint8_t)((wave * 0.5f + 0.5f) * 255.0f);
            ledcWrite(CHASER_LEDC_CHANNEL, brightness);
            break;
        }

        case ChaserPattern::CHASE: {
            uint32_t halfPeriod = (uint32_t)period / 2;
            bool on = ((now - _animStartMs) / halfPeriod) % 2 == 0;
            ledcWrite(CHASER_LEDC_CHANNEL, on ? 255 : 0);
            break;
        }
    }
}
