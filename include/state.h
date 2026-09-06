/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/state.h
 *  Block 4: shared state singleton
 *
 *  Holds values pushed to dashboard and display:
 *    - Keepsake Warm Touches (persisted in NVS)
 *    - Internal ESP32 Hardware RTC (POSIX settimeofday / time(nullptr))
 *    - Deep Sleep standby power management (~5uA)
 *    - Multi-source time synchronization (RTC memory, NVS flash, browser sync, NTP)
 */
#ifndef AURORA_BLOCK4_STATE_H
#define AURORA_BLOCK4_STATE_H

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>
#include "config.h"

enum class TimeSyncSource : uint8_t {
    NONE = 0,
    COMPILE_TIME,
    NVS_BACKUP,
    RTC_HARDWARE,
    BROWSER_SYNC,
    NTP_SYNC
};

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

    // Two-way touch animation signaling
    bool     hasPendingTouch() const { return _pendingTouch; }
    void     clearPendingTouch()     { _pendingTouch = false; }

    // Telemetry button count aliases
    uint32_t btnUp()   const { return _touches; }
    uint32_t btnSel()  const { return _touches; }
    uint32_t btnDown() const { return _touches; }
    uint32_t btnWifi() const { return _touches; }

    // ---- Hardware RTC & Time Synchronization ----
    void           initTime(uint32_t compileEpochFallback);
    void           setEpoch(uint32_t epoch, TimeSyncSource source = TimeSyncSource::BROWSER_SYNC);
    uint32_t       epoch() const;
    uint32_t       localEpoch() const;
    TimeSyncSource timeSource() const { return _timeSource; }
    const char*    timeSourceString() const;
    void           savePeriodicEpoch(bool force = false);

    // ---- Deep Sleep Standby (~5uA, RTC active) ----
    void           enterDeepSleep();

    // ---- Diagnostics ----
    uint32_t bootCount()       const { return _bootCount; }
    uint32_t rtcBootCount()    const;

private:
    AuroraState();

    uint16_t       _batMv;
    uint8_t        _batPct;
    BatStatus      _batStatus;
    bool           _wifiOn;
    bool           _ledOn;
    uint32_t       _touches;
    uint32_t       _lastTouchMs;
    uint32_t       _lastTouchEpoch;
    bool           _pendingTouch;
    uint32_t       _bootCount;
    TimeSyncSource _timeSource;
    uint32_t       _lastNvsEpochSaveMs;
};

#endif // AURORA_BLOCK4_STATE_H
