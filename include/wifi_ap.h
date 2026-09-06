/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/wifi_ap.h
 *  Block 4: WiFi softAP bring-up
 *
 *  Self-contained: no STA mode in Block 4 (Block 3 will add station mode
 *  with NTP sync). The ESP32-C3 just opens a WPA2 AP that Chandni's phone
 *  connects to; the dashboard is served on AURORA_AP_IP.
 */
#ifndef AURORA_BLOCK4_WIFI_AP_H
#define AURORA_BLOCK4_WIFI_AP_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

namespace aurora_wifi {

//  Begin the AP. Returns true on success.
bool begin();

//  Whether the AP is currently up.
bool isActive();

//  Number of connected stations (for diagnostics).
uint8_t stationCount();

//  Pretty-print a status line for Serial.
void printStatus();

//  Pump DNS requests for captive portal support. Call in main loop.
void loop();

} // namespace aurora_wifi

#endif // AURORA_BLOCK4_WIFI_AP_H
