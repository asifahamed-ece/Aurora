/**
 *  Aurora -- Birthday Gift Firmware
 *  File: include/config.h
 *
 *  Central configuration: pin map, constants, build info, AP/WS settings.
 *  Every Aurora firmware file #includes this. Change pin numbers here only.
 *
 *  Build block tag is set by the PlatformIO env's build_flags (-DAURORA_BUILD_BLOCK=...)
 *  so a single config.h serves both Block 1 (OLED + LED chaser) and
 *  Block 4 (WiFi AP + dashboard).
 *
 *  Hardware required (shared across all blocks):
 *    - ESP32-C3 (your Robu "ESP32-C3 with soldering" board)
 *    - 2x tactile push buttons
 *       o GPIO0  -> Warm Touch (pets the deskmate)
 *       o GPIO2  -> Mode cycle (short press) / WiFi AP toggle (3s hold)
 *    - 1x LiPo battery (3.7V) + 2x 100k resistors (voltage divider for ADC)
 *    - 1x 470 uF electrolytic capacitor (across 3.3V and GND, near C3)
 *    - 1x 10 kOhm resistor (external pull-up on GPIO0 / Warm Touch)
 *
 *  Block 1 adds: 0.96" SSD1306 OLED (I2C) + 1 LED + 220-ohm resistor.
 *  Block 4 adds: WiFi softAP + LittleFS dashboard (runs in the browser).
 *
 *  Pin map (shared by both blocks):
 *
 *      ESP32-C3          OLED (SSD1306/SH1106)   LED Chaser          Buttons
 *      ------            ---------------         -----------         --------
 *      GPIO8   --------> SDA   (AURORA_OLED_SDA_PIN)
 *      GPIO9   --------> SCL   (AURORA_OLED_SCL_PIN)
 *      3.3V    --------> VCC                    220 ohm -> LED -> GPIO4
 *      GND     --------> GND                    LED cathode -> GND
 *
 *      GPIO4   ---[220 ohm]---|--- LED anode   (LED cathode -> GND)
 *
 *      GPIO0   <----- BTN_TOUCH (10k ohm pull-up to 3.3V, external)
 *      GPIO2   <----- BTN_MULTI (internal pull-up; short = OLED mode cycle,
 *                                3 second hold = WiFi AP toggle; GPIO2 is a
 *                                strapping pin, never hold it LOW at boot)
 *
 *      Battery Monitor:
 *      Battery+ ---[100k ohm]---+---[100k ohm]--- GND
 *                               |
 *                               +-----> GPIO3 (ADC1_CH3)
 *
 *      470 uF cap between 3.3V and GND (very close to the C3).
 *
 *  Note on GPIO2: This is a strapping pin. The internal pull-up keeps it
 *  HIGH at boot, which is correct. If you hold the WiFi toggle button
 *  (GPIO2) during a reset, the boot may fail. Solution: never hold any
 *  button while pressing RESET or plugging in USB.
 *
 *  Note on GPIO3: GPIO3 is shared with the UART0 RX pin. While we're using
 *  USB-CDC for Serial (per platformio.ini), GPIO3 is free for ADC use. If
 *  you switch to UART0 Serial, you'd need to use a different ADC pin.
 */

#ifndef AURORA_CONFIG_H
#define AURORA_CONFIG_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
//  Build info
// ----------------------------------------------------------------------------
#define AURORA_PROJECT_NAME  "Aurora"
#define AURORA_VERSION       "0.5.1"
#define AURORA_DEDICATEE     "Chandni"
// AURORA_BUILD_BLOCK is supplied by the PlatformIO env's -D flag
// (e.g. -DAURORA_BUILD_BLOCK=\"Block1-Breadboard\"). A sensible fallback
// keeps the firmware compilable from a plain `arduino-cli compile` too.
#ifndef AURORA_BUILD_BLOCK
  #define AURORA_BUILD_BLOCK  "Aurora (unspecified block)"
#endif

// ----------------------------------------------------------------------------
//  Hardware pin map
// ----------------------------------------------------------------------------

// --- I2C (OLED display) ---
#define AURORA_OLED_SDA_PIN   8
#define AURORA_OLED_SCL_PIN   9
// I2C address - try these in order if display shows garbage:
//   0x3C - standard for 0.96" SSD1306
//   0x3D - common for 1.3" SSD1306
//   0x78 - some Chinese modules
#define AURORA_OLED_ADDR      0x3C
// Display driver type:
//   0 = SSD1306 (for 0.96" displays, most common)
//   1 = SH1106 (for 1.3" displays - many Chinese 1.3" OLEDs use this!)
#define AURORA_OLED_DRIVER    1  // <-- SET TO 1 IF USING 1.3" SH1106 DISPLAY
#define AURORA_OLED_WIDTH     128
#define AURORA_OLED_HEIGHT    64
#define AURORA_OLED_RESET     -1         // -1 = no reset pin (use Arduino reset)
#define AURORA_OLED_I2C_FREQ  400000    // 400 kHz fast-mode I2C (SSD1306 supports it)

// --- LED chaser (single LED or LED strip via MOSFET) ---
// GPIO4 is unused on most C3 boards; it has no special boot restrictions.
#define AURORA_LED_PIN        4
// Pattern period in ms -- how long one full "chaser cycle" takes.
#define AURORA_LED_CHASER_PERIOD_MS 1500

// --- Buttons (active LOW with pull-up; press = LOW) ---
// Button 1: Single touch button (GPIO0) - Records Warm Touches & Deskmate petting
// GPIO0 has an external 10k pull-up to keep boot mode safe.
#define AURORA_BTN_TOUCH_PIN        0

// Button 2: Mode-cycle & WiFi push button (GPIO2)
// Single short press: cycles through OLED screen modes
// (DESKMATE -> CLOCK_DATE -> DAILY_QUOTE -> QR_CODE -> HEARTBEAT_KEEPSAKE -> wrap).
// A 3-second hold of the same button toggles the WiFi softAP on/off.
#define AURORA_BTN_MULTI_PIN        2

// Debounce time in ms, applied to BOTH GPIO0 (warm touch) and GPIO2 (mode
// cycle). 80 ms is a comfortable margin for tactile switches and keeps
// both buttons consistent.
#define AURORA_BTN_DEBOUNCE_MS      80

// --- Deskmate Pet & Attention Timing ---
// 1 hour of no touch -> Deskmate feels lonely and needs attention
#define AURORA_DESKMATE_NEGLECT_MS  (1 * 3600 * 1000UL)
// ~20 FPS animation refresh (50ms per frame)
#define AURORA_DESKMATE_FRAME_MS    50

// ----------------------------------------------------------------------------
//  Battery monitor (voltage divider on ADC1_CH3 = GPIO3)
// ----------------------------------------------------------------------------
//   Battery+ ---[R1: 100kOhm]---+---[R2: 100kOhm]--- GND
//                                └──-> GPIO3 (ADC input)
//
//   V_adc = V_battery x R2 / (R1 + R2) = V_battery / 2
//   ESP32-C3 ADC range: 0-3.3V (max 2500 mV per the calibrated eFuse range,
//   but we'll trust the raw 0-3.3V span).
//
//   To compensate for the divider ratio, we multiply ADC reading by 2.
//   This gives us V_battery in volts, approximately.
#define AURORA_BAT_ADC_PIN        3    // ADC1_CH3
#define AURORA_BAT_DIVIDER_RATIO  2.0f // (R1 + R2) / R2 = 2 with 100k/100k
#define AURORA_BAT_ADC_MAX_MV     3300 // ESP32-C3 ADC reference ~3.3V
#define AURORA_BAT_SAMPLES        8    // Number of ADC samples to average (noise reduction)

#define AURORA_BAT_OK_MV          3600 // Above this, "OK" -- fully charged LiPo
#define AURORA_BAT_LOW_MV         3400 // Below this, show "LOW" warning
#define AURORA_BAT_CRITICAL_MV    3100 // Below this, show "CRITICAL" and dim LEDs
#define AURORA_BAT_DEAD_MV        2800 // Below this, refuse to boot (deep sleep)

// Battery status thresholds for display / behavior.
// Using BatStatus_* to avoid conflict with Arduino LOW/HIGH macros
enum class BatStatus : uint8_t {
    BatStatus_OK,        // > 3.5V
    BatStatus_LOW,       // 3.3V - 3.5V
    BatStatus_CRITICAL,  // 3.0V - 3.3V
    BatStatus_DEAD       // < 3.0V -- but we boot-block below AURORA_BAT_DEAD_MV
};

// ----------------------------------------------------------------------------
//  Timing constants
// ----------------------------------------------------------------------------
#define AURORA_HEARTBEAT_MS    5000    // Serial "alive" print interval
#define AURORA_OLED_REFRESH_MS 200     // Don't redraw OLED more than 5x/sec
#define AURORA_LED_REFRESH_MS  30      // LED animation frame interval

// Popup display time (WiFi activated, etc.)
#define AURORA_POPUP_DURATION_MS  3000

// ----------------------------------------------------------------------------
//  Color / status text presets (no RGB on chaser, but keep palette for OLED)
// ----------------------------------------------------------------------------
#define COLOR_AMBER        255, 120, 0
#define COLOR_AMBER_SOFT   255, 180, 80
#define COLOR_CREAM        255, 230, 200
#define COLOR_RAINBOW(r,g,b) (uint32_t)((r) << 16 | (g) << 8 | (b))

// ----------------------------------------------------------------------------
//  Debug helpers
// ----------------------------------------------------------------------------
#ifndef AURORA_DEBUG_LOG
#define AURORA_DEBUG_LOG 1
#endif

// ----------------------------------------------------------------------------
//  WiFi AP (Block 4) -- softAP credentials, channel, max clients, IP
// ----------------------------------------------------------------------------
#define AURORA_AP_SSID         "Aurora"
#define AURORA_AP_PASS         "for-chandni"
#define AURORA_AP_CHANNEL      6
#define AURORA_AP_MAX_CONN     4
//  IP the C3 hands out as the AP gateway. Phone browsers land on
//  http://192.168.4.1/ after joining the AP.
#define AURORA_AP_IP           192, 168, 4, 1
#define AURORA_AP_NETMASK      255, 255, 255, 0

// ----------------------------------------------------------------------------
//  WebSocket (Block 4) -- push cadence
// ----------------------------------------------------------------------------
#define AURORA_WS_PATH         "/ws"
#define AURORA_WS_PUSH_MS      2000   // 0.5 Hz state broadcast to all clients

#if AURORA_DEBUG_LOG
  #define DBG_PRINT(x)    Serial.print(x)
  #define DBG_PRINTLN(x)  Serial.println(x)
  #define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DBG_PRINT(x)
  #define DBG_PRINTLN(x)
  #define DBG_PRINTF(...)
#endif

#endif // AURORA_CONFIG_H
