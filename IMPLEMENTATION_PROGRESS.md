# Aurora — Implementation Progress Tracker

> **Owner:** Asif (ECE final-year) • **AI Partner:** Claude Code / Antigravity
> **Goal:** A hand-built ESP32-C3 birthday gift for Chandni (CSE classmate), due Sept 10, 2026.
> **Current Version:** `aurora-firmware-deskmate` (Fork with Animated Romantic OLED Deskmate & Touch Button Consolidation)

---

## Current Firmware Status: ✅ ALL BLOCKS COMPLETE & VERIFIED

The project has been upgraded to **Aurora Deskmate Edition** in `aurora-firmware-deskmate`. Technical readouts (AP name, station count, raw millivolts, button counts) have been removed from the 128x64 OLED display and replaced with an expressive, living **Animated Deskmate ("Aurora")**.

---

## Subsystem Implementation Summary

| Subsystem | Status | Notes |
|---|---|---|
| **OLED Deskmate Engine** | ✅ PASSED | Living OLED pet with expressive eyes (normal, happy, heart eyes + floating hearts, sad/lonely after 5+ hours, midnight reminder, sleeping) |
| **OLED RHS Text Clipping Fix** | ✅ PASSED | Dynamic text measurement with `u8g2.getStrWidth()` and font cascading (`7x14` -> `6x12` -> `5x7`). Text is centered and never clipped. |
| **Single Touch Button** | ✅ PASSED | Consolidated to single touch button on GPIO0 (`AURORA_BTN_TOUCH_PIN`). Bumps Warm Touches, cheers deskmate, opens midnight reminders. |
| **Attention / Neglect System** | ✅ PASSED | Tracks elapsed time since last touch; transitions to `LONELY_SAD` with droopy eyes, quivering mouth, sliding teardrop after 5+ hours. |
| **Midnight Message Reminder** | ✅ PASSED | Automatically triggers at 00:00 (12:00 AM) with animated envelope, heart seal, sparkles, and daily message prompt. |
| **LED Heartbeat Tempo Sync** | ✅ PASSED | Syncs LED breathing period to deskmate mood (800ms excited heartbeat when touched, 1500ms normal, 3200ms slow lonely sigh). |
| **WiFi SoftAP & Captive Portal** | ✅ PASSED | SoftAP `Aurora` (`192.168.4.1`) with AsyncWebServer, AsyncWebSocket, DNS captive portal, and zero-crash RISC-V heap protection. |
| **LittleFS Web Dashboard** | ✅ PASSED | Mobile dashboard serving 30 daily messages, keepsake envelope jar, music box player, and birthday mode. |

---

## Block Architecture & Verification Status

### Block 1 — Hardware Drivers + Animated Deskmate Engine
- **Single Touch Button (GPIO0):** Active LOW with 10kΩ external pull-up for boot safety.
- **OLED Display (GPIO8 SDA, GPIO9 SCL):** 400kHz I2C, supports 0.96" SSD1306 and 1.3" SH1106 displays.
- **LED Breathing Chaser (GPIO4):** LEDC PWM channel 0 with dynamic period control.
- **Battery ADC (GPIO3):** Voltage divider (100kΩ/100kΩ), averaged sampling.
- **Status:** ✅ PASSED & VERIFIED

### Block 4 — Connectivity + State Management
- **AsyncWebServer & WebSocket Server:** Real-time state push on touch events.
- **Persistent Keepsake Touches:** Saved to NVS flash (`Preferences`) along with last touch timestamps.
- **RISC-V Heap Protection:** Pre-allocated request buffer (`_temp.reserve(2048)`) preventing RISC-V memory corruption.
- **Status:** ✅ PASSED & VERIFIED

---

## Firmware Build Results

```
Processing esp32c3 (platform: espressif32; board: esp32-c3-devkitm-1; framework: arduino)
RAM:   12.8% (used 41828 bytes from 327680 bytes)
Flash: 70.2% (used 920396 bytes from 1310720 bytes)
Building .pio/build/esp32c3/firmware.bin
Successfully created esp32c3 image.
[SUCCESS] Took 14.05 seconds
```

### LittleFS Image Build
```
Building FS image from 'data' directory to .pio/build/esp32c3/littlefs.bin
/messages.js
/index.html
/app.js
/letter.html
/style.css
[SUCCESS] Took 1.08 seconds
```

---

## Change Log & Milestones

| Date | Event / Upgrade | Details |
|---|---|---|
| Sept 5 | Initial Project Setup | PIO configuration, 4-button input, OLED wrapper, battery ADC, single-LED chaser |
| Sept 6 | Block 1 + Block 4 Merge | Unified setup/loop, LittleFS filesystem mount, AsyncTCP stack size increase to 8KB |
| Sept 6 | RISC-V Stability Patch | Vendored `ESPAsyncWebServer-aurora` pre-allocating request buffer to prevent heap crashes |
| Sept 6 | Birthday Milestone & Letters | Chapter 22 birthday detection, opening loading curtain, 6 keepsake letters, music box |
| Sept 6 | **Deskmate Fork (`aurora-firmware-deskmate`)** | **Created fork; consolidated to 1 touch button (GPIO0); built Animated OLED Deskmate engine; fixed RHS text clipping; implemented 5hr neglect mechanism, 12 AM midnight reminder, and LED heartbeat sync.** |

---

## Flash & Run Commands

```bash
cd "/home/shadow/Desktop/ClaudeCode Chats/aurora-firmware-deskmate"

# Build & upload firmware
pio run -t upload

# Upload LittleFS dashboard assets
pio run -t uploadfs

# Monitor serial output
pio device monitor -b 115200
```
