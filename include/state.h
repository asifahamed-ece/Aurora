/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/state.h
 *  Block 4: shared state singleton
 *
 *  Holds the values that are pushed to the dashboard. Updated by the input
 *  loop (battery, buttons) and read by the WebSocket broadcast loop.
 *  Thread-safety note: ESP32-C3 is single-core under the Arduino framework
 *  with a FreeRTOS cooperative scheduler — no locks needed for our access
 *  pattern (update from loop(), read from loop()).
 */
#ifndef AURORA_BLOCK4_STATE_H
#define AURORA_BLOCK4_STATE_H

#include <Arduino.h>
#include "config.h"

class AuroraState {
public:
    static AuroraState& instance();

    // ---- Initialize (loads persistent counters from NVS flash) ----
    void begin();

    // ---- Battery ----
    void    setBattery(uint16_t mv, uint8_t pct, BatStatus s);
    uint16_t batteryMv()    const { return _batMv; }
    uint8_t  batteryPct()   const { return _batPct; }
    BatStatus batteryStatus() const { return _batStatus; }
    const char* batteryStatusString() const;

    // ---- WiFi / LED ----
    void    setWifiOn(bool on)        { _wifiOn = on; }
    bool    wifiOn()            const { return _wifiOn; }
    void    setLedOn(bool on)         { _ledOn = on; }
    bool    ledOn()             const { return _ledOn; }

    // ---- Keepsake Touches (persisted to NVS flash) ----
    void     bumpTouches();
    uint32_t touches() const { return _touches; }
    uint32_t lastTouchMs() const { return _lastTouchMs; }
    uint32_t lastTouchEpoch() const { return _lastTouchEpoch; }
    uint32_t msSinceLastTouch() const { return millis() - _lastTouchMs; }
    float    hoursSinceLastTouch() const { return (float)(millis() - _lastTouchMs) / 3600000.0f; }
    bool     consumePendingTouch() {
        if (_pendingTouchReaction) {
            _pendingTouchReaction = false;
            return true;
        }
        return false;
    }

    // ---- Time (dynamically advances with millis) ----
    void     setEpoch(uint32_t epoch);
    uint32_t epoch() const;

    // ---- Boot count (for diagnostics) ----
    uint32_t bootCount()       const { return _bootCount; }
    void     incBootCount()         { _bootCount++; }

    // ---- Midnight Ack DOY (persisted to NVS) ----
    uint16_t midnightAckDoy() const { return _midnightAckDoy; }
    void     setMidnightAckDoy(uint16_t doy);

private:
    AuroraState();

    uint16_t  _batMv;
    uint8_t   _batPct;
    BatStatus _batStatus;
    bool      _wifiOn;
    bool      _ledOn;
    uint32_t  _touches;
    uint32_t  _lastTouchMs;
    uint32_t  _lastTouchEpoch;
    bool      _pendingTouchReaction;
    uint32_t  _epoch;
    uint32_t  _epochBaseMs;
    uint32_t  _bootCount;
    uint16_t  _midnightAckDoy;
};

#endif // AURORA_BLOCK4_STATE_H
