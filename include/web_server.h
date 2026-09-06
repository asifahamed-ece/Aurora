/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/web_server.h
 *  Block 4: AsyncWebServer + AsyncWebSocket + LittleFS
 *
 *  - Mounts LittleFS (read-only) and serves /, /style.css, /app.js, /messages.js,
 *    /letter.html, and any other file in /data.
 *  - Handles GET /api/state — returns a JSON snapshot.
 *  - Handles WS /ws — pushes state every AURORA_WS_PUSH_MS, replies to ping
 *    frames with pong, and emits a single 'hello' frame on connect.
 *  - Captive-portal-style fallback: any unknown GET returns index.html so
 *    phones that auto-redirect to a "login" page land on the dashboard.
 */
#ifndef AURORA_BLOCK4_WEB_SERVER_H
#define AURORA_BLOCK4_WEB_SERVER_H

#include <Arduino.h>

namespace aurora_web {

//  Mount LittleFS, register routes, start the server, and start the WS
//  broadcast loop. Returns true if LittleFS mounted.
bool begin();

//  Pump the WS broadcast timer. Call from loop().
void loop();

//  Bump a "now" flag so the next loop tick pushes immediately (used after
//  button events so the user sees the counter change without a 2s wait).
void requestImmediatePush();

} // namespace aurora_web

#endif // AURORA_BLOCK4_WEB_SERVER_H
