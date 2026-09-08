# CLAUDE.md

This file provides guidance to Claude Code / Antigravity when working with code in this repository.

## Project Overview

This repository (`aurora-firmware-deskmate`) is the **Animated Romantic OLED Deskmate** firmware for the ESP32-C3 Aurora birthday gift for Chandni. The project uses PlatformIO for building and managing dependencies.

The firmware combines:
- **Block 1 Hardware & OLED Deskmate Engine**: Living animated OLED desk pet ("Aurora") with expressive eyes (normal, happy, heart eyes + floating hearts, sad/lonely after 1 hour neglect, 12:00 AM midnight daily message reminder, sleeping), single physical touch button input on GPIO0, dynamic mood-synced breathing LED, battery monitoring, and text bounds checking (fixing RHS text clipping).
- **Block 4 Dashboard & SoftAP**: WiFi softAP (`Aurora`), AsyncWebServer, AsyncWebSocket, and LittleFS mobile dashboard (`192.168.4.1`) serving daily messages, keepsake letters, and music box.

## Development Commands

### Building
- `pio run` - Build the firmware binary
- `pio run -t upload` - Build and upload firmware to ESP32-C3
- `pio run -t buildfs` - Build the LittleFS filesystem binary image
- `pio run -t uploadfs` - Upload LittleFS image (dashboard files)
- `pio run -t clean` - Clean build artifacts

### Monitoring
- `pio device monitor -b 115200` - Open serial monitor
- `pio device list` - List connected devices

## Project Structure

```
aurora-firmware-deskmate/
├── src/                  # Main source code
│   ├── main.cpp          # Main firmware setup and loop
│   ├── block1/           # Display, buttons, LED chaser, battery driver
│   └── block4/           # WiFi AP, AsyncWebServer, state, messages, clock
├── include/              # Header files (config.h, display.h, buttons.h, etc.)
├── lib/                  # External libraries (vendored ESPAsyncWebServer-aurora)
├── data/                 # Data files for LittleFS dashboard (index.html, app.css, app.js, messages.js)
├── docs/                 # Hardware connection documentation (PIN_DIAGRAM.md)
├── platformio.ini        # PlatformIO build configuration
└── partitions_aurora.csv # Partition table (LittleFS labeled "littlefs")
```

### Key Files
- `src/main.cpp` - Unified firmware entry point (deskmate animation loop, touch events, midnight check, attention check)
- `include/config.h` - Central configuration (GPIO pin map, deskmate frame rate, neglect interval, WiFi AP credentials)
- `include/display.h` - `AuroraDisplay` class declaration with `DeskmateMood` enum and particle engine
- `src/block1/display.cpp` - Animated OLED Deskmate character renderer, procedural eyes, floating hearts, and RHS clipping fix
- `include/buttons.h` & `src/block1/buttons.cpp` - Single touch button handler on GPIO0
- `include/chaser.h` & `src/block1/chaser.cpp` - LED breathing chaser with mood-synchronized period
- `include/state.h` & `src/block4/state.cpp` - State singleton tracking Warm Touches, last touch timestamps, battery, and epoch
- `platformio.ini` - ESP32-C3 PlatformIO configuration with U8g2 and ESPAsyncWebServer

## Architecture Highlights

### Hardware Configuration
- **OLED Display**: SDA=GPIO8, SCL=GPIO9 (I2C at 400kHz, 128x64, supports SSD1306 and SH1106).
- **Single Touch Button**: GPIO0 (`AURORA_BTN_TOUCH_PIN`) with 10kΩ external pull-up. Records Warm Touches, wakes/cheers up the deskmate, and opens midnight reminders.
- **LED Breathing Chaser**: GPIO4 via 220Ω resistor. Breathing period adjusts dynamically based on Aurora's mood (800ms excited heartbeat when touched, 1500ms normal, 3200ms slow lonely breath).
- **Battery ADC**: GPIO3 via 100kΩ/100kΩ voltage divider.

### Deskmate Character States (`DeskmateMood`)
- `IDLE_NORMAL`: Organic blinking, eye gaze saccades, cat smile `w`.
- `HAPPY`: Bouncy arched eyes `^ ^`, blushing cheeks `///`, open smile `\_/`.
- `LOVE_TOUCHED`: Beating heart eyes `<3 <3`, floating hearts rising up, blushing cheeks, rotating romantic quote banner.
- `LONELY_SAD`: Triggered after 1 hour without a Warm Touch. Droopy eyes, downward quivering mouth, sliding teardrop, prompt `"Miss you... Touch me?"`.
- `MIDNIGHT_REMINDER`: Triggered at 00:00 (12:00 AM midnight). Animated love envelope with wax seal, sparkles, and prompt to check phone dashboard for the Daily Message.
- `SLEEPING`: Peaceful closed curved eyes with floating `"z Z Z"` bubbles.

## Important Notes

- All technical metrics (client count, raw AP stats, touch counters) are hidden from the OLED to preserve a soft, romantic companion experience. Technical statistics remain accessible via the web dashboard at `192.168.4.1`.
- Text width bounds checking (`_u8g2.getStrWidth()`) is strictly enforced in `popup()` and message banners to prevent RHS screen clipping.