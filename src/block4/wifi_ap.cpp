/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/wifi_ap.cpp
 *  Block 4: WiFi softAP bring-up
 */
#include "wifi_ap.h"
#include "config.h"
#include <DNSServer.h>
#include <esp_wifi.h>

namespace aurora_wifi {

static DNSServer dnsServer;

bool begin() {
    // 1. Ensure clean AP mode
    WiFi.mode(WIFI_AP);

    // 2. Disable all WiFi power savings so the RF transceiver stays active continuously
    WiFi.setSleep(WIFI_PS_NONE);

    // 3. Configure IP addressing before launching the AP
    IPAddress local_ip(AURORA_AP_IP);
    IPAddress gateway(AURORA_AP_IP);
    IPAddress subnet(AURORA_AP_NETMASK);
    WiFi.softAPConfig(local_ip, gateway, subnet);

    // 4. Start SoftAP
    bool ok = WiFi.softAP(AURORA_AP_SSID, AURORA_AP_PASS,
                          AURORA_AP_CHANNEL, /*ssid_hidden=*/0, AURORA_AP_MAX_CONN);
    if (!ok) {
        DBG_PRINTLN(F("[WIFI] softAP() failed to start"));
        return false;
    }

    // 5. Tune ESP-IDF AP settings for rock-solid beaconing & mobile discovery:
    //    - Beacon interval = 100ms (standard 100 TU for fast discovery by phones)
    //    - max_connection = 4
    //    - authmode = WPA2_PSK
    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_AP, &conf) == ESP_OK) {
        conf.ap.beacon_interval = 100; // 100ms beacon broadcast
        conf.ap.max_connection = AURORA_AP_MAX_CONN;
        conf.ap.channel = AURORA_AP_CHANNEL;
        conf.ap.authmode = WIFI_AUTH_WPA2_PSK;
        esp_wifi_set_config(WIFI_IF_AP, &conf);
    }

    // 6. Set TX Power to maximum 19.5dBm (or 19dBm) for strong, clear signal
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    IPAddress ip = WiFi.softAPIP();
    DBG_PRINTF("[WIFI] AP up: SSID=%s PASS=%s IP=%s ch=%d max=%d\n",
               AURORA_AP_SSID, AURORA_AP_PASS,
               ip.toString().c_str(),
               AURORA_AP_CHANNEL, AURORA_AP_MAX_CONN);

    // 7. Start captive portal DNS server resolving all queries to our local IP
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", local_ip);
    DBG_PRINTLN(F("[WIFI] Captive portal DNS server started on port 53"));

    return true;
}

bool isActive() {
    return WiFi.getMode() & WIFI_AP;
}

uint8_t stationCount() {
    return WiFi.softAPgetStationNum();
}

void printStatus() {
    DBG_PRINTF("[WIFI] AP active=%d stations=%d IP=%s\n",
               isActive() ? 1 : 0,
               stationCount(),
               WiFi.softAPIP().toString().c_str());
}

void loop() {
    dnsServer.processNextRequest();
}

} // namespace aurora_wifi

