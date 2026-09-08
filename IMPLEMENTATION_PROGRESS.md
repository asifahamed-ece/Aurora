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
| **OLED Deskmate Engine** | ✅ PASSED | Living OLED pet with expressive eyes (normal, happy, heart eyes + floating hearts, sad/lonely after 1 hour, midnight reminder, sleeping) |
| **Dashboard Touch Reaction on OLED** | ✅ PASSED | Touches sent from mobile web dashboard now trigger the exact same excited heart-eyes reaction and accelerated heartbeat on the physical OLED deskmate. |
| **OLED Safe Margin & Anti-Clipping** | ✅ PASSED | Strict 114-116px width clamping with multi-line word wrapping and font scaling. Safe against bezel and SH1106 column shifts. |
| **Emoji & Pixel-Art Hearts** | ✅ PASSED | Completely eliminated literal `<3` text strings; replaced with clean romantic phrasing and rendered pixel-art graphic hearts (`drawHeart`). |
| **Dynamic Animated Pulse Meter** | ✅ PASSED | Heartbeat Keepsake mode features an animated, travelling ECG waveform and a dynamic lub-dub pulsing heart with BPM acceleration (72 to 120 BPM upon touch). |
| **Multi-Function Button 2 (GPIO2)** | ✅ PASSED | Short press cycles OLED display modes (Deskmate -> Clock & Date -> Daily Quote -> Heartbeat Keepsake). 3-second long press toggles WiFi AP. |
| **Attention / Neglect System** | ✅ PASSED | Tracks elapsed time since last touch; transitions to `LONELY_SAD` with droopy eyes, quivering mouth, sliding teardrop after 1 hour. |
| **Midnight Message Reminder** | ✅ PASSED | Automatically triggers at 00:00 (12:00 AM) with animated envelope, heart seal, sparkles, and daily message prompt. |
| **LED Heartbeat Tempo Sync** | ✅ PASSED | Syncs LED breathing period to deskmate mood (800ms excited heartbeat when touched, 1500ms normal, 3200ms slow lonely sigh). |
| **WiFi SoftAP & Captive Portal** | ✅ PASSED | SoftAP `Aurora` (`192.168.4.1`) with AsyncWebServer, AsyncWebSocket, DNS captive portal, and zero-crash RISC-V heap protection. |
| **LittleFS Web Dashboard** | ✅ PASSED | Mobile dashboard serving 30 daily messages, keepsake envelope jar, music box player, and birthday mode. |

---

## Block Architecture & Verification Status

### Block 1 — Hardware Drivers + Animated Deskmate Engine
- **Touch Button (GPIO0):** Active LOW touch button for Warm Touches.
- **Multi-Function Push Button (GPIO2):** Short click to cycle 4 display modes; 3-second hold to toggle WiFi SoftAP.
- **OLED Display (GPIO8 SDA, GPIO9 SCL):** 400kHz I2C, supports 0.96" SSD1306 and 1.3" SH1106 displays (`AURORA_OLED_DRIVER 1`).
- **LED Breathing Chaser (GPIO4):** LEDC PWM channel 0 with dynamic period control.
- **Battery ADC (GPIO3):** Voltage divider (100kΩ/100kΩ), averaged sampling.
- **Status:** ✅ PASSED & VERIFIED

### Block 4 — Connectivity + State Management
- **AsyncWebServer & WebSocket Server:** Real-time bi-directional touch synchronization.
- **Persistent Keepsake Touches:** Saved to NVS flash (`Preferences`) along with last touch timestamps.
- **RISC-V Heap Protection:** Pre-allocated request buffer (`_temp.reserve(2048)`) preventing RISC-V memory corruption.
- **Status:** ✅ PASSED & VERIFIED

---

## Firmware Build Results

```
Processing esp32c3 (platform: espressif32; board: esp32-c3-devkitm-1; framework: arduino)
RAM:   12.8% (used 41852 bytes from 327680 bytes)
Flash: 70.7% (used 927130 bytes from 1310720 bytes)
Building .pio/build/esp32c3/firmware.bin
Successfully created esp32c3 image.
[SUCCESS] Took 4.86 seconds
```

### LittleFS Image Build
```
Building FS image from 'data' directory to .pio/build/esp32c3/littlefs.bin
/messages.js
/index.html
/app.js
/letter.html
/style.css
[SUCCESS] Took 1.14 seconds
```

---

## Change Log & Milestones

| Date | Event / Upgrade | Details |
|---|---|---|
| Sept 5 | Initial Project Setup | PIO configuration, 4-button input, OLED wrapper, battery ADC, single-LED chaser |
| Sept 6 | Block 1 + Block 4 Merge | Unified setup/loop, LittleFS filesystem mount, AsyncTCP stack size increase to 8KB |
| Sept 6 | RISC-V Stability Patch | Vendored `ESPAsyncWebServer-aurora` pre-allocating request buffer to prevent heap crashes |
| Sept 6 | Birthday Milestone & Letters | Chapter 22 birthday detection, opening loading curtain, 6 keepsake letters, music box |
| Sept 6 | Deskmate Fork & Modes | Consolidated to 2 buttons; animated OLED Deskmate engine; 4 display modes; 1hr neglect mechanism, 12 AM midnight reminder. |
| Sept 7 | **Interactive Polish & Bug Fixes** | **Dashboard touch OLED reaction hook, strict RHS safe margins, replaced `<3` with graphic mini-hearts, dynamic scrolling ECG pulse wave with 72->120 BPM acceleration on touch, and 3-second long press for WiFi toggle.** |

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
