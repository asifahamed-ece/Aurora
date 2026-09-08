# Aurora — Pin Diagram Guide

> **IMPORTANT:** Read this BEFORE connecting hardware. The ESP32-C3 has strapping pins — wiring them wrong can prevent boot. The diagram below mirrors what's in `include/config.h` and matches the firmware exactly.

---

## Board Overview

The firmware drives everything with just **2 tactile buttons**, the OLED, one LED and the battery ADC:

```
   ESP32-C3 (Robu Supermini / DevKitM-1)
   ─────────────────────────────────────
   3V3  ●           GND  ●          ← Power rails
   GPIO8 ──► OLED SDA
   GPIO9 ──► OLED SCL
   GPIO4 ──[220Ω]──► LED anode   (LED cathode → GND)
   GPIO0 ───► BTN-TOUCH   (Warm Touch)   → also 10kΩ pull-up to 3V3
   GPIO2 ───► BTN-MULTI   (short = mode cycle / 3s hold = WiFi toggle)
   GPIO3 ───► battery voltage divider mid-point
```

---

## Pin-by-pin wiring

### 1. I²C OLED (SSD1306 0.96" or SH1106 1.3")

```
   OLED module                    ESP32-C3
   ───────────                    ────────
   VCC  ─────────────────────────► 3V3     (red)
   GND  ─────────────────────────► GND    (black)
   SDA  ─────────────────────────► GPIO8  (yellow)
   SCL  ─────────────────────────► GPIO9  (green)
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
* LED **breathes** smoothly (PWM via LEDC channel 0)

---

### 3. Buttons (2× tactile, active-LOW)

Each button has 2 pins; one side goes to its GPIO, the other to GND. The internal pull-up keeps the pin HIGH; pressing the button shorts it LOW.

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

| Button | GPIO | Action | Notes |
|---|---|---|---|
| Warm Touch | **GPIO0** | Pets the deskmate / records a Warm Touch | **External 10kΩ pull-up to 3V3 required** (no internal pull on GPIO0 of some C3 modules) |
| Mode / WiFi | **GPIO2** | Short press = cycle OLED modes; 3s hold = toggle WiFi AP | Internal pull-up enabled. **Strapping pin — do NOT hold during reset!** |

> ⚠️ **Critical: GPIO2 is a strapping pin.** Hold the Mode/WiFi button while pressing RESET (or plugging in USB) and the ESP32-C3 may fail to boot. Let go and press it only after boot.

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
```

* The divider halves the battery voltage so GPIO3 never sees LiPo's full 3.7V+.
* **Never** connect the LiPo positive directly to GPIO3 — you'll fry the ADC and possibly the chip.

---

### 5. Power rails (mandatory)

* **3V3 rail** ← ESP32-C3 `3V3` pin (or USB 5V → onboard 3.3V LDO)
* **GND rail** ← ESP32-C3 `GND` pin
* **Decoupling cap**: 470µF electrolytic across 3V3 ↔ GND, **physically close** to the C3 (within 5cm). Without it you may see OLED flicker or random resets.

---

## Complete breadboard layout (one screen view)

```
       3V3 RAIL ────────►    (LED, plus GPIO0 pull-up and OLED VCC)
       GND RAIL ────────►    (LED cathode, both buttons, OLED GND)
       │
       │  └── 100kΩ ──┬── 100kΩ ── GND
       │               │
       │               └────────────► GPIO3  (battery ADC)
       │
       └── GPIO8 ──► OLED SDA
       └── GPIO9 ──► OLED SCL
       └── GPIO4 ──[220Ω]──► |>|── GND   (LED chaser)
       └── GPIO0 ──► BTN-TOUCH (pin A)      pin B → GND ; 10kΩ pull-up → 3V3
       └── GPIO2 ──► BTN-MULTI (pin A)      pin B → GND   ⚠ strapping pin
```

**ESP32-C3 PIN CONNECTIONS (summary)**
```
3V3    ──► 3V3 rail
GND    ──► GND rail
GPIO8  ──► OLED SDA
GPIO9  ──► OLED SCL
GPIO4  ──► 220Ω ──► LED anode
GPIO0  ──► BTN-TOUCH   (Warm Touch; also 10kΩ pull-up to 3V3)
GPIO2  ──► BTN-MULTI   (short = OLED mode cycle / 3s hold = WiFi toggle; do NOT hold at reset)
GPIO3  ──► Battery divider mid-point
```

---

## Hardware checklist before you flash

- [ ] OLED VCC/GND/SDA/SCL wired to 3V3/GND/GPIO8/GPIO9
- [ ] LED on GPIO4 via 220Ω resistor, cathode to GND
- [ ] 2 buttons wired: each button one leg to its GPIO, the other to GND
- [ ] 10kΩ pull-up on GPIO0 → 3V3
- [ ] Battery divider: LiPo+ → 100kΩ → (100kΩ → GND); mid-point → GPIO3
- [ ] 470µF electrolytic across 3V3 and GND, near the C3
- [ ] No button held when powering on / pressing RESET
- [ ] All grounds common (3V3 rail, GND rail, LiPo GND, OLED GND)

---

## Quick reference: pin → firmware name

| ESP32-C3 GPIO | Firmware define | Component |
|---|---|---|
| GPIO8 | `AURORA_OLED_SDA_PIN` | OLED SDA |
| GPIO9 | `AURORA_OLED_SCL_PIN` | OLED SCL |
| GPIO4 | `AURORA_LED_PIN` | LED chaser |
| GPIO0 | `AURORA_BTN_TOUCH_PIN` | Touch button (Warm Touch) |
| GPIO2 | `AURORA_BTN_MULTI_PIN` | Mode-cycle / WiFi-toggle button |
| GPIO3 | `AURORA_BAT_ADC_PIN` | Battery ADC |

If you ever need to change a pin, change **only the `#define` in `include/config.h`** — every module picks it up from there.
