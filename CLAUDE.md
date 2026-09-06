# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-C3 firmware project for an Aurora birthday gift. The project uses PlatformIO for building and managing dependencies. The firmware combines:
- Block 1: OLED status screen + LED breathing chaser + 4 buttons + battery monitor
- Block 4: WiFi softAP + AsyncWebServer + WebSocket + LittleFS dashboard

## Development Commands

### Building
- `pio run` - Build the firmware
- `pio run -t upload` - Build and upload firmware to ESP32-C3
- `pio run -t uploadfs` - Build and upload LittleFS image (dashboard files)
- `pio run -t clean` - Clean build artifacts
- `pio run -e esp32c3_test` - Build test variant

### Monitoring
- `pio device monitor -b 115200` - Open serial monitor
- `pio device list` - List connected devices

### Testing
- `pio test` - Run unit tests (if configured)
- Individual test files can be run with specific filters

### Common Tasks
- `pio update` - Update PlatformIO and libraries
- `pio lib rebuild` - Rebuild libraries
- `pio run -t update` - Update PlatformIO core and platform

## Project Structure

```
aurora-firmware/
├── src/                  # Main source code (main.cpp)
├── include/              # Header files (config.h, display.h, etc.)
├── lib/                  # External libraries (vendored ESPAsyncWebServer-patched)
├── data/                 # Data files for LittleFS dashboard (index.html, app.css, app.js)
├── docs/                 # Documentation (PIN_DIAGRAM.md)
├── .pio/                 # PlatformIO build artifacts
├── platformio.ini        # PlatformIO configuration
└── partitions_aurora.csv # Partition table configuration (LittleFS labeled "littlefs")
```

### Key Files
- `src/main.cpp` - Main firmware implementation (setup() and loop())
- `include/config.h` - Central configuration (pin map, constants, WiFi settings)
- `include/display.h` - OLED wrapper + popup mechanism
- `include/buttons.h` - 4-button debounce handler
- `include/chaser.h` - LED chaser patterns (breathe, solid, chase)
- `include/battery.h` - ADC + battery status monitoring
- `platformio.ini` - Build configuration with ESP32-C3 specific flags
- `partitions_aurora.csv` - Defines LittleFS partition for dashboard storage
- `data/index.html` - Main dashboard entry point
- `data/app.css` - Dashboard styling
- `data/app.js` - Dashboard logic (WebSocket connection, state updates)

## Architecture Highlights

### Firmware Structure
- Single `src/main.cpp` entry point that initializes all subsystems
- Modular header files in `include/` for each hardware subsystem
- Uses PlatformIO's library dependency management with vendored patches
- Configured specifically for ESP32-C3 RISC-V core with stack size adjustments
- LittleFS filesystem for serving dashboard assets from `data/` directory

### Build System Specifics
- PlatformIO manages ESP-IDF framework (via espressif32 platform)
- Custom partition table defines two partitions: app (0x10000) and littlefs (0x210000)
- Build flags in platformio.ini address RISC-V specific issues:
  - `CONFIG_ASYNC_TCP_RUNNING_CORE=0` - pins AsyncTCP to core 0
  - `CONFIG_ASYNC_TCP_STACK_SIZE=8192` - increases stack size to prevent heap corruption
  - `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1` - USB CDC configuration
- Library dependencies include patched ESPAsyncWebServer to prevent RISC-V heap corruption
- LittleFS image built automatically from `data/` directory during `uploadfs` target

### Code Organization
- Hardware abstraction via header files in `include/`
- State management through `AuroraState` singleton pattern
- Event-driven architecture with `yield()` in main loop for AsyncTCP cooperation
- Popup system for transient OLED messages (battery warnings, WiFi status)
- Modular design allows independent testing of subsystems

## Development Workflow

1. **Local Development**: Edit source files in `src/` and `include/`
2. **Build Firmware**: Run `pio run` to compile (or `pio run -t upload` to build+flash)
3. **Build Dashboard**: Run `pio run -t uploadfs` to compile and upload LittleFS image
4. **Monitor Output**: Use `pio device monitor -b 115200` in separate terminal
5. **Iterate**: Repeat build/upload/monitor cycle as needed
6. **Full Deployment**: For initial flash, run both upload and uploadfs targets

## Important Notes

- The project is specifically configured for ESP32-C3 (Robu "ESP32-C3 with soldering" board)
- Keep only one `[env:esp32c3]` section in platformio.ini for hardware targeting
- Refer to `docs/PIN_DIAGRAM.md` for detailed hardware connections:
  - OLED: SDA=GPIO6, SCL=GPIO7, Address=0x3C (try 0x3D if needed)
  - LED: GPIO4 via 220Ω resistor
  - Buttons: GPIO0(Up, external 10k pull-up), GPIO1(Select), GPIO10(Down), GPIO2(WiFi)
  - Battery: GPIO3 via 100k/100k voltage divider
- PlatformIO handles toolchain (~150MB) and SDK management automatically
- Library dependencies are pinned to specific versions to ensure compatibility
- The patched ESPAsyncWebServer library (`ESPAsyncWebServer-aurora`) resolves RISC-V heap corruption issues
- LittleFS partition label must match `partitions_aurora.csv` (currently "littlefs")
- Serial communication uses USB-CDC at 115200 baud for debugging
- Watchdog timer is planned for implementation in Block 5 for system reliability