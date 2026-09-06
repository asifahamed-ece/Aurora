/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/wifi_ap.cpp
 *  Block 4: WiFi softAP bring-up
 */
#include "wifi_ap.h"
#include "config.h"
#include <DNSServer.h>

namespace aurora_wifi {

static DNSServer dnsServer;

bool begin() {
    WiFi.mode(WIFI_AP);
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

