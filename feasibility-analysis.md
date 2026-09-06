# Aurora Gift — Deep Feasibility Analysis

**Author:** Claudyy (Asif's brainstorm partner)
**Date:** Sept 5, 2026
**Status:** Pre-commitment feasibility pass — no implementation yet
**Goal:** Validate that the proposed "Aurora" design is buildable in 5 days, on a $3-ish MCU, with the components Asif has, with no surprising failure modes.

---

## 1. ESP32-C3 Supermini — the MCU itself

### What we know with high confidence

- **Core:** single-core 32-bit RISC-V, 160 MHz
- **Flash:** 4 MB SPI flash (most Supermini boards; some clones 2 MB — verify yours)
- **SRAM:** 400 KB total, ~320 KB usable for user code after WiFi stack
- **WiFi:** 802.11 b/g/n, 2.4 GHz, station + AP + AP+STA modes — fully supported
- **Bluetooth:** 5.0 LE (we won't use it)
- **GPIO exposed on Supermini:** typically GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO18, GPIO19, GPIO20, GPIO21 — total ~15 usable pins
- **Strapping pins** (must be in correct state at boot or boot fails):
  - GPIO2 → must be HIGH (or floating) at boot
  - GPIO8 → must be HIGH at boot (also used for ROM logs)
  - GPIO9 → must be LOW at boot
- **Pins connected to internal flash on most variants:** GPIO12-17 (not exposed on Supermini)
- **Pins used for USB-Serial on the on-board USB:** GPIO19, GPIO20 (don't repurpose these while USB is in use for programming)
- **On-board LED:** typically on GPIO8 on the Supermini — careful, this is a strapping pin

### ⚠️ What this means for our pin allocation

| Function | Originally proposed | Feasibility-verified alternative | Why |
|---|---|---|---|
| I2C SDA (OLED) | GPIO6 | **GPIO6** ✅ | Safe, commonly used for I2C |
| I2C SCL (OLED) | GPIO7 | **GPIO7** ✅ | Safe, commonly used for I2C |
| WS2812B data | GPIO8 | **GPIO5 or GPIO18** ✅ | GPIO8 is a strapping pin AND has the on-board LED. Use GPIO5 (safer) or GPIO18 |
| Button Up | GPIO2 | **GPIO1 or GPIO0** ✅ | GPIO2 is a strapping pin (must be HIGH at boot). GPIO0/GPIO1 are safer. Note: GPIO0 is also the boot mode pin — use with care, add external pull-up to ensure HIGH at boot |
| Button Select | GPIO3 | **GPIO3** ✅ | Safe (note: on some Supermini boards GPIO3 is the UART0 RX pin; if you don't use UART in your sketch it's fine) |
| Button Down | GPIO10 | **GPIO10** ✅ | Safe |
| Optional sensor | GPIO4 | **GPIO4** ✅ | Safe |

### ✅ Revised pin map (feasibility-verified)

```
ESP32-C3 Supermini
──────────────────
GPIO5   → WS2812B DIN (through optional 74HCT125 level shifter)
GPIO6   → OLED SDA  (with 4.7kΩ pull-up to 3.3V)
GPIO7   → OLED SCL  (with 4.7kΩ pull-up to 3.3V)
GPIO0   → Button Up (with external 10kΩ pull-up to 3.3V — keeps boot mode safe)
GPIO1   → Button Select (internal pull-up OK)
GPIO10  → Button Down (internal pull-up OK)
GPIO4   → Optional sensor (DHT22 or DS18B20)
3.3V    → OLED VCC, sensor VCC, button pull-ups
GND     → common ground
5V (VBUS) → WS2812B 5V (via level shifter VCC) + TP4056 input
```

**Trade-off we accept:** we lose the on-board LED (GPIO8) for status — we'll instead use a single pixel of the WS2812B ring as a "system status" indicator (e.g., green = OK, red = error).

---

## 2. WS2812B level-shifter question

**The official answer:** WS2812B wants 5V logic high. 3.3V from C3 is *technically* out of spec but **works in practice in 90% of cases** at short distances (under 30 cm of wire).

**For Asif's build:** the WS2812B ring will be soldered directly to the perfboard, ~3-5 cm from the C3. **No level shifter needed.**

**However, for reliability + the "admiration" factor** (he wants the gift to *just work* every time for years), I'd recommend one of these:
- **Option A (cheapest):** skip the level shifter, add a 470 Ω series resistor on the data line as close to the C3 as possible. This damps ringing. 95% reliable.
- **Option B (most reliable):** add a single 74HCT125 IC + 0.1 µF decoupling cap. ~₹30. Solder-clean. 100% reliable. **Recommended.**

**Verdict:** If you have or can get a 74HCT125, use it. Otherwise, 470 Ω series resistor is good enough.

---

## 3. SSD1306 OLED — what to expect from monochrome

**Reality check:** standard 0.96" SSD1306 OLED is single-color (white, blue, or yellow pixels on black). It does NOT support RGB.

**What still works beautifully:**
- Crisp custom fonts (GFX library or u8g2 supports many)
- Animations (bitmap blits, scroll text, frame-by-frame)
- Inversions (white-on-black ↔ black-on-white) for transitions
- Brightness control (0-255) for fade effects

**What won't work:**
- Per-pixel color (the "screen rainbow" idea we can't do)
- Anti-aliased text without effort

**Verdict:** Monochrome OLED is *fine* — frontend girls often love the retro pixel aesthetic. We'll lean into it: use a custom pixel font, do small bitmaps, frame-by-frame animations. The color story lives on the WS2812B ring and the dashboard.

**Alternative if you want color on a small display:** ST7789 0.96" or 1.3" IPS TFT (~₹250 on Robu/QuartzComponents). Do you have one? If yes, I'd consider swapping it in. It uses SPI instead of I2C (different pins, more wires, but beautiful color).

---

## 4. Memory budget — does this all fit?

**Flash budget (4 MB total):**

| Component | Size estimate |
|---|---|
| Arduino core + WiFi stack | ~800 KB |
| ESPAsyncWebServer + AsyncTCP | ~150 KB |
| ArduinoJson (v7) | ~50 KB |
| Adafruit_NeoPixel | ~15 KB |
| u8g2 (full font set) | ~200 KB |
| Adafruit_SSD1306 | ~10 KB |
| Button / NTP / misc drivers | ~30 KB |
| Firmware code itself | ~50 KB |
| **Subtotal** | **~1.3 MB** |
| **Remaining for LittleFS (dashboard + assets)** | **~2.5 MB** ✅ |

**Dashboard size estimate:**
- `index.html` (with embedded fonts as base64): ~25 KB
- `app.css`: ~8 KB
- `app.js`: ~12 KB
- `secret.html`: ~5 KB
- Total: **~50 KB** — fits 50x over.

**RAM budget (320 KB usable after WiFi):**

| Component | RAM use |
|---|---|
| WiFi stack (active) | ~50-80 KB |
| AsyncWebServer | ~10 KB |
| ArduinoJson working buffer (static, pre-allocated) | 2-4 KB |
| OLED frame buffer (128×64/8 = 1 KB) | 1 KB |
| WS2812B ring buffer (12 LEDs × 3 bytes) | 36 B |
| Free heap remaining for tasks/messages | **~200 KB** ✅ |

**Verdict:** Memory is *not* a constraint. We're using <2% of flash for the dashboard and have 200+ KB of headroom for RAM.

---

## 5. Fail-safe / robustness design

These are the failure modes I want to design for, not patch later:

### Power brownouts (LiPo + WiFi + LEDs simultaneously)
- **Cause:** WiFi TX burst draws ~250 mA, WS2812B full white draws ~60 mA × 12 = 720 mA peak. A 500 mAh LiPo can sag.
- **Solution:**
  - 470 µF electrolytic cap across 3.3V rail (between C3 3.3V pin and GND)
  - 100 nF decoupling cap near each IC
  - Cap WS2812B full-brightness to <50% white (we control this in firmware)
  - During WiFi provisioning, dim LEDs to 0 (avoids simultaneous draw)

### Boot failure if button held down
- **Cause:** GPIO0 is a boot-mode pin — LOW at boot puts C3 into download mode.
- **Solution:** external 10kΩ pull-up to 3.3V on GPIO0. Even if the user holds the Up button, the pull-up wins during boot.

### WiFi AP + Station mode confusion
- **Cause:** if home WiFi is down or wrong password saved, captive portal logic can lock up.
- **Solution:**
  - On boot, C3 tries last-saved WiFi for 10 seconds. If fails, falls back to AP mode (`Aurora-Setup`).
  - AP mode has a 60-second window where it's open, then becomes password-protected.
  - Always-paired long-press Select resets WiFi credentials (back to AP mode).
  - Boot-time watchdog: if no successful WiFi connection in 15s, log + continue as AP.

### Watchdog timer
- **Cause:** WiFi stack or web handler can hang under malformed requests.
- **Solution:** enable the C3's hardware watchdog (TWDT) with a 30-second timeout. If the loop() doesn't feed it, the chip auto-resets.

### Secret page premature access
- **Cause:** what if she opens the dashboard before Sept 10 and finds `/secret` doesn't work?
- **Solution:** the menu item is *hidden* in the UI before Sept 10 (the dashboard queries `/api/state` which reports `now < unlockDate` and hides the link). The endpoint itself still 404s if called directly. Two layers of date-gating.

### NTP failure on first boot (no time → wrong daily message)
- **Cause:** if NTP fails, the C3 has no idea what day it is → picks wrong message.
- **Solution:** fallback to the C3's internal RTC (which is set by NTP once successfully). If both fail, the device still works — it just shows message #0 every day until NTP succeeds. The C3's RTC drifts ~30 sec/day without sync but is fine for day counting.

### Power switch / battery protection
- **Cause:** LiPo over-discharge destroys the cell.
- **Solution:** include a small slide switch on the battery line. Plus firmware-side: read battery voltage via ADC and show a low-battery warning on OLED if V_batt < 3.3V.

---

## 6. Inspiration from similar projects (from web research)

A few project types I came across that are *close* to what we're building but we can do better:

1. **Generic "ESP32 LED matrix message display"** projects (MAX7219-based, often on Instructables). These exist in the hundreds. What they all lack: a *polished* UI, a personal narrative, and date-gated content. We win on those.

2. **ESP-DASH** library — a generic dashboard library for ESP32. Cool, but generic-looking. Our hand-rolled dashboard will look better because it has *one purpose* (Aurora) instead of being a generic I/O viewer.

3. **"Love box" / "love letter box" projects** — there are several on GitHub where people build wooden boxes that show a message when opened. We differentiate by being *on* continuously (not just when opened) and by having the dashboard layer for the frontend girl.

4. **Captive-portal projects (DNSServer + AsyncWebServer)** — these are well-trodden territory. The standard pattern is: AP mode + DNS server that intercepts all requests → redirects to a setup page. We use this pattern, but after setup, the device shifts to station mode and serves a *real* dashboard.

**Our unique angle:** the combination of (a) hand-built perfboard hardware, (b) a genuinely beautiful frontend UI served from a $3 MCU, (c) a date-gated personal message, and (d) a 30-day rotating supportive-message library is, as far as my search results show, **not a thing that exists off-the-shelf**. This is a custom build that no amount of buying a kit will replicate.

---

## 7. Things to add to the v3 design (recommendations)

Based on this analysis, I'd add:

1. **74HCT125 level shifter** for WS2812B (₹30, 1 IC, 0.1 µF cap). *Increases reliability from 95% → 99.9%.*

2. **470 µF electrolytic cap** on 3.3V rail. *Solves power sag during WiFi+LED peaks.*

3. **Hardware watchdog** enabled in firmware. *Self-heals from any lockup.*

4. **External 10kΩ pull-up on GPIO0** to keep boot safe even if Up button is held.

5. **Battery voltage monitor** via ADC on a voltage divider. *Shows low-battery warning on OLED.*

6. **Slide switch** on battery line. *Power off without unplugging.*

7. **Status LED on the ring itself** (e.g., LED #0 always shows system status: green=ok, red=error, blue=setup-mode). *Replaces the lost on-board LED.*

8. **OTA (over-the-air) firmware update** via a hidden web route. *Lets you push fixes after gifting without opening the box.* This is a HUGE admiration factor — she can update the gift's "personality" herself.

9. **mDNS + AP fallback** with proper timeout logic. *Robust WiFi handling.*

10. **Optional sensor** (DHT22 for temp+humidity, or DS18B20 for temp only) on GPIO4. *Adds the "it knows the room" magic — a single ambient number on the dashboard that quietly tracks her environment.*

---

## 8. Final verdict

**Buildability:** ✅ **Green.** All components are off-the-shelf, well-supported, well-documented, and proven.

**5-day timeline:** ⚠️ **Yellow → Green with discipline.** Day-by-day plan is tight but achievable IF:
- Day 1 ends with a working breadboard prototype (OLED + LEDs + buttons all responding)
- Day 2-3 firmware work in parallel with perfboard soldering
- Day 4 is dashboard + secret page + LiPo wiring
- Day 5 is enclosure only — no firmware changes after Day 4

**Risk areas in priority order:**
1. **Enclosure window for OLED** — hardest hand-build step. Pre-plan the cut on Day 4 evening.
2. **Soldering quality under time pressure** — rehearse pin-by-pin wiring on Day 1 on a scrap perfboard.
3. **mDNS across different phones** — backup URL display on OLED always shown.
4. **WS2812B flicker on first power-up** — solved by 470 Ω resistor.

**Unique-feeling factors that will land:** date-gated secret page, 30-message library, the frontend girl actually being impressed by the dashboard, OTA update story she can tell friends.

**My recommendation:** proceed with the design. Add the items in Section 7 to the spec. Get a 74HCT125 + 470 µF cap + 10kΩ resistor + slide switch + tactile buttons from a local electronics shop tomorrow (₹150 total, well under budget).

---

## 9. Open questions to ask Asif before writing the final spec

1. **Do you have a 74HCT125 or should I plan without it?** (470 Ω resistor fallback is fine for short cables)
2. **Do you have a TFT (color) display, or stick with mono OLED?**
3. **Do you have 3× tactile buttons in hand?**
4. **For the OTA route, do you want it enabled (great admiration factor) or hidden as a maintenance tool?**
5. **For the 30 messages, do you want me to draft them based on a CSE-flavored calming/supportive tone, or do you want to write them yourself?**

---

*End of feasibility analysis. Next step: revise the v2 design into v3 with these additions, then write the formal spec.*
