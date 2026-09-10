/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/web_server.cpp
 *  Block 4: AsyncWebServer + AsyncWebSocket + LittleFS
 *
 *  Dependencies (added in platformio.ini [env:esp32c3_dashboard]):
 *    - esphome/ESPAsyncWebServer-esphome @ ^3.2.0
 *    - me-no-dev/AsyncTCP @ ^1.1.1
 *    - bblanchon/ArduinoJson @ ^6.21.0
 */
#include "web_server.h"
#include "config.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

#if AURORA_OTA_ENABLED
#include <Update.h>
#endif

#include "state.h"
#include "clock.h"
#include "messages.h"

namespace aurora_web {

// ----------------------------------------------------------------------------
//  Module state
// ----------------------------------------------------------------------------
static AsyncWebServer      server(80);
static AsyncWebSocket      ws("/ws");
static bool                g_fsReady   = false;
static uint32_t            g_lastPush  = 0;
static bool                g_pushNow   = false;

// ----------------------------------------------------------------------------
//  JSON state serialisation
// ----------------------------------------------------------------------------
static size_t buildStateJson(char* out, size_t outSize) {
    auto& st = AuroraState::instance();

    StaticJsonDocument<512> doc;
    doc["type"] = "state";

    char timeBuf[24], dateBuf[16];
    uint32_t epoch = st.epoch() ? st.epoch() : (millis() / 1000);
    aurora_clock::formatTime(timeBuf, sizeof(timeBuf), epoch);
    aurora_clock::formatDate(dateBuf, sizeof(dateBuf), epoch);
    doc["time"] = timeBuf;
    doc["date"] = dateBuf;

    uint16_t doy = aurora_clock::dayOfYear(epoch);
    doc["doy"]     = doy;
    doc["msg_idx"] = aurora_messages::indexForDoy(doy);

    // Battery sub-object
    JsonObject bat = doc.createNestedObject("bat");
    bat["pct"]   = st.batteryPct();
    bat["mv"]    = st.batteryMv();
    bat["state"] = st.batteryStatusString();

    doc["wifi"] = st.wifiOn();
    doc["led"]  = st.ledOn() ? "on" : "off";
    doc["touches"] = st.touches();

    size_t n = serializeJson(doc, out, outSize);
    return n;
}

static size_t buildHelloJson(char* out, size_t outSize) {
    StaticJsonDocument<192> doc;
    doc["type"]    = "hello";
    doc["project"] = AURORA_PROJECT_NAME;
    doc["version"] = AURORA_VERSION;
    doc["build"]   = AURORA_BUILD_BLOCK;
    return serializeJson(doc, out, outSize);
}

// ----------------------------------------------------------------------------
//  Push a state frame to every connected WS client
// ----------------------------------------------------------------------------
static void pushStateToAll() {
    if (ws.count() == 0) return;
    char buf[512];
    size_t n = buildStateJson(buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        DBG_PRINTF("[WS] state frame too large: %u\n", (unsigned)n);
        return;
    }
    ws.textAll(buf, n);
}

static void pushHelloTo(AsyncWebSocketClient* c) {
    if (!c) return;
    char buf[192];
    size_t n = buildHelloJson(buf, sizeof(buf));
    if (n > 0) c->text(buf, n);
}

// ----------------------------------------------------------------------------
//  WS event handler
// ----------------------------------------------------------------------------
static void onWsEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg,
                      uint8_t* data,
                      size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            DBG_PRINTF("[WS] client %u connected (total=%u)\n",
                       client->id(), server->count());
            pushHelloTo(client);
            //  Push an immediate state frame so the UI is never blank.
            g_pushNow = true;
            break;

        case WS_EVT_DISCONNECT:
            DBG_PRINTF("[WS] client %u disconnected (total=%u)\n",
                       client->id(), server->count());
            break;

        case WS_EVT_DATA: {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len &&
                info->opcode == WS_TEXT) {
                //  Try to parse the frame. We only reply to {"type":"ping"}.
                StaticJsonDocument<128> doc;
                if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                    const char* t = doc["type"] | "";
                    if (strcmp(t, "ping") == 0) {
                        StaticJsonDocument<64> pong;
                        pong["type"] = "pong";
                        char buf[64];
                        size_t n = serializeJson(pong, buf, sizeof(buf));
                        client->text(buf, n);
                    } else if (strcmp(t, "time_sync") == 0) {
                        uint32_t clientEpoch = doc["epoch"] | 0;
                        if (clientEpoch > 1700000000) {
                            AuroraState::instance().setEpoch(clientEpoch);
                            DBG_PRINTF("[CLK] synced from client: epoch=%u\n", (unsigned)clientEpoch);
                            // A real time_sync means the dashboard is alive
                            // and the clock is now accurate — hide the sync
                            // hint for the rest of this boot session.
                            AuroraState::instance().markTimeSyncedFromClient();
                            g_pushNow = true;
                        }
                    } else if (strcmp(t, "heart_tap") == 0) {
                        AuroraState::instance().bumpTouches();
                        DBG_PRINTLN(F("[TOUCH] heart tap received from phone"));
                        g_pushNow = true;
                    } else if (strcmp(t, "mood") == 0) {
                        const char* m = doc["mood"] | "unknown";
                        DBG_PRINTF("[MOOD] Chandni's mood: %s\n", m);
                    }
                }
            }
            break;
        }

        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

// ----------------------------------------------------------------------------
//  /api/state handler — same JSON, sent over HTTP for first-paint
// ----------------------------------------------------------------------------
static void handleApiState(AsyncWebServerRequest* req) {
    char buf[512];
    size_t n = buildStateJson(buf, sizeof(buf));
    // Pass raw char* directly to avoid an extra String() copy
    AsyncWebServerResponse* r = req->beginResponse(200, "application/json", buf);
    r->addHeader("Cache-Control", "no-store");
    req->send(r);
    (void)n;
}

// ----------------------------------------------------------------------------
//  Captive-portal probe suppression & fallback
// ----------------------------------------------------------------------------
static void handleNotFound(AsyncWebServerRequest* req) {
    //  Only redirect for GETs that look like browser navigations.
    if (req->method() != HTTP_GET) {
        req->send(404);
        return;
    }
    // Use raw char* URL to avoid String() construction
    const char* url = req->url().c_str();
    if (strncmp(url, "/api", 4) == 0 || strncmp(url, "/ws", 3) == 0) {
        req->send(404);
        return;
    }
    // Redirect all unknown paths to dashboard (captive portal fallback)
    DBG_PRINTF("[HTTP] fallback -> / for %s\n", url);
    req->redirect("/");
}

// ----------------------------------------------------------------------------
//  OTA -- web-based firmware + filesystem updates (softAP only)
// ----------------------------------------------------------------------------
#if AURORA_OTA_ENABLED

static File g_uploadFile;
static bool g_otaDenied = false;

//  Token check: accept a "key" multipart form field or "X-Aurora-Key" header.
static bool otaKeyOk(AsyncWebServerRequest* req) {
    const char* key = AURORA_OTA_KEY;
    if (strlen(key) == 0) return true;   // auth disabled for this build
    if (req->hasParam("key", true) &&
        req->getParam("key", true)->value() == key) return true;
    if (req->hasHeader("X-Aurora-Key") &&
        req->header("X-Aurora-Key") == key) return true;
    return false;
}

static const char OTA_INDEX_HTML[] PROGMEM =
    "<!doctype html><html><head><meta charset=utf-8>"
    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
    "<title>Aurora Update</title></head>"
    "<body style=\"font-family:sans-serif;max-width:520px;margin:40px auto;"
    "text-align:center;background:#fff0f5;color:#461628\">"
    "<h2>Aurora — Update</h2>"
    "<form method=post action=/update enctype=multipart/form-data>"
    "<p>Firmware (.bin)</p><input type=file name=firmware required><br><br>"
    "<button>Update Firmware</button></form><hr>"
    "<form method=post action=/upload enctype=multipart/form-data>"
    "<p>Dashboard / content file (index.html, messages.js, images...)</p>"
    "<input type=file name=file required><br><br>"
    "<button>Upload File</button></form></body></html>";

static void handleOtaForm(AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", OTA_INDEX_HTML);
}

//  POST /update -- firmware image. Requires header "X-Aurora-Key".
//  First callback runs when the whole multipart request has been consumed;
//  second callback streams each received chunk into the Update engine.
static void handleOtaUpdate(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* r;
    if (g_otaDenied) {
        r = req->beginResponse(403, "text/plain", "DENIED: bad key");
    } else if (Update.hasError()) {
        char err[96];
        snprintf(err, sizeof(err), "UPDATE FAILED: %s\n", Update.errorString());
        r = req->beginResponse(400, "text/plain", err);
    } else {
        r = req->beginResponse(200, "text/plain", "UPDATE OK - rebooting\n");
    }
    r->addHeader("Connection", "close");
    req->send(r);
    if (!g_otaDenied && !Update.hasError()) {
        delay(1500);          // let the response flush
        ESP.restart();
    }
    g_otaDenied = false;
}

static void handleOtaUpdateUpload(AsyncWebServerRequest* req,
                                  const String& filename, size_t index,
                                  uint8_t* data, size_t len, bool final) {
    if (index == 0) {
        g_otaDenied = false;
        if (!otaKeyOk(req)) {
            g_otaDenied = true;
            DBG_PRINTLN(F("[OTA] update denied (bad key)"));
            return;
        }
        DBG_PRINTF("[OTA] firmware start: %s (%u bytes)\n",
                   filename.c_str(), (unsigned)len);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            return;
        }
    }
    if (g_otaDenied) return;
    if (!Update.hasError() && !Update.write(data, len)) {
        Update.printError(Serial);
    }
    if (final) {
        if (!Update.hasError()) {
            DBG_PRINTLN(F("[OTA] firmware end, verifying..."));
            if (!Update.end(true)) {
                Update.printError(Serial);
            }
        }
    }
}

//  POST /upload -- write a single file into LittleFS. Path-traversal guarded,
//  no subdirectories (dashboard assets all live at the FS root).
static void handleFsUpload(AsyncWebServerRequest* req) {
    AsyncWebServerResponse* r;
    if (g_otaDenied) {
        r = req->beginResponse(403, "text/plain", "DENIED: bad key");
    } else {
        r = req->beginResponse(200, "text/plain", "UPLOAD OK");
    }
    r->addHeader("Connection", "close");
    req->send(r);
    g_otaDenied = false;
}

static void handleFsUploadBody(AsyncWebServerRequest* req,
                               const String& filename, size_t index,
                               uint8_t* data, size_t len, bool final) {
    if (index == 0) {
        g_otaDenied = false;
        if (!otaKeyOk(req)) {
            g_otaDenied = true;
            DBG_PRINTLN(F("[OTA] upload denied (bad key)"));
            return;
        }
        //  Reject traversal attempts and anything that is not a filename.
        if (filename.indexOf("..") >= 0 || filename.startsWith("/") ||
            filename.isEmpty()) {
            g_otaDenied = true;
            DBG_PRINTLN(F("[OTA] upload rejected: bad filename"));
            return;
        }
        g_uploadFile = LittleFS.open("/" + filename, "w");
        if (!g_uploadFile) {
            g_otaDenied = true;
            DBG_PRINTF("[OTA] upload open failed: %s\n", filename.c_str());
            return;
        }
        if (len > 0) g_uploadFile.write(data, len);
        DBG_PRINTF("[OTA] upload start: /%s\n", filename.c_str());
        return;
    }
    if (g_otaDenied) return;
    if (g_uploadFile && len > 0) g_uploadFile.write(data, len);
    if (final) {
        if (g_uploadFile) {
            g_uploadFile.close();
            g_uploadFile = File();
            DBG_PRINTLN(F("[OTA] upload complete"));
        }
    }
}

#endif // AURORA_OTA_ENABLED

// ----------------------------------------------------------------------------
//  Public API
// ----------------------------------------------------------------------------
bool begin() {
    g_fsReady = LittleFS.begin(true /* format on fail */, "/littlefs", 5, "littlefs");
    if (!g_fsReady) {
        DBG_PRINTLN(F("[FS]  LittleFS mount failed"));
        return false;
    }
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    DBG_PRINTF("[FS]  LittleFS mounted: %u / %u bytes used\n",
               (unsigned)used, (unsigned)total);

    //  1. /api/state — JSON snapshot for first-paint (registered BEFORE serveStatic)
    server.on("/api/state", HTTP_GET, handleApiState);

    //  2. /version — tiny endpoint for connectivity checks
    server.on("/version", HTTP_GET, [](AsyncWebServerRequest* req) {
        char buf[96];
        snprintf(buf, sizeof(buf), "{\"project\":\"%s\",\"version\":\"%s\"}",
                 AURORA_PROJECT_NAME, AURORA_VERSION);
        req->send(200, "application/json", buf);
    });

    //  3. Captive Portal probe handling.
    //     Android/Windows/Linux probes get 204 ("internet OK") so phones
    //     connect SILENTLY — no "Sign in to network" page, which is the
    //     WebView that scrolls poorly. The dashboard is opened by scanning
    //     the OLED's dashboard QR (http://192.168.4.1/).
    //     iOS probes still redirect so iPhone auto-opens Safari (which
    //     scrolls fine), keeping that convenience intact.
    auto respondNoContent = [](AsyncWebServerRequest* req) {
        req->send(204);
    };

    // Android captive portal probes -> 204 (silent connect, no sign-in page)
    server.on("/generate_204", HTTP_GET, respondNoContent);
    server.on("/generate204", HTTP_GET, respondNoContent);
    server.on("/gen_204", HTTP_GET, respondNoContent);
    server.on("/mobile/status", HTTP_GET, respondNoContent);
    server.on("/check_network_status", HTTP_GET, respondNoContent);

    // Apple / iOS captive portal probes: keep redirect for auto-open in Safari
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* req) {
        // iOS expects "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"
        // for "no captive portal". Anything else triggers the sign-in sheet.
        req->redirect("http://192.168.4.1/");
    });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });

    // Windows NCSI + Firefox / Linux captive portal probes -> 204 (silent)
    server.on("/connecttest.txt", HTTP_GET, respondNoContent);
    server.on("/ncsi.txt", HTTP_GET, respondNoContent);
    server.on("/redirect", HTTP_GET, respondNoContent);
    server.on("/success.txt", HTTP_GET, respondNoContent);
    server.on("/canonical.html", HTTP_GET, respondNoContent);

    //  OTA endpoints (firmware reflash + LittleFS file upload over WiFi).
    //  Registered only while the softAP is up, reachable at 192.168.4.1.
#if AURORA_OTA_ENABLED
    server.on("/update", HTTP_GET, handleOtaForm);
    server.on("/update", HTTP_POST, handleOtaUpdate, handleOtaUpdateUpload);
    server.on("/upload", HTTP_POST, handleFsUpload, handleFsUploadBody);
    DBG_PRINTLN(F("[OTA] /update + /upload endpoints armed"));
#endif

    //  4. Static file serving from LittleFS (checked only for asset files)
    server.serveStatic("/", LittleFS, "/")
          .setDefaultFile("index.html")
          .setCacheControl("public, max-age=86400");

    server.onNotFound(handleNotFound);
    server.addHandler(&ws);
    ws.onEvent(onWsEvent);
    server.begin();
    DBG_PRINTLN(F("[HTTP] server started on port 80"));
    return true;
}

void loop() {
    if (!AuroraState::instance().wifiOn()) return;

    uint32_t now = millis();
    if (g_pushNow || (now - g_lastPush) >= (uint32_t)AURORA_WS_PUSH_MS) {
        g_lastPush  = now;
        g_pushNow   = false;
        pushStateToAll();
    }
    //  Clean up any clients that have been idle too long.
    ws.cleanupClients();
}

void requestImmediatePush() {
    g_pushNow = true;
}

} // namespace aurora_web
