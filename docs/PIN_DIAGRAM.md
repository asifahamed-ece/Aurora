# Aurora — Pin Diagram Guide

> **IMPORTANT:** Read this BEFORE connecting hardware. The ESP32-C3 has strapping pins — wiring them wrong can prevent boot. The diagram below mirrors what's in `include/config.h` and matches the firmware exactly.

---

## Board Overview

```
                        ┌────────────────────────────┐
                        │       ESP32-C3 (Robu)      │
                        │  Supermini / DevKitM-1     │
                        │                            │
                        │  GND ●        3V3 ●        │  ← Power rails
                        │                            │
   Buttons ──────────►  │  GPIO0   GPIO1   GPIO2  ●  │  ◄─── WiFi button
                        │  (UP)    (SEL)   (WiFi)    │
                        │                            │
                        │  GPIO3   GPIO4   ...    ●  │  ◄─── Battery ADC
                        │  (ADC)   (LED)             │
                        │                            │
   I2C ──────────────►  │  GPIO6   GPIO7   GPIO10 ●  │  ◄─── Down button
                        │  (SDA)   (SCL)   (DOWN)    │
                        │                            │
                        └────────────────────────────┘
```

---

## Pin-by-pin wiring

### 1. I²C OLED (SSD1306 0.96")

```
   OLED module                    ESP32-C3
   ───────────                    ────────
   VCC  ─────────────────────────► 3V3     (red)
   GND  ─────────────────────────► GND    (black)
   SDA  ─────────────────────────► GPIO6  (yellow)
   SCL  ─────────────────────────► GPIO7  (green)
```

Most SSD1306 modules have on-board 4.7kΩ pull-ups on SDA/SCL — no external resistors needed.

---

### 2. LED chaser (single LED)

```
   ESP32-C3                     Breadboard
   ────────                     ──────────
   GPIO4 ──[220Ω]──► |◄─── LED anode
                            LED cathode ──► GND rail
```

* GPIO4 → 220Ω resistor → LED anode (long leg)
* LED cathode (short leg) → GND
* LED will **breathe** smoothly (PWM via LEDC channel 0)

---

### 3. Buttons (4× tactile, active-LOW)

Each button has 2 pins. One side goes to its GPIO, the other to GND. The internal pull-up keeps the pin HIGH; pressing the button shorts it LOW.

```
        ┌── pin A
   ●────┤   Tactile
        └── pin B
        │         │
        A         B
        │         │
        │         └────────► GND (common rail)
        │
        └─────► GPIO (per table below)
```

| Button | GPIO | Notes |
|---|---|---|
| UP     | **GPIO0**  | **External 10kΩ pull-up to 3V3 required** (no internal pull on GPIO0 of some C3 modules) |
| SELECT | **GPIO1**  | Internal pull-up enabled in firmware |
| DOWN   | **GPIO10** | Internal pull-up enabled in firmware |
| WiFi   | **GPIO2**  | Internal pull-up enabled. **Strapping pin — do NOT hold during reset!** |

⚠️ **Critical: GPIO2 strapping pin.** If you hold the WiFi button down while pressing RESET or plugging in USB, the ESP32-C3 may fail to boot. Let go of the button, then press it after boot is complete.

---

### 4. Battery monitor (voltage divider)

```
   LiPo 3.7V
     +
     │
     ├──[ R1: 100kΩ ]──┬──[ R2: 100kΩ ]── GND
     │                 │
     │              (mid-point)
     │                 │
     │                 └────────► GPIO3  (ADC1_CH3)
     │
   (not connected to GND here)
```

* R1 and R2 must be **100kΩ each** (1% tolerance ideal, 5% OK).
* GPIO3 reads V_battery / 2 (so a 4.0V battery reads ~2.0V at the pin).
* **Do not** connect LiPo directly to GPIO3 — you'll fry the ADC and possibly the chip.

---

### 5. Power rails (mandatory)

* **3V3 rail** ← ESP32-C3 `3V3` pin (or USB 5V → onboard 3.3V LDO)
* **GND rail** ← ESP32-C3 `GND` pin
* **Decoupling cap**: 470µF electrolytic across 3V3 ↔ GND, **physically close** to the C3 (within 5cm). Without this you may see OLED flicker or random resets.

---

## Complete breadboard layout (one screen view)

```
                            ┌─────────────────────────────────────────────┐
   ────── 3V3 RAIL ────────► │ ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  │
   ────── GND RAIL ────────► │ ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  ●  │
                            └─────────────────────────────────────────────┘
       │  │  │  │           │                │          │   │   │   │
       │  │  │  │           │                │          │   │   │   │
       │  │  │  │           │                │          │   │   │   └─[ BTN-DOWN ]── GND
       │  │  │  │           │                │          │   │   │
       │  │  │  │           │                │          │   │   └────[ BTN-SELECT ]── GND
       │  │  │  │           │                │          │   │
       │  │  │  │           │                │          │   └─────────[ BTN-UP ]── 10kΩ ── 3V3
       │  │  │  │           │                │          │
       │  │  │  │           │                │          └────────────[ BTN-WIFI ]── GND
       │  │  │  │           │                │
       │  │  │  │           │                └──────[220Ω]──|>|── GND   (LED chaser)
       │  │  │  │           │
       │  │  │  │           └─────────────────────── GND (OLED, all buttons)
       │  │  │  │
       │  │  │  └───── 100kΩ ──┬── 100kΩ ── GND
       │  │  │                  │
       │  │  │                  └────────────────► GPIO3   (battery ADC)
       │  │  │
       │  │  └───── GPIO10 (DOWN button)
       │  │
       │  └────── GPIO2 (WiFi button)   ⚠ strapping pin
       │
       └──────── GND rail (final common)

ESP32-C3 PIN CONNECTIONS (summary)
==================================
3V3  ──► 3V3 rail
GND  ──► GND rail
GPIO6 ──► OLED SDA
GPIO7 ──► OLED SCL
GPIO4 ──► 220Ω ──► LED anode
GPIO0 ──► BTN-UP    (also 10kΩ pull-up to 3V3)
GPIO1 ──► BTN-SELECT
GPIO10──► BTN-DOWN
GPIO2 ──► BTN-WIFI  (do NOT hold at reset)
GPIO3 ──► Battery divider mid-point
```

---

## Hardware checklist before you flash

- [ ] OLED VCC/GND/SDA/SCL wired to 3V3/GND/GPIO6/GPIO7
- [ ] LED on GPIO4 via 220Ω resistor, cathode to GND
- [ ] 4 buttons wired: each button has one leg to its GPIO, the other to GND
- [ ] 10kΩ pull-up from GPIO0 to 3V3
- [ ] Battery divider: LiPo+ → 100kΩ → (100kΩ → GND); mid-point → GPIO3
- [ ] 470µF electrolytic across 3V3 and GND, near the C3
- [ ] **No button is being held down** when you power on or press RESET
- [ ] All grounds are common (3V3 rail, GND rail, LiPo GND, OLED GND)

---

## Quick reference: pin → firmware name

| ESP32-C3 GPIO | Firmware define | Component |
|---|---|---|
| GPIO6 | `AURORA_OLED_SDA_PIN` | OLED SDA |
| GPIO7 | `AURORA_OLED_SCL_PIN` | OLED SCL |
| GPIO4 | `AURORA_LED_PIN` | LED chaser |
| GPIO0 | `AURORA_BTN_UP_PIN` | Button UP |
| GPIO1 | `AURORA_BTN_SELECT_PIN` | Button SELECT |
| GPIO10 | `AURORA_BTN_DOWN_PIN` | Button DOWN |
| GPIO2 | `AURORA_BTN_WIFI_PIN` | Button WiFi |
| GPIO3 | `AURORA_BAT_ADC_PIN` | Battery ADC |

If you ever need to change a pin, change **only the `#define` in `include/config.h`** — every module picks it up from there.
