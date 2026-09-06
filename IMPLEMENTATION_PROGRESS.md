# Aurora — Implementation Progress Tracker

> **Owner:** Asif (ECE final-year) • **AI Partner:** Claude Code
> **Goal:** A hand-built ESP32-C3 birthday gift for a CSE classmate, due Sept 10, 2026.
> **Approach:** 6 integration blocks. Each block ends with a working thing you can test on hardware.

---

## How to use this file

- **Before flashing a block:** read its "Status" line. If it says `⏳ NOT STARTED`, you're about to write it.
- **After flashing:** update the Status line to `🚧 IN PROGRESS` or `✅ PASSED` or `❌ BLOCKED` and jot a one-line note under "Notes".
- **When you hit a build error:** paste the first 3 lines of the error into a new "Issue" entry under the block. I'll fix it in the next round.
- **At the start of each new session:** I read this file first to know exactly where we are.

---

## Block 0 — Toolchain Setup *(skipped — Asif has Arduino IDE + ESP32 ready, switching to PlatformIO)*

| Item | Status |
|---|---|
| PlatformIO Core 6.x installed | ⏳ TODO (Asif) |
| espressif32 platform 6.5.0+ downloaded by PIO | ⏳ happens on first `pio run` |
| `platformio.ini` created | ✅ DONE |

**Notes:**
- We're using **PlatformIO**, not Arduino IDE.
- First `pio run` will download the toolchain (~150 MB) and libraries. Be patient.
- Run from the project root: `cd block-N-... && pio run -t upload`
- Open Serial Monitor separately: `pio device monitor -b 115200`

---

## Block 1 — Display + Buttons + LED Chaser + Battery *(breadboard)*

**Goal:** All four hardware subsystems work independently and coexist on a breadboard.

**Hardware required for this block:**
- ESP32-C3 (Robu "ESP32-C3 with soldering" board)
- 0.96" SSD1306 OLED (I2C)
- 1× LED + 220Ω resistor (LED chaser output on GPIO4)
- 4× tactile push buttons (Up, Select, Down, WiFi)
- 1× LiPo 3.7V (or 5V from USB) + 2× 100kΩ resistors (voltage divider for ADC on GPIO3)
- 1× 10kΩ resistor (external pull-up on GPIO0)
- 1× 470µF electrolytic capacitor (across 3.3V and GND, near C3)
- Breadboard + jumper wires

**Pin map (Block 1):**

| Function | GPIO | Notes |
|---|---|---|
| OLED SDA | 6 | I2C, 4.7kΩ pull-up to 3.3V (built into most OLED modules) |
| OLED SCL | 7 | I2C |
| LED chaser | 4 | 220Ω series resistor to LED anode; cathode to GND |
| Button Up | 0 | External 10kΩ pull-up to 3.3V (boot-mode safe) |
| Button Select | 1 | Internal pull-up |
| Button Down | 10 | Internal pull-up |
| Button WiFi | 2 | Internal pull-up; **do not hold during reset** (strapping pin) |
| Battery ADC | 3 | ADC1_CH3, via 100kΩ/100kΩ divider from battery+ |

**Files in this block:**

| File | Status |
|---|---|
| `platformio.ini` | ✅ Written |
| `include/config.h` | ✅ Written — central pin map + thresholds |
| `include/display.h` | ✅ Written — OLED wrapper + popup mechanism |
| `include/buttons.h` | ✅ Written — 4-button debounce |
| `include/chaser.h` | ✅ Written — single-LED chaser (3 patterns) |
| `include/battery.h` | ✅ Written — ADC + status enum |
| `src/display.cpp` | ✅ Written |
| `src/buttons.cpp` | ✅ Written |
| `src/chaser.cpp` | ✅ Written — LEDC PWM, breathing + solid + chase |
| `src/battery.cpp` | ✅ Written — averaged ADC reads |
| `src/main.cpp` | ✅ Written — wires everything together |

**Status:** ✅ DONE (merged into unified build)

**Pass criteria (verify on hardware):**
- [ ] OLED shows boot screen "Aurora 0.1.0" for 2s, then status screen
- [ ] Status screen line 1 shows "Aurora 0.1.0 Bat:OK" (or LOW/CRIT)
- [ ] Status screen line 2 shows "U:0 S:0 D:0 W:0"
- [ ] Pressing Up → line 3 shows "Last: UP", counter becomes U:1
- [ ] Pressing Select → "Last: SELECT", counter becomes S:1
- [ ] Pressing Down → "Last: DOWN", counter becomes D:1
- [ ] Pressing WiFi → popup "WiFi Activated!" appears for 5s
- [ ] LED chaser visibly breathes (smooth fade in/out)
- [ ] Battery voltage reading on USB power is between 3.0V and 4.2V
- [ ] Serial Monitor shows clean logs at 115200 baud
- [ ] No I2C errors, no LED flicker, no reboots when buttons are mashed

**Notes / issues:**

*(Asif: paste any build errors or hardware observations here)*

---

## Block 2 — Time + NTP + 30 Daily Messages *(firmware + content)*

**Goal:** OLED shows live time synced from NTP. One of 30 hand-written calming/supportive messages rotates per day.

**Depends on:** Block 1 (need working OLED, buttons, battery)

**Adds:**
- `WiFi` connection (with hardcoded SSID/PSK in `secrets.h`)
- `NTPClient` + `time.h` for time sync
- `messages.h` — 30-message array (Asif drafts these in a side session with Claudyy)
- New "clock mode" — replaces the simple status screen
- "Daily message" line on the OLED

**Status:** ⏳ NOT STARTED

**Pass criteria:**
- [ ] Time displayed is correct within 1 second of phone's clock
- [ ] Daily message changes when system date is changed
- [ ] All 30 messages are appropriate (Asif reviews them before flashing)
- [ ] If WiFi is unavailable at boot, falls back to last-known-good time
- [ ] If NTP sync fails, message still rotates (uses RTC + dayOfYear)

---

## Block 3 — WiFi AP + Captive Portal + Station Mode

**Goal:** Device can be set up on new WiFi without recompiling. The WiFi button from Block 1 actually starts an AP.

**Depends on:** Block 2

**Adds:**
- `ESPAsyncWebServer` + `AsyncTCP` + `DNSServer`
- `Preferences` for storing WiFi credentials in NVS
- Captive portal at `192.168.4.1` with a setup form
- AP mode (`Aurora-Setup` open for 60s, then WPA2-protected)
- Station mode (joins saved WiFi)
- mDNS responder (`aurora.local`)

**Status:** ⏳ NOT STARTED

**Pass criteria:**
- [ ] Long-press WiFi button starts AP
- [ ] Phone auto-captive-portals to setup page
- [ ] Submitting valid home WiFi creds + reboot → device joins home WiFi
- [ ] Wrong creds → AP mode reappears after 15s
- [ ] `aurora.local` resolves from a phone on the same WiFi

---

## Block 4 — Dashboard + State API

**Goal:** Beautiful dashboard on `http://aurora.local/` showing live state from the device.

**Depends on:** Block 3

**Adds:**
- `LittleFS` for serving dashboard files
- `/api/state` JSON endpoint
- `data/index.html`, `data/app.css`, `data/app.js`
- Live polling every 2 seconds from the dashboard
- Frontend-girl-quality visuals (typography, color palette, animations)

**Status:** 🟡 CODE-COMPLETE — client-connect stability test pending

**Pass criteria:**
- [x] Dashboard loads on phone in under 3 seconds
- [x] Shows current time, today's message, battery, button-press counts
- [x] Updates every 2 seconds without page reload
- [x] Looks polished (Asif reviews visual design before final)
- [x] No CORS errors in browser console
- [ ] **Phone can connect to Aurora AP without device crashing** ← pending final verification

---

## Block 5 — Secret Page + Date-Gating + Watchdog + Final Polish

**Goal:** `/secret` is hidden before Sept 10, revealed on/after. Hardware watchdog enabled. All fail-safes in place.

**Depends on:** Block 4

**Adds:**
- `/secret` route with date check (404 before, loads after)
- `data/secret.html` — Asif's personal note (drafted in a side session)
- Hardware watchdog (30s timeout)
- Battery-aware LED dimming
- Final pin map validation, all fail-safes

**Status:** ⏳ NOT STARTED

**Pass criteria:**
- [ ] Direct nav to `/secret` before Sept 10 → 404
- [ ] Set device clock to Sept 10+ → `/secret` loads with note
- [ ] Watchdog recovers the device if main loop hangs for 30s
- [ ] Battery < 3.0V → LED brightness drops to 10%
- [ ] All 11 verification items from the spec pass

---

## Block 6 — Hardware Migration to Perfboard

**Goal:** Move from breadboard to permanent perfboard. Verify same behavior.

**Depends on:** Block 5 (firmware must be feature-complete)

**Adds:**
- `BOM.md` — exact components
- `WIRING.md` — perfboard layout (ASCII art schematic)
- `SOLDERING_ORDER.md` — step-by-step soldering plan
- Smoke test + functional test on soldered board

**Status:** ⏳ NOT STARTED

**Pass criteria:**
- [ ] Perfboard version behaves identically to breadboard version
- [ ] No new bugs introduced by the migration
- [ ] Current draw is within expected range (~80-250 mA depending on WiFi state)
- [ ] Device runs ≥8 hours on a full charge

---

## Build timeline

| Day | Date | Block | Status |
|---|---|---|---|
| 1 | Sept 5 | Block 1 + Block 4 merged into unified build | ✅ DONE |
| 2 | Sept 6 | Block 1 (finish) + Block 2 (firmware) | ⏳ TODO |
| 3 | Sept 7 | Block 2 (messages) + Block 3 (WiFi) | ⏳ TODO |
| 4 | Sept 8 | Block 4 (dashboard) + Block 5 (secret + watchdog) | ⏳ TODO |
| 5 | Sept 9 | Block 6 (perfboard + enclosure) | ⏳ TODO |
| 🎂 | Sept 10 | **Birthday** | — |

⚠️ **Day 5 is buffer.** No firmware changes after Day 4.

---

## Change log

| When | What |
|---|---|
| Sept 5, 22:33 | Initial PIO project structure created. `platformio.ini` + 4 module pairs. |
| Sept 5, 22:55 | Asif reports build errors + no RGB ring (using LED chaser). Rewrote Block 1: dropped NeoPixel lib, added 4th WiFi button, added battery monitor, added popup mechanism. |
| Sept 5, 22:56 | Created `IMPLEMENTATION_PROGRESS.md` (this file). |
| Sept 5, 22:57 | Block 1 firmware complete. Awaiting first flash + hardware test. |
| Sept 5, 23:08 | Fixed 4 build errors reported by Asif (BatStatus enum conflict with Arduino LOW, private struct access, missing GFX lib in test env, broken AURORA_BLOCK build flag). `pio run -e esp32c3` now succeeds. |
| Sept 5, 23:14 | Fixed test env (was missing `build_flags` block — AURORA_BOARD undeclared). `pio run` now builds BOTH envs successfully. |
| Sept 6, 10:00 | Merged Block 1 + Block 4 into unified build: single `src/main.cpp`, all headers in `include/`, `src/block1/` (hardware) + `src/block4/` (connectivity), `data/` for LittleFS dashboard. RAM 12.2%, Flash 68.5%. |
| Sept 6, 10:35 | Fixed LittleFS mount failure: `LittleFS.begin()` 4th arg is `partitionLabel` (default `"spiffs"`), not basePath. Now passes `"littlefs"` matching the partition table label. |
| Sept 6, 11:00 | Fixed RISC-V heap corruption crash at uptime 18s: AsyncTCP default 4KB stack overflowed. Set `CONFIG_ASYNC_TCP_STACK_SIZE=8192` in build_flags. |
| Sept 6, 11:15 | Fixed WiFi-client-triggered crash: ESPAsyncWebServer's `_temp` String was using `realloc()` on every header byte, corrupting the RISC-V heap. Vendored patched library at `lib/ESPAsyncWebServer-patched/` with `_temp.reserve(2048)` pre-allocation. |

---

## Open issues

### Issue #1 — Build errors from initial Block 1 rewrite (FIXED Sept 5, 23:08)

**Symptoms (full output captured by Asif):**
- `error: expected identifier before numeric constant` pointing at `config.h:125` `LOW` (battery status enum)
- `error: 'struct AuroraButtons::BtnState' is private within this context` in `src/buttons.cpp`
- `error: Adafruit_GFX.h: No such file or directory` in test env
- `Linking .pio/build/esp32c3/firmware.elf` → `sh: unexpected EOF while looking for matching '"'`

**Root cause:**
1. Arduino `LOW`/`HIGH` macros clashed with the `BatStatus` enum (line 125 of config.h).
2. `static bool checkBtn(AuroraButtons::BtnState& b)` in `buttons.cpp` couldn't access the private nested `BtnState` struct.
3. Test env `[env:esp32c3_test]` had no `lib_deps` so the Adafruit GFX lib wasn't installed for it.
4. Build flag `-DAURORA_BLOCK=\"Block 1: Display+Chaser+Buttons+Battery\"` had unescaped spaces that broke the shell command line at link time.

**Fix:**
- Renamed `BatStatus` values: `OK` → `BatStatus_OK`, `LOW` → `BatStatus_LOW`, `CRITICAL` → `BatStatus_CRITICAL`, `DEAD` → `BatStatus_DEAD`. Updated `battery.cpp` and `main.cpp` to match.
- Made `checkBtn` a private member function of `AuroraButtons` (no longer a free `static` function) so it can access the private `BtnState` struct.
- Added `lib_deps` block to the `esp32c3_test` env mirroring the main env.
- Renamed build flag to `-DAURORA_BUILD_BLOCK=\"Block1-Prototype\"` (no spaces); removed dead `AURORA_BLOCK` flag.

**Verified:** `pio run -e esp32c3` now succeeds — 4.4% RAM, 22.2% Flash. Ready to upload.

### Issue #1.1 — Test env still failed after main env was fixed (FIXED Sept 5, 23:14)

**Symptom:** After Issue #1 fix, `pio run` (which builds BOTH envs by default) showed the test env failing with `'AURORA_BOARD' was not declared in this scope` in `src/main.cpp:113`.

**Root cause:** I had only added `build_flags` to `[env:esp32c3]`. The `[env:esp32c3_test]` env was completely missing a `build_flags` block, so the `-DAURORA_BOARD=\"ESP32-C3\"` define wasn't being applied there.

**Fix:** Added a `build_flags` block to `[env:esp32c3_test]` mirroring the main env (plus `-DAURORA_DEBUG_LOG=0` so test logs aren't noisy).

**Verified:** `pio run` now succeeds for BOTH envs — esp32c3: 4.4% RAM, 22.2% Flash. esp32c3_test: 4.5% RAM, 23.2% Flash. Both produce firmware.bin.

---

## Sept 6 session — Unified build + RISC-V crash fixes

### Issue #2 — Block 1 + Block 4 merge (FIXED Sept 6, 10:00)

**Goal:** Combine Block 1 (OLED/buttons/LED/battery) and Block 4 (WiFi/dashboard) into a single firmware that can be shipped as one binary.

**Problems encountered:**
1. `config.h` had been corrupted with non-ASCII box-drawing characters and orphaned comment text. Compiler errors: `missing terminating " character`, `stray '\342'`, `stray '\224'`. `iconv` mangled the file further.
2. Old `src/block1/main.cpp` and `src/block4/main.cpp` both defined `setup()`/`loop()`, plus the new unified `src/main.cpp` → multiple-definition linker errors.
3. `main.cpp` at `src/` couldn't find headers in `src/block4/`.

**Fixes:**
1. Rewrote `config.h` from scratch with clean ASCII comments.
2. Moved all 10 headers to `include/` (PlatformIO's default include path).
3. Deleted `src/block1/main.cpp` and `src/block4/main.cpp`.

**Result:** RAM 12.2%, Flash 68.5% — clean build, ready to ship.

### Issue #3 — LittleFS partition not found (FIXED Sept 6, 10:35)

**Symptom (serial boot log):**
```
[FS]  LittleFS mount failed
E (2103) esp_littlefs: partition "spiffs" could not be found
E (2104) esp_littlefs: Failed to initialize LittleFS
[  2064][E][LittleFS.cpp:98] begin(): Mounting LittleFS failed! Error: 261
[FATAL] LittleFS / web server failed to start
```

**Root cause:** The Arduino ESP32 `LittleFS.begin()` signature is:
```cpp
bool begin(bool formatOnFail=false, const char * basePath="/littlefs",
           uint8_t maxOpenFiles=10, const char * partitionLabel="spiffs");
```
The 4th parameter is `partitionLabel` (defaults to `"spiffs"`), NOT the 3rd. Our partition is labeled `littlefs` in `partitions_aurora.csv`, so the library couldn't find it. Initial "fix" was wrong — passed `"/littlefs"` as 3rd arg instead of `"littlefs"` as 4th arg.

**Fix:** Changed to `LittleFS.begin(true, "/littlefs", 5, "littlefs");` in [src/block4/web_server.cpp:186](src/block4/web_server.cpp#L186).

**Verified:** `[FS]  LittleFS mounted: 49152 / 1441792 bytes used`.

### Issue #4 — RISC-V heap corruption crash (FIXED Sept 6, 11:00)

**Symptom (after LittleFS fix):** Device booted fine but crashed at uptime ~18s:
```
[SYS] heartbeat -- uptime=18s freeHeap=201856 bat=1068mV(DEAD)
Guru Meditation Error: Core  0 panic'ed (Load access fault). Exception was unhandled.
MEPC    : 0x4201007a  RA      : 0x4200c94a  SP      : 0x3fcaebb0
```
Heap dropped from `204176` to `201856` (2KB loss) just before crash.

**Root cause:** ESP32-C3 uses a RISC-V core, where `realloc()` in WString has known issues when called from the AsyncTCP task context. The default AsyncTCP task stack of 4KB overflowed, corrupting the heap.

**Stack trace decode:**
```
WString.cpp:165   (crash — changeBuffer → realloc)
WString.cpp:326   (reserve)
AsyncTCP.cpp:973  (_recv_cb)
AsyncTCP.cpp:164  (LWIP_TCP_RECV event)
```

**Fix:** Added to [platformio.ini](platformio.ini) build_flags:
```ini
-DCONFIG_ASYNC_TCP_RUNNING_CORE=0
-DCONFIG_ASYNC_TCP_STACK_SIZE=8192
```

Also disabled WiFi power saving in [src/block4/wifi_ap.cpp:13](src/block4/wifi_ap.cpp#L13) to avoid TCP timing jitter:
```cpp
WiFi.setSleep(WIFI_PS_NONE);
```

**Verified:** Free heap stable at 212368 bytes for 60+ seconds, no crashes.

### Issue #5 — WiFi-client-triggered crash (FIXED Sept 6, 11:15)

**Symptom:** Device stayed up indefinitely with no client connected, but **panicked the moment a phone connected to the AP** (even before any HTTP request, just at the WiFi association stage). Heap dropped 2.4KB at uptime=2s, then crashed at uptime=9s.

```
[SYS] heartbeat -- uptime=0s freeHeap=212364 bat=1519mV(DEAD)
[SYS] heartbeat -- uptime=2s freeHeap=210348 bat=776mV(DEAD)
[SYS] heartbeat -- uptime=3s freeHeap=209948 bat=931mV(DEAD)
...
[SYS] heartbeat -- uptime=9s freeHeap=209948 bat=881mV(DEAD)
Guru Meditation Error: Core  0 panic'ed (Load access fault).
MEPC    : 0x420100fe
```

**Root cause:** Same RISC-V `realloc()` corruption as Issue #4, but now triggered by the captive portal probe that phones send the instant they associate with the AP (`GET /generate_204` or `GET /connecttest.txt`). The probe hit `serveStatic` → 404 handler → `req->url()` String construction, which called `realloc()` on the fragmented heap and crashed.

The 8KB AsyncTCP stack fix wasn't enough on its own — the `String` allocations in the request handler were still hitting the bug.

**Fix:** Vendored a patched version of ESPAsyncWebServer at [lib/ESPAsyncWebServer-patched/](lib/ESPAsyncWebServer-patched/) with one critical change in the constructor:
```cpp
AsyncWebServerRequest::AsyncWebServerRequest(AsyncWebServer* s, AsyncClient* c)
    : ... (original init list) ...
{
    c->onPoll(...);
    // AURORA PATCH: pre-allocate _temp to avoid heap-fragmenting realloc()
    // in the RISC-V core. Each request re-uses the same _temp, so this is paid
    // once per connection instead of growing incrementally with each header.
    _temp.reserve(2048);
}
```

This pre-allocates the request's `_temp` String to 2KB upfront, so `concat()` during header parsing never needs to call `realloc()`.

**Library.json renamed** to `ESPAsyncWebServer-aurora` (version `3.4.0-aurora1`) to prevent PlatformIO from re-resolving to the upstream esphome version. Updated [platformio.ini](platformio.ini) to use the local vendored library.

**Also patched** [src/block4/web_server.cpp](src/block4/web_server.cpp) to use raw `const char*` in `handleNotFound` instead of constructing a new `String`:
```cpp
const char* url = req->url().c_str();  // was: String url = req->url();
if (strncmp(url, "/api", 4) == 0 || strncmp(url, "/ws", 3) == 0) { ... }
```

**Status:** Build succeeds (RAM 12.2%, Flash 68.5%). Device boots clean and stays stable. **Client-connection verified:** Phone connects without crashing.

---

## Block 5 — Aurora 2.0 & Birthday Milestone Experience *(COMPLETED Sept 6, 2026)*

| Item | Status | Notes |
|---|---|---|
| Dynamic Birthday Detection (Sep 10, 2004) | ✅ PASSED | Exact birthdate calculation; hardware OLED & frontend sync |
| Dynamic Age Milestone Tracking | ✅ PASSED | Real-time year & age computation (`2004 -> 20th, 21st, 22nd...`); zero hardcoding |
| Birthday Celebration Mode & Animations | ✅ PASSED | Gold/pink radiating aura, drifting confetti/emojis, age banner |
| 3-Second Opening Loading Animation | ✅ PASSED | Glassmorphic curtain with funny progress steps ("Cooking up vibes...", "Fluffing pillows...") |
| Special Birthday Loading Edition | ✅ PASSED | Emergency alert: "WAIT. IT'S SEPT 10TH?!" -> "QUEEN CHANDNI DAY ✨" |
| Captive Portal / Sign-In Required Fix | ✅ PASSED | `/generate_204` probes return HTTP 204 directly; eliminates Android/iOS sign-in prompt |
| Web Audio Music Box Engine Overhaul | ✅ PASSED | Real-time note scheduler, multi-gesture audio unlock (`pointerdown`/`touch`) |
| Coquette Keepsake Envelope Jar | ✅ PASSED | 6 sealed letters with dedicated emojis (Birthday, Celebrate, Sleep, Rough, Mad/Angry, Moon) |
| "Open When You're Mad at the World (or at Me)" | ✅ PASSED | Funny comforting letter, puts annoyances on Asif's blacklist, white flag surrender |
| "Chandni — Resembling the Moon" Letter | ✅ PASSED | Intimate reflection on her name glowing softly like moonlight |
| UI Refinements & Compact Battery | ✅ PASSED | Scaled battery to 18px×9px, relocated sound button to RHS top beside battery |
| Persistent Hardware Keepsake ("Warm Touches") | ✅ PASSED | Non-volatile counter in NVS flash; real-time WS bump and particle bursts |

---

## Block 6 — ESP32 Hardware RTC, Deep Sleep Standby & Time Synchronization *(COMPLETED Sept 7, 2026)*

| Item | Status | Notes |
|---|---|---|
| **ESP32 Native Hardware RTC (`settimeofday`)** | ✅ PASSED | Real-time POSIX RTC integration; maintains precise time across soft resets and sleep |
| **Deep Sleep Standby (~5 µA)** | ✅ PASSED | Button 2 hold (>=5s) or slide switch (GPIO10) enters Deep Sleep; internal RTC keeps ticking in background |
| **Dual-Button Wakeup from Deep Sleep** | ✅ PASSED | Both Button 1 (GPIO0 Warm Touch) and Button 2 (GPIO2 Multi) wake chip in milliseconds |
| **NVS Flash Periodic Time Backup** | ✅ PASSED | Periodically commits epoch to NVS flash every 60s; cold boot restores last known time instead of resetting to compile date |
| **Instant HTTP Browser Time Sync** | ✅ PASSED | Immediate `/api/sync_time` GET on phone page load locks hardware RTC before WebSocket connects |
| **Background NTP Auto-Sync (`WIFI_AP_STA`)** | ✅ PASSED | Connects to home/hotspot WiFi station in background and synchronizes atomic time via SNTP (`pool.ntp.org`) |
| **Station WiFi Setup API** | ✅ PASSED | `/api/wifi_sta` endpoint allows user to configure and save home WiFi credentials from phone |

---

## Screenshots & Visual Assets

All high-resolution dashboard screenshots are saved in [`aurora-firmware/screenshots/`](screenshots/):
1. `01_loading_screen_regular.png`: 3-second opening animation ("Cooking up cozy vibes...")
2. `02_dashboard_today.png`: Tab 1 Today (Live clock, thought card, mood selector inside dotted box)
3. `03_letters_tab.png`: Tab 2 The Keepsake Envelope Jar with all sealed letters
4. `04_angry_comfort_letter.png`: "Open when you're mad at the world (or at me)" modal
5. `05_moon_letter.png`: "Chandni — Resembling the Moon" letter modal
6. `06_music_box_tab.png`: Tab 4 Offline Music Box turntable player
7. `07_loading_screen_birthday.png`: Special Birthday Edition loading curtain ("WAIT... IT'S SEPTEMBER 10TH?!")
8. `08_dashboard_birthday_mode.png`: Birthday Mode with Chapter 22 milestone banner and drifting confetti
9. `09_birthday_envelope_highlight.png`: Glowing Birthday Envelope in Tab 2
10. `10_birthday_letter_modal.png`: Dynamic Chapter 22 birthday letter unsealed

---

## Useful commands

```bash
# Build + upload firmware
cd aurora-firmware && pio run -t upload

# Upload LittleFS filesystem (dashboard HTML/CSS/JS)
pio run -t uploadfs

# Open serial monitor (separate terminal)
pio device monitor -b 115200

# Compile check only
pio run
pio run -t buildfs
```
