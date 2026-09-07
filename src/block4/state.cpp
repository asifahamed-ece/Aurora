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
      _btnUp(0), _btnSel(0), _btnDown(0), _btnWifi(0),
      _epoch(0), _epochBaseMs(0), _bootCount(0) {}

void AuroraState::setEpoch(uint32_t epoch) {
    _epoch = epoch;
    _epochBaseMs = millis();
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
        _btnUp          = prefs.getUInt("btn_up", 0);
        _btnSel         = prefs.getUInt("btn_sel", 0);
        _btnDown        = prefs.getUInt("btn_down", 0);
        _btnWifi        = prefs.getUInt("btn_wifi", 0);
        if (_touches == 0 && (_btnUp || _btnSel || _btnDown || _btnWifi)) {
            _touches = _btnUp + _btnSel + _btnDown + _btnWifi;
            prefs.putUInt("touches", _touches);
        }
        _bootCount = prefs.getUInt("boot_cnt", 0) + 1;
        prefs.putUInt("boot_cnt", _bootCount);
        prefs.end();
    }
    DBG_PRINTF("[NVS] Loaded keepsake stats: WarmTouches=%u (boot #%u, lastTouchEpoch=%u)\n",
               _touches, _bootCount, _lastTouchEpoch);
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

const char* AuroraState::batteryStatusString() const {
    switch (_batStatus) {
        case BatStatus::BatStatus_OK:       return "OK";
        case BatStatus::BatStatus_LOW:      return "LOW";
        case BatStatus::BatStatus_CRITICAL: return "CRIT";
        case BatStatus::BatStatus_DEAD:     return "DEAD";
    }
    return "?";
}

