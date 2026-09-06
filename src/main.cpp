/**
 *  Aurora -- Birthday Gift Firmware
 *  File: src/main.cpp
 *
 *  Unified build: Block 1 (OLED + LED + buttons + battery) + Block 4 (WiFi AP + dashboard)
 *
 *  This is the final merged firmware. It combines:
 *    - Block 1: OLED status screen, LED breathing chaser, 4-button input, battery monitor
 *    - Block 4: WiFi AP, AsyncWebServer, AsyncWebSocket, LittleFS dashboard
 *
 *  The OLED shows the status screen when no browser is connected to the AP.
 *  Once a client connects, the dashboard takes over and the OLED continues
 *  showing battery alerts as popups. LittleFS serves the dashboard files.
 *
 *  Build: pio run
 *  Flash: pio run -t upload
 *  LittleFS: pio run -t uploadfs
 */

#include <Arduino.h>

#include "config.h"
#include "state.h"
#include "wifi_ap.h"
#include "web_server.h"
#include "clock.h"

// Block 1 hardware classes
#include "display.h"
#include "buttons.h"
#include "chaser.h"
#include "battery.h"

// Hardware singletons
AuroraDisplay display;
AuroraButtons buttons;
AuroraChaser  chaser;
AuroraBattery battery;

// ----------------------------------------------------------------------------
//  Compile-time epoch (replaced by NTP in Block 2)
// ----------------------------------------------------------------------------
static uint32_t compileEpoch() {
    const char* d = __DATE__;
    const char* t = __TIME__;
    const char* mnames = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mstr[4] = {0};
    strncpy(mstr, d, 3);
    int month = 1;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(mnames + i*3, mstr, 3) == 0) { month = i + 1; break; }
    }
    int day  = atoi(d + 4);
    int year = atoi(d + 7);
    int hour = atoi(t);
    int min  = atoi(t + 3);
    int sec  = atoi(t + 6);
    struct tm tm = {};
    tm.tm_year  = year - 1900;
    tm.tm_mon   = month - 1;
    tm.tm_mday  = day;
    tm.tm_hour  = hour;
    tm.tm_min   = min;
    tm.tm_sec   = sec;
    // mktime() uses local TZ; NTP sync in Block 2 will fix this.
    return (uint32_t)mktime(&tm);
}

// ----------------------------------------------------------------------------
//  OLED status screen -- shows battery + button counts + last press
// ----------------------------------------------------------------------------
static uint32_t s_lastScreenRefreshMs = 0;

static void refreshStatusScreen(bool force = false) {
    uint32_t now = millis();
    if (!force && (now - s_lastScreenRefreshMs) < (uint32_t)AURORA_OLED_REFRESH_MS) return;
    s_lastScreenRefreshMs = now;

    char line1[24];
    char line2[24];
    char line3[24];

    uint32_t epoch = AuroraState::instance().epoch();
    if (aurora_clock::isBirthday(epoch)) {
        uint8_t age = aurora_clock::birthdayAge(epoch);
        snprintf(line1, sizeof(line1), "* Happy B-Day! *");
        snprintf(line2, sizeof(line2), "Chandni turns %u!", (unsigned)age);
        snprintf(line3, sizeof(line3), "<3 Chapter %u <3", (unsigned)age);
    } else {
        // Line 1: project name + battery
        snprintf(line1, sizeof(line1), "%s for Chandni", AURORA_PROJECT_NAME);

        // Line 2: Warm Touches count (persisted keepsake counter)
        snprintf(line2, sizeof(line2), "Touches: %lu",
                 (unsigned long)AuroraState::instance().touches());

        // Line 3: WiFi AP status
        if (AuroraState::instance().wifiOn()) {
            snprintf(line3, sizeof(line3), "AP: %s (%d)",
                     AURORA_AP_SSID, aurora_wifi::stationCount());
        } else {
            snprintf(line3, sizeof(line3), "AP: OFF");
        }
    }

    display.showStatusScreen(line1, line2, line3);
}

// ----------------------------------------------------------------------------
//  setup()
// ----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);
    DBG_PRINTLN();
    DBG_PRINTF("=== %s %s -- %s ===\n",
               AURORA_PROJECT_NAME, AURORA_VERSION, AURORA_BUILD_BLOCK);
    DBG_PRINTF("=== For: %s ===\n", AURORA_DEDICATEE);
    DBG_PRINTF("=== Board: %s ===\n", AURORA_BOARD);
    DBG_PRINTLN();

    // --- Display (OLED) ---
    if (!display.begin()) {
        DBG_PRINTLN(F("[FATAL] OLED failed. Check wiring:"));
        DBG_PRINTLN(F("        SDA=GPIO8, SCL=GPIO9, VCC=3.3V, GND=GND"));
        DBG_PRINTLN(F("        If horizontal lines: set AURORA_OLED_DRIVER=1 in config.h for SH1106"));
        DBG_PRINTLN(F("        Also try changing AURORA_OLED_ADDR from 0x3D to 0x3C"));
        while (true) { delay(1000); }
    }
    display.showBootScreen();
    delay(1500);

    // --- LED chaser ---
    chaser.begin();
    chaser.setPattern(ChaserPattern::BREATHE);

    // --- Buttons ---
    buttons.begin();

    // --- Battery ---
    battery.begin();
    battery.update();
    DBG_PRINTF("[BAT] initial: %dmV (%s)\n",
               battery.voltageMillivolts(), battery.statusString());

    // --- State: restore saved keepsake counters from NVS flash ---
    AuroraState::instance().begin();

    // --- State: initial battery reading ---
    AuroraState::instance().setBattery(
        battery.voltageMillivolts(),
        100,  // rough -- percentage is less critical on the OLED
        battery.status()
    );

    // --- WiFi AP ---
    if (!aurora_wifi::begin()) {
        DBG_PRINTLN(F("[FATAL] WiFi AP failed to start"));
        while (true) { delay(1000); }
    }
    AuroraState::instance().setWifiOn(true);
    aurora_wifi::printStatus();

    // --- Web server + LittleFS dashboard ---
    if (!aurora_web::begin()) {
        DBG_PRINTLN(F("[FATAL] LittleFS / web server failed to start"));
        while (true) { delay(1000); }
    }

    // --- Clock: seed with compile-time epoch ---
    AuroraState::instance().setEpoch(compileEpoch());
    DBG_PRINTF("[CLK] epoch=%u (%s)\n",
               (unsigned)AuroraState::instance().epoch(), __DATE__);

    // --- Show initial status screen on OLED ---
    refreshStatusScreen(true);

    DBG_PRINTLN();
    DBG_PRINTF("[SYS] SSID: %s  pass: %s  http://%u.%u.%u.%u/\n",
               AURORA_AP_SSID, AURORA_AP_PASS,
               AURORA_AP_IP);
    DBG_PRINTLN(F("[SYS] Setup complete. Entering main loop."));
    DBG_PRINTLN();
}

// ----------------------------------------------------------------------------
//  loop()
// ----------------------------------------------------------------------------
static uint32_t s_lastBatMs    = 0;
static uint32_t s_lastHeartMs  = 0;
static uint32_t s_bootMs       = 0;

void loop() {
    uint32_t now = millis();
    if (s_bootMs == 0) s_bootMs = now;

    // --- Pump DNS captive portal & WebSocket broadcast timers ---
    aurora_wifi::loop();
    aurora_web::loop();

    // --- Physical touch / button input ---
    uint8_t events = buttons.update();
    if (events != BTN_NONE) {
        bool changed = false;

        // Any physical button press counts as a Warm Touch!
        if (events & (BTN_UP | BTN_SELECT | BTN_DOWN)) {
            AuroraState::instance().bumpTouches();
            changed = true;
            display.popup("  <3 Warm Touch! <3", 2000);
            DBG_PRINTLN(F("[TOUCH] physical touch recorded"));
        }

        // WiFi button toggles AP
        if (events & BTN_WIFI) {
            AuroraState::instance().bumpTouches();
            AuroraState::instance().setWifiOn(!AuroraState::instance().wifiOn());
            changed = true;
            display.popup(AuroraState::instance().wifiOn() ? "WiFi ON" : "WiFi OFF",
                          AURORA_POPUP_DURATION_MS);
            DBG_PRINTLN(F("[BTN] WIFI toggle"));
        }

        if (changed) aurora_web::requestImmediatePush();
    }

    // --- LED chaser ---
    chaser.update();

    // --- Battery: sample once per second ---
    if (now - s_lastBatMs >= 1000) {
        s_lastBatMs = now;
        battery.update();
        AuroraState::instance().setBattery(
            battery.voltageMillivolts(),
            100,
            battery.status()
        );

        // Low-battery popups on the OLED
        if (!display.isPopupActive()) {
            BatStatus s = battery.status();
            if (s == BatStatus::BatStatus_LOW) {
                display.popup("Battery LOW", AURORA_POPUP_DURATION_MS);
            } else if (s == BatStatus::BatStatus_CRITICAL) {
                display.popup("Battery CRIT", AURORA_POPUP_DURATION_MS);
            }
        }
    }

    // --- OLED status screen (throttled; hidden when popup is active) ---
    if (!display.isPopupActive()) {
        refreshStatusScreen();
    }

    // --- Heartbeat ---
    if (now - s_lastHeartMs >= (uint32_t)AURORA_HEARTBEAT_MS) {
        s_lastHeartMs = now;
        uint32_t uptime = (now - s_bootMs) / 1000;
        DBG_PRINTF("[SYS] heartbeat -- uptime=%lus freeHeap=%u bat=%dmV(%s)\n",
                   (unsigned long)uptime,
                   (unsigned)ESP.getFreeHeap(),
                   battery.voltageMillivolts(),
                   battery.statusString());
    }

    // Yield to the AsyncTCP stack (no delay() so WS server stays responsive)
    yield();
}
