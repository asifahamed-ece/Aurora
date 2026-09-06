/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/block4/wifi_ap.cpp
 *  Block 4: WiFi softAP + optional station NTP bring-up
 */
#include "wifi_ap.h"
#include "config.h"
#include "state.h"
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>

namespace aurora_wifi {

static DNSServer dnsServer;
static bool s_hasStaConfig = false;
static uint32_t s_lastNtpCheckMs = 0;

bool begin() {
    Preferences prefs;
    String staSsid = "";
    String staPass = "";
    if (prefs.begin("aurora", true)) {
        staSsid = prefs.getString("sta_ssid", "");
        staPass = prefs.getString("sta_pass", "");
        prefs.end();
    }

#ifdef AURORA_STA_SSID
    if (staSsid.length() == 0 && strlen(AURORA_STA_SSID) > 0) {
        staSsid = AURORA_STA_SSID;
        staPass = AURORA_STA_PASS;
    }
#endif

    if (staSsid.length() > 0) {
        s_hasStaConfig = true;
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(staSsid.c_str(), staPass.c_str());
        // Configure NTP with Indian Standard Time offset
        configTime(AURORA_TIMEZONE_OFFSET_SEC, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
        DBG_PRINTF("[WIFI] Running in AP+STA mode. Connecting to Station: %s\n", staSsid.c_str());
    } else {
        s_hasStaConfig = false;
        WiFi.mode(WIFI_AP);
    }

    // Disable power saving to avoid TCP timing / heap issues in the AsyncTCP stack
    WiFi.setSleep(WIFI_PS_NONE);

    // Explicitly configure softAP IP, gateway and netmask
    IPAddress local_ip(AURORA_AP_IP);
    IPAddress gateway(AURORA_AP_IP);
    IPAddress subnet(AURORA_AP_NETMASK);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    // Set TX power to 17 dBm to prevent current spikes and voltage dips on USB/breadboard
    WiFi.setTxPower(WIFI_POWER_17dBm);

    WiFi.softAP(AURORA_AP_SSID, AURORA_AP_PASS,
                AURORA_AP_CHANNEL, /*ssid_hidden=*/0, AURORA_AP_MAX_CONN);
    delay(100);

    IPAddress ip = WiFi.softAPIP();
    DBG_PRINTF("[WIFI] AP up: SSID=%s PASS=%s IP=%s ch=%d max=%d\n",
               AURORA_AP_SSID, AURORA_AP_PASS,
               ip.toString().c_str(),
               AURORA_AP_CHANNEL, AURORA_AP_MAX_CONN);

    // Start captive portal DNS server resolving all queries to our local IP
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", local_ip);
    DBG_PRINTLN(F("[WIFI] Captive portal DNS server started on port 53"));

    return true;
}

bool isActive() {
    return (WiFi.getMode() & WIFI_AP) != 0;
}

uint8_t stationCount() {
    return WiFi.softAPgetStationNum();
}

void printStatus() {
    DBG_PRINTF("[WIFI] AP active=%d stations=%d IP=%s (STA connected=%d)\n",
               isActive() ? 1 : 0,
               stationCount(),
               WiFi.softAPIP().toString().c_str(),
               WiFi.status() == WL_CONNECTED ? 1 : 0);
}

void loop() {
    dnsServer.processNextRequest();

    // Check NTP time sync if station is configured
    if (s_hasStaConfig) {
        uint32_t nowMs = millis();
        if (nowMs - s_lastNtpCheckMs >= 3000) {
            s_lastNtpCheckMs = nowMs;
            if (WiFi.status() == WL_CONNECTED) {
                time_t now = time(nullptr);
                if (now > 1700000000 && AuroraState::instance().timeSource() != TimeSyncSource::NTP_SYNC) {
                    AuroraState::instance().setEpoch(now, TimeSyncSource::NTP_SYNC);
                    DBG_PRINTF("[NTP] Synced atomic time from NTP: %u\n", (unsigned)now);
                }
            }
        }
    }
}

} // namespace aurora_wifi
