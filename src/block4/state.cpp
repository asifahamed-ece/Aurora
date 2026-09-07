#include "state.h"
#include <Preferences.h>

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
      _touches(0), _lastTouchMs(0), _lastTouchEpoch(0),
      _pendingTouchReaction(false),
      _epoch(0), _epochBaseMs(0), _bootCount(0), _midnightAckDoy(0) {}

void AuroraState::setEpoch(uint32_t epoch) {
    if (epoch == _epoch) return;  // no change → no NVS wear
    _epoch = epoch;
    _epochBaseMs = millis();
    // Persist so the next boot doesn't restart from a stale compile-time seed
    saveKey("epoch", _epoch);
}

uint32_t AuroraState::epoch() const {
    if (_epoch == 0) return 0;
    return _epoch + ((millis() - _epochBaseMs) / 1000);
}

void AuroraState::begin() {
    _lastTouchMs = millis();
    Preferences prefs;
    if (prefs.begin("aurora", false)) {
        _touches        = prefs.getUInt("touches", 0);
        _lastTouchEpoch = prefs.getUInt("last_touch", 0);
        uint32_t btnUp   = prefs.getUInt("btn_up", 0);
        uint32_t btnSel  = prefs.getUInt("btn_sel", 0);
        uint32_t btnDown = prefs.getUInt("btn_down", 0);
        uint32_t btnWifi = prefs.getUInt("btn_wifi", 0);
        if (_touches == 0 && (btnUp || btnSel || btnDown || btnWifi)) {
            _touches = btnUp + btnSel + btnDown + btnWifi;
            prefs.putUInt("touches", _touches);
        }
        _bootCount = prefs.getUInt("boot_cnt", 0) + 1;
        prefs.putUInt("boot_cnt", _bootCount);
        _midnightAckDoy = prefs.getUShort("mid_ack", 0);
        // Restore last-known epoch if any (post 2024 sanity check so a wiped
        // NVS doesn't seed us with 0, which would brick the date math).
        uint32_t savedEpoch = prefs.getUInt("epoch", 0);
        if (savedEpoch > 1700000000UL) {
            _epoch      = savedEpoch;
            _epochBaseMs = millis();
        }
        prefs.end();
    }
    DBG_PRINTF("[NVS] Loaded keepsake stats: WarmTouches=%u (boot #%u, lastTouchEpoch=%u epoch=%u)\n",
               _touches, _bootCount, _lastTouchEpoch, (unsigned)_epoch);
}

void AuroraState::bumpTouches() {
    _touches++;
    _lastTouchMs = millis();
    _lastTouchEpoch = epoch();
    _pendingTouchReaction = true;
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

void AuroraState::setMidnightAckDoy(uint16_t doy) {
    _midnightAckDoy = doy;
    Preferences prefs;
    if (prefs.begin("aurora", false)) {
        prefs.putUShort("mid_ack", doy);
        prefs.end();
    }
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

