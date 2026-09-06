/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block4/state.cpp
 *  Block 4: shared state singleton implementation
 */

#include "state.h"
#include <Preferences.h>
#include <esp_sleep.h>
#include <WiFi.h>

// RTC Slow Memory — survives deep sleep standby and soft resets as long as power is applied
RTC_DATA_ATTR static uint32_t s_rtcMagic = 0;
RTC_DATA_ATTR static uint32_t s_rtcBootCount = 0;
RTC_DATA_ATTR static uint32_t s_rtcLastEpoch = 0;
RTC_DATA_ATTR static uint8_t  s_rtcSource = 0;

static void saveKey(const char* key, uint32_t val) {
    Preferences prefs;
    if (prefs.begin("aurora", false)) {
        prefs.putUInt(key, val);
        prefs.end();
    }
}

AuroraState& AuroraState::instance() {
    static AuroraState s;
    return s;
}

AuroraState::AuroraState()
    : _batMv(0), _batPct(0), _batStatus(BatStatus::BatStatus_OK),
      _wifiOn(false), _ledOn(true),
      _touches(0), _lastTouchMs(0), _lastTouchEpoch(0), _pendingTouch(false),
      _bootCount(0), _timeSource(TimeSyncSource::NONE),
      _lastNvsEpochSaveMs(0) {}

uint32_t AuroraState::rtcBootCount() const {
    return s_rtcBootCount;
}

void AuroraState::begin() {
    _lastTouchMs = millis();
    _lastNvsEpochSaveMs = millis();

    Preferences prefs;
    if (prefs.begin("aurora", false)) {
        _touches        = prefs.getUInt("touches", 0);
        _lastTouchEpoch = prefs.getUInt("last_touch", 0);
        _bootCount      = prefs.getUInt("boot_cnt", 0) + 1;
        prefs.putUInt("boot_cnt", _bootCount);
        prefs.end();
    }
    DBG_PRINTF("[NVS] Keepsake WarmTouches=%u (boot #%u, lastTouchEpoch=%u)\n",
               _touches, _bootCount, _lastTouchEpoch);
}

void AuroraState::initTime(uint32_t compileEpochFallback) {
    s_rtcBootCount++;

    // 1. Check if the internal ESP32 Hardware RTC is already running and valid
    // (e.g. woke from Deep Sleep standby or software reset)
    time_t now = time(nullptr);
    if (s_rtcMagic == AURORA_RTC_MAGIC && now > 1700000000) {
        _timeSource = (s_rtcSource != 0) ? (TimeSyncSource)s_rtcSource : TimeSyncSource::RTC_HARDWARE;
        DBG_PRINTF("[RTC] Internal Hardware RTC active across wake/reset! epoch=%u (source=%s, rtcBoot#%u)\n",
                   (unsigned)now, timeSourceString(), (unsigned)s_rtcBootCount);
        return;
    }

    // 2. Cold boot: check NVS flash backup
    Preferences prefs;
    uint32_t nvsEpoch = 0;
    if (prefs.begin("aurora", true)) {
        nvsEpoch = prefs.getUInt("last_epoch", 0);
        prefs.end();
    }

    if (nvsEpoch > 1700000000) {
        struct timeval tv = { .tv_sec = (time_t)nvsEpoch, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        s_rtcMagic     = AURORA_RTC_MAGIC;
        s_rtcLastEpoch = nvsEpoch;
        s_rtcSource    = (uint8_t)TimeSyncSource::NVS_BACKUP;
        _timeSource    = TimeSyncSource::NVS_BACKUP;
        DBG_PRINTF("[RTC] Restored last known epoch from NVS flash backup: %u\n", (unsigned)nvsEpoch);
        return;
    }

    // 3. Last fallback: compile time
    struct timeval tv = { .tv_sec = (time_t)compileEpochFallback, .tv_usec = 0 };
    settimeofday(&tv, nullptr);
    s_rtcMagic     = AURORA_RTC_MAGIC;
    s_rtcLastEpoch = compileEpochFallback;
    s_rtcSource    = (uint8_t)TimeSyncSource::COMPILE_TIME;
    _timeSource    = TimeSyncSource::COMPILE_TIME;
    DBG_PRINTF("[RTC] Initialized with compile-time fallback: %u\n", (unsigned)compileEpochFallback);
}

void AuroraState::setEpoch(uint32_t epoch, TimeSyncSource source) {
    if (epoch < 1700000000) return;

    struct timeval tv = { .tv_sec = (time_t)epoch, .tv_usec = 0 };
    settimeofday(&tv, nullptr);

    s_rtcMagic     = AURORA_RTC_MAGIC;
    s_rtcLastEpoch = epoch;
    s_rtcSource    = (uint8_t)source;
    _timeSource    = source;

    saveKey("last_epoch", epoch);
    _lastNvsEpochSaveMs = millis();
    DBG_PRINTF("[RTC] Epoch updated: %u (source: %s)\n", (unsigned)epoch, timeSourceString());
}

uint32_t AuroraState::epoch() const {
    time_t now = time(nullptr);
    return (uint32_t)now;
}

uint32_t AuroraState::localEpoch() const {
    uint32_t ep = epoch();
    if (ep == 0) return 0;
    return ep + AURORA_TIMEZONE_OFFSET_SEC;
}

void AuroraState::savePeriodicEpoch(bool force) {
    uint32_t nowMs = millis();
    if (force || (nowMs - _lastNvsEpochSaveMs >= 60000)) {
        uint32_t cur = epoch();
        if (cur > 1700000000) {
            saveKey("last_epoch", cur);
            s_rtcLastEpoch = cur;
            _lastNvsEpochSaveMs = nowMs;
        }
    }
}

const char* AuroraState::timeSourceString() const {
    switch (_timeSource) {
        case TimeSyncSource::RTC_HARDWARE: return "RTC_Hardware";
        case TimeSyncSource::NVS_BACKUP:   return "NVS_Backup";
        case TimeSyncSource::BROWSER_SYNC: return "Phone_Browser";
        case TimeSyncSource::NTP_SYNC:     return "NTP_Atomic";
        case TimeSyncSource::COMPILE_TIME: return "Compile_Time";
        case TimeSyncSource::NONE:         return "Unset";
    }
    return "?";
}

void AuroraState::enterSleep() {
    DBG_PRINTLN(F("[PWR] Power switch toggled OFF. Sleeping with RTC active in backend..."));
    savePeriodicEpoch(true);

    // Shut down WiFi cleanly
    bool wasWifiOn = _wifiOn;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(40);

    // Enable GPIO Wakeup on ESP32-C3:
    // 1. Button 0 (GPIO0): Touch button wakes on press (LOW)
    // 2. Power Switch (GPIO10): Wakes when switch is flipped back ON (HIGH)
    gpio_wakeup_enable((gpio_num_t)AURORA_BTN_TOUCH_PIN, GPIO_INTR_LOW_LEVEL);

#ifdef AURORA_SLEEP_SWITCH_PIN
    if (AURORA_SLEEP_SWITCH_PIN >= 0) {
        gpio_wakeup_enable((gpio_num_t)AURORA_SLEEP_SWITCH_PIN, GPIO_INTR_HIGH_LEVEL);
    }
#endif

    esp_sleep_enable_gpio_wakeup();

    // Sleep with CPU halted and peripherals powered down (~130uA).
    // Internal hardware RTC continues ticking accurately throughout sleep!
    esp_light_sleep_start();

    DBG_PRINTLN(F("[PWR] Power switch toggled ON! Waking up..."));
    delay(50);
    if (wasWifiOn) {
        WiFi.mode(WIFI_AP);
    }
}

void AuroraState::bumpTouches() {
    _touches++;
    _lastTouchMs = millis();
    _lastTouchEpoch = epoch();
    _pendingTouch = true;
    saveKey("touches", _touches);
    if (_lastTouchEpoch > 0) {
        saveKey("last_touch", _lastTouchEpoch);
    }
}

void AuroraState::setBattery(uint16_t mv, uint8_t pct, BatStatus s) {
    _batMv     = mv;
    _batPct    = pct;
    _batStatus = s;
}

const char* AuroraState::batteryStatusString() const {
    switch (_batStatus) {
        case BatStatus::BatStatus_OK:       return "OK";
        case BatStatus::BatStatus_LOW:      return "LOW";
        case BatStatus::BatStatus_CRITICAL: return "CRIT";
        case BatStatus::BatStatus_DEAD:     return "DEAD";
    }
    return "?";
}
