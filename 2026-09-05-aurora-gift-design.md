---
title: "Aurora — A Birthday Gift Design Spec"
date: 2026-09-05
status: ✅ SHIPPED & DELIVERED (updated 2026-09-12)
author: Claudyy (brainstorm partner with Asif, ECE final-year)
---

# Aurora — Birthday Gift Design Spec

## Context

**Who:** Asif (ECE final-year) is building a DIY birthday gift for a girl he likes from CSE.
**When:** Her birthday is Sept 10, 2026 — **5 days from this spec's date**.
**Why this is meaningful:** His previous gift (a heart-shaped 555-timer breathing LED on a custom PCB in a hand-built box) was well-received. This gift must be a clear engineering and emotional step up — using the ESP32-C3 Supermini he already owns, with WiFi capability, a local dashboard, and a more sophisticated narrative.
**Emotional target:** admiration ("he built *this* for me?") paired with deep respect, with a one-shot soft confession as the private layer that becomes visible on/after Sept 10. The 30 daily messages are calming/supportive (not romantic). The /secret page is a single honest note, not a year-long campaign. Frame: "I made this because you're worth making things for, and I wanted to be honest with you before we graduate."
**Recipient's interests:** Web/Frontend/UI-UX (she'll appreciate the dashboard, the typography, the animations).
**Form factor:** small hand-built enclosure, perfboard prototype, no 3D printer available.

The intended outcome: a working, polished, robust gift delivered on Sept 9 (one day buffer before the birthday). On unboxing, the gift is impressive on first sight; on first interaction, the recipient is delighted by the dashboard quality; on discovery, the date-gated personal message lands.

---

## Delivery status — ✅ built, delivered (updated Sept 12, 2026)

| Item | Status |
|---|---|
| **Components** | ✅ **All purchased** — every line of the BOM below is in hand |
| **Firmware — Block 1 (deskmate engine)** | ✅ Complete & verified — living OLED deskmate, moods, LED chaser, battery ADC |
| **Firmware — Block 4 (WiFi + dashboard)** | ✅ Complete & verified — SoftAP `Aurora`, AsyncWebServer + WebSocket, LittleFS dashboard |
| **The gift** | ✅ Built & delivered in time for her **Sept 10** birthday |
| **Live dashboard preview** | ✅ Hosted at [startling-cuchufli-ccb77c.netlify.app](https://startling-cuchufli-ccb77c.netlify.app/) |

> **The build evolved while it was made.** The device that shipped is the **"Aurora Deskmate Edition"** — an animated OLED desk companion with a single touch button and a breathing LED, replacing this spec's original 3 buttons + NeoPixel ring. The core intent (hand-built, WiFi dashboard, 30 daily messages, warmth without pressure) shipped as designed; the "as-built" details live in [IMPLEMENTATION_PROGRESS.md](IMPLEMENTATION_PROGRESS.md), [docs/PIN_DIAGRAM.md](docs/PIN_DIAGRAM.md) and [`include/config.h`](include/config.h).

---

## High-level concept

A small hand-built box containing:
- An ESP32-C3 Supermini
- A mood-synced breathing LED (single, GPIO4, via 220Ω resistor)
- A 1.3" SH1106 monochrome OLED display (128×64, U8g2; also SSD1306-compatible)
- 2 tactile buttons: Warm-Touch (GPIO0) + Mode/WiFi (GPIO2)
- A 3.7V LiPo + TP4056 charging module + slide switch
- 470 Ω series resistor + 10kΩ pull-up (fail-safe components)

The device shipped as the **Aurora Deskmate Edition** — an animated OLED companion with **6 display modes** cycled by the Mode button (short press):
1. **Deskmate** (default) — living animated pet: blinking eyes, heart-eyes on touch, lonely/sad after 1 hour neglect, 12:00 AM daily-message reminder, sleeping.
2. **Clock & Date** — day, date, live time.
3. **Daily Quote** — one of 30 daily calming/supportive messages (`dayOfYear % 30`).
4. **Scan WiFi QR** — on-OLED QR code to join the `Aurora` hotspot.
5. **Scan Dashboard QR** — on-OLED QR code for `http://192.168.4.1/`.
6. **Heartbeat Keepsake** — animated ECG pulse line + accelerating lub-dub heart.

A 3-second hold on the Mode button toggles the WiFi softAP. The original clock/interaction/WiFi-setup 3-mode concept below was superseded by this design during the build.

---

## Hardware

### Bill of materials

| Component | Source | Status |
|---|---|---|
| ESP32-C3 Supermini | Owned | ✅ Purchased |
| 12× WS2812B ring (or 8-pixel if that's what he has) | Owned | ✅ Purchased *(superseded in shipped build — see As-built note)* |
| 0.96" SSD1306 OLED (I2C) | Owned | ✅ Purchased |
| 3× 6mm tactile buttons | Owned | ✅ Purchased *(consolidated to 2 in shipped build)* |
| 3.7V LiPo (500-1000 mAh) | Owned | ✅ Purchased |
| TP4056 charging module | Owned | ✅ Purchased |
| 470 µF electrolytic capacitor | Purchased (₹10) | ✅ Purchased |
| 10 kΩ resistor (for GPIO0 pull-up) | Purchased (₹2) | ✅ Purchased |
| 470 Ω resistor (for data line) | Purchased (₹2) | ✅ Purchased |
| Slide switch (SPST) | Purchased (₹15) | ✅ Purchased |
| 4× M3 brass standoffs (15mm) | Purchased (₹40) | ✅ Purchased |
| 3mm clear acrylic sheet (200mm × 150mm) | Purchased (₹100) | ✅ Purchased |
| Black acrylic or thin plywood (for box body) | Purchased (₹100) | ✅ Purchased |
| M3 screws (8×) | Purchased (₹20) | ✅ Purchased |
| Perfboard (5×7 cm or similar) | Purchased (₹30) | ✅ Purchased |

**Total cost of new components: ~₹300** (~$3.50) — ✅ **fully spent; nothing left on the list.**

### Final pin map (feasibility-verified)

```
ESP32-C3 Supermini
──────────────────────────────
GPIO5   → WS2812B DIN  (via 470Ω series resistor)
GPIO6   → OLED SDA     (4.7kΩ pull-up to 3.3V)
GPIO7   → OLED SCL     (4.7kΩ pull-up to 3.3V)
GPIO0   → Button Up    (10kΩ external pull-up to 3.3V; internal pull-up NOT used)
GPIO1   → Button Select (internal pull-up enabled)
GPIO10  → Button Down  (internal pull-up enabled)
3.3V    → OLED VCC, button pull-ups
GND     → common ground
5V (VBUS) → TP4056 IN, WS2812B 5V
Battery+ → TP4056 BAT+
470 µF  → across 3.3V and GND (near C3)
```

**Rationale for pin choices:**
- GPIO0: External 10kΩ pull-up ensures the C3 boots correctly even if the Up button is held during reset (GPIO0 is a boot-mode pin).
- GPIO5 for WS2812B: GPIO8 was the originally-suggested pin but it's a strapping pin AND the on-board LED. GPIO5 is safe.
- GPIO1 and GPIO10 are safe for buttons with internal pull-ups.

### As-built pin map (Deskmate Edition — matches `include/config.h` exactly)

The map above was superseded during the Deskmate Edition consolidation. The pins actually wired and flashed:

```
GPIO8 → OLED SDA    GPIO9 → OLED SCL        (I2C 400 kHz, SSD1306 or SH1106)
GPIO4 → LED chaser (single breathing LED, 220Ω in series)
GPIO0 → Warm-Touch button (external 10kΩ pull-up to 3V3)
GPIO2 → Mode-cycle button — short press = next screen / 3s hold = WiFi AP toggle
GPIO3 → battery voltage-divider mid-point (2× 100kΩ, ADC1_CH3)
470µF → across 3V3 and GND, physically close to the C3
```

Verification: [docs/PIN_DIAGRAM.md](docs/PIN_DIAGRAM.md).

---

## Firmware architecture

### Tech stack
- **Framework:** Arduino (ESP32 Arduino core, PlatformIO `espressif32`)
- **Libraries (from `platformio.ini`):**
  - `olikraus/U8g2` (OLED — runs both SH1106 and SSD1306)
  - `ESPAsyncWebServer-aurora` (vendored, patched for RISC-V heap safety) + `AsyncTCP-esphome`
  - `bblanchon/ArduinoJson` v6 (state API)
  - `ricmoo/QRCode` (on-OLED QR generation)
  - `WiFi` + `Preferences` (built-in; no `NTPClient` — the clock is seeded from the compile-time epoch and advanced by `millis()`)
- **Storage:** LittleFS for the dashboard
- **OTA:** Enabled over the softAP, token-secured via `AURORA_OTA_KEY` (reversed from "disabled")

### Operating model (as-built)

The original three modes were superseded during the build by the Deskmate Edition's **two buttons + six screens**:

| Button | Action |
|---|---|
| **Warm-Touch (GPIO0)** — short press | Records a Warm Touch; wakes the deskmate and flips it into excited heart-eyes + accelerated heartbeat |
| **Mode (GPIO2)** — short press | Cycles the 6 screens: Deskmate → Clock & Date → Daily Quote → Scan WiFi QR → Scan Dashboard QR → Heartbeat Keepsake |
| **Mode (GPIO2)** — 3-second hold | Toggles the WiFi softAP on/off (`Aurora` / `for-chandni`, `192.168.4.1`) |

- **Attention / neglect:** 1 hour without a touch → `LONELY_SAD` (droopy eyes, teardrop, "Miss you... Touch me?").
- **Midnight:** 12:00 AM triggers an animated envelope reminder to read the day's message on the dashboard.
- **WiFi (shipped):** always-available softAP with captive portal + dashboard; no home-WiFi station mode and no mDNS in the shipped build.

### Daily message library
- 30 messages stored in `src/block4/messages.cpp` (flash-resident); the dashboard loads its own copy from `data/messages.js`
- Selected by `dayOfYear % 30`
- Tone: CSE-flavored, calming, supportive, identity-affirming — **the kind of things a person who genuinely cares about her would want her to hear on a hard day.** Warm but not presumptuous. They can show care and even affection, but they don't assume a future together.
- Length: ~10-20 words each
- Asif drafts these in-session (see Build Timeline)

**Tone guidance — what to include:**
- Acknowledgement of her craft, her effort, her intelligence, her growth
- Permission to fail, rest, change direction, outgrow things
- Quiet confidence boosts
- A sense of *"someone in your corner, watching you with warmth"*
- References that show you know her — her field, her pace, the way she works
- The occasional line that says *"I'm proud of you"* without being saccharine

**Tone guidance — what to avoid:**
- Possessive language ("you belong to someone like me")
- Future-presuming language ("we'll always have...", "when we're old")
- Courtship cliché ("you're the most beautiful...")
- Anything that sounds like a transaction ("I did this so you'd...")
- Daily pressure to feel a certain way

**The point:** the 30 messages are *her daily companion* for as long as she keeps Aurora on her desk. They should feel like a steady, warm presence — not like a slow-motion ask. The one-shot note shipped as the keepsake **letter jar** + birthday letter in the dashboard; the date-gated `/secret` page was superseded during the build. Everything else just says *"you matter, and I'm glad I knew you."*

### Watchdog
- **Not in the shipped build** (`esp_task_wdt` was dropped) — stability comes from the AsyncWebServer heap-safety patch (pre-allocated request buffer) instead.

### Battery monitoring
- ADC read on a voltage divider (2× 100kΩ) from battery+ → GPIO3 (ADC1_CH3)
- Thresholds (see `AURORA_BAT_*` in `include/config.h`): `OK` ≥ 3.6 V · `LOW` < 3.4 V · `CRITICAL` < 3.1 V · boot-blocked below 2.8 V
- Status is shown on the dashboard (`/api/state`) and reflected in the deskmate's mood

### Power-on behavior
- Slide switch controls battery line
- Boot time: < 2 seconds to the Deskmate screen (default screen)
- State (warm touches, last-touch time, clock epoch, midnight acknowledgement) persists in Preferences (NVS)

---

## Web dashboard (served from C3's LittleFS)

### File structure

```
/data/
├── index.html          (main dashboard — self-contained, CSS/JS inlined, includes the letters + music box)
├── messages.js         (the 30 daily thoughts, loaded by the dashboard)
└── sleeping_chandni.jpg (Dreamland photo card)
```

Total: ~80 KB. Trivially fits in 4 MB flash.

### Visual design language

- **Theme:** light "strawberry-milk" (background gradient white → `#ffd2dc`, text `#461628`)
- **Color palette:** blush pink + cream + rose accents (`#ff5c8a`, `#ff3366`), `theme-color #ffe5ec`
- **Typography:** system font stack + rounded display font for headings
- **Layout:** single column on mobile, max-width centered
- **Background:** CSS-only floating hearts/petals (all generated in the browser)

### Dashboard components (`index.html`)

1. **Today** — greeting, live time, today's daily message, mood, battery, touch stats
2. **Letters** — keepsake envelope jar of short personal letters (incl. "Open when you can't sleep at 2 AM")
3. **Dreamland** — resting photo card + quiet note + integrated Music Box (Web Audio, browser-generated)
4. **Birthday mode** — auto-celebrated when the clock hits Sept 10
5. **Footer** — small "made with care" credit + version number

### API endpoints

```
GET  /api/state         → JSON snapshot (time, message, stats, battery, network) for first paint
GET  /version           → firmware version string
WS   /ws                → push socket — state broadcast ~every 2 s + live touch sync
GET  /update, POST /update → Web OTA firmware reflash (hotspot only)
POST /upload            → LittleFS upload (dashboard asset OTA)
GET  /generate_204, /hotspot-detect.html → silent captive-portal detection
onNotFound / fallback   → unknown routes serve the dashboard (captive-portal "login" landing)
```

### Live-update strategy
- Dashboard fetches `/api/state` once for first paint, then receives live updates over the WebSocket (`/ws`, ~2 s broadcasts)
- Touches sent from the dashboard are mirrored on the physical OLED in real time

### Gating in the shipped build
- The `/secret` date-gating below was **superseded**: content is surfaced as the always-open **keepsake letter jar**, a **music box**, and automatic **birthday mode** when the clock hits Sept 10, plus a 12:00 AM daily-message reminder on the OLED.

### Content of the /secret page (soft-confession)
*(In the shipped build this landed as the keepsake letter jar + birthday letter in the dashboard; the voice guidance below carried over to those letters.)*
- **Type:** a single, short, honest note. One page. ~150-250 words.
- **Voice:** Asif's actual voice — not flowery, not theatrical. Quiet.
- **Tone:** honest about his feelings, clear that he doesn't expect anything back, graceful about the future being unknown.
- **Structure (suggested):**
  1. Open with the fact: *"I like you. I have for a while. I wanted to say it once, clearly, before we graduate and our lives go in different directions."*
  2. Acknowledge reality without apology: *"You don't feel the same way, and that's okay. I wanted you to know the truth from me, not guess at it."*
  3. Reframe the gift: *"Aurora is not a way of asking you to change your mind. It's a way of saying thank you for being someone worth paying attention to."*
  4. Close with grace: *"Whatever we are after this — friends, occasional check-ins, two people who once shared a college corridor — I'm glad it happened. Take care of yourself. — Asif"*
- **What to avoid:**
  - Pressure to respond a certain way
  - A "but maybe..." negotiation
  - Promises about the future ("I'll wait for you")
  - An implicit ask ("so... can we try?")
- **Final form:** the page should feel like closure, not an opening move. It should leave her feeling *honored*, not *pressured*.

---

## Enclosure design (hand-built, no 3D printer)

### Structure
- **Top panel:** 3mm clear acrylic, laser-cut or hand-scored, with a rectangular cutout for the OLED window (~28mm × 16mm) and circular cutouts for the 2 buttons (~6mm diameter each)
- **Side panels:** black acrylic (3mm) or thin plywood, cut to 80mm × 30mm × 4 pieces, glued or screwed together
- **Bottom panel:** black acrylic, with a cutout for the slide switch and a micro-USB access hole
- **Standoffs:** 4× M3 brass standoffs (15mm) screwed between top and bottom panels, with the perfboard mounted on top of the standoffs

### Mounting strategy
- **OLED:** hot-glued to underside of top panel, aligned with the cutout
- **Buttons:** soldered to perfboard, button caps protrude through top panel cutouts
- **Breathing LED:** soldered directly to the perfboard, glowing out of the box (single LED, no diffuser needed)
- **Battery:** loose inside the box, secured with a dab of hot glue
- **TP4056:** mounted on the side, micro-USB accessible from the side of the box

### Estimated dimensions
- Outer: ~80mm × 50mm × 30mm (smaller than a coffee mug)
- Inner usable space: ~70mm × 40mm × 20mm

---

## Build timeline (5 days)

| Day | Date | Tasks |
|---|---|---|
| **Day 1** | Sept 5 (today) | ✅ Hardware confirmed. Buy remaining components (caps, resistors, switch, standoffs, acrylic). Build breadboard prototype: OLED + LEDs + 3 buttons all responding. Verify I2C scan, pin choices, basic NeoPixel test. |
| **Day 2** | Sept 6 | Firmware Mode 1 + Mode 2: clock, NTP, message rotation, button navigation, breathing LED animation. Rehearse soldering pin-by-pin on a scrap perfboard. |
| **Day 3** | Sept 7 | Firmware Mode 3: WiFi AP/station, mDNS, captive portal, dashboard JSON API. **Draft the 30 daily messages in-session with Claudyy.** Start perfboard soldering (parallel with firmware). |
| **Day 4** | Sept 8 | Dashboard HTML/CSS/JS. Date-gated secret page. LiPo + TP4056 + slide switch wiring. Test the full stack on a soldered perfboard. |
| **Day 5** | Sept 9 | Enclosure build: cut acrylic, mount components, button caps through holes, OLED window alignment. Final assembly. Rehearse the unboxing flow. Charge battery. |

### Actual outcome (shipped)

| Day | Planned | Shipped |
|---|---|---|
| **Day 1** | Components + breadboard prototype | ✅ All components bought; breadboard verified |
| **Day 2–4** | Firmware modes, dashboard, secret page | ✅ Deskmate Edition firmware + LittleFS dashboard, letters & music box |
| **Day 5** | Enclosure + final assembly | ✅ Hand-built enclosure, delivered for the **Sept 10** birthday |

### Risk mitigation
- Day 5 is buffer. No firmware changes after Day 4.
- Soldering happens on Day 3-4 in parallel with firmware — they meet on Day 4 evening.
- If enclosure is harder than expected, fall back to a simpler design (acrylic top + 4-sided wrap of cardboard + bottom).

---

## Verification (how to test end-to-end)

Before gifting, Asif must verify (shipped checklist):

1. **Power on from battery:** device boots to the Deskmate screen within 2s; the clock keeps time without NTP.
2. **Button navigation:** Warm-Touch (GPIO0) triggers heart-eyes; Mode button (GPIO2) cycles all 6 screens; 3-second hold toggles WiFi.
3. **LED animations:** breathing is smooth and mood-synced (800 ms touched / 1500 ms normal / 3200 ms lonely), no flicker.
4. **WiFi:** softAP `Aurora` appears, captive portal loads, `http://192.168.4.1/` serves the dashboard.
5. **Dashboard:** from the hotspot, the dashboard loads within 3 seconds, shows live time and today's message, updates live over WebSocket.
6. **Touch sync:** touches sent from the dashboard trigger the excited heart-eyes reaction on the physical OLED.
7. **Neglect & midnight:** leaving the device untouched for 1 hour shows the lonely mood; 12:00 AM shows the envelope reminder.
8. **OTA** over the hotspot: token-secured web reflash works.
9. **Battery life:** fully charged with WiFi + display running → device runs well beyond the workday.
10. **Brownout test:** rapid button presses during WiFi use → device must not crash.
11. **Rehearsal:** do a full unboxing rehearsal (open box, touch the deskmate, cycle screens, join the hotspot, open the dashboard, open the letters). The whole flow should take < 60 seconds and feel magical.

---

## Source of truth (as-built)

The files planned below now exist as a PlatformIO project (this repo):

| Planned | Actual in repo |
|---|---|
| `firmware/aurora.ino` | `src/main.cpp` — setup/loop: deskmate animation, touch & midnight checks |
| `firmware/state.h` | `include/state.h` + `src/block4/state.cpp` — NVS-backed state singleton |
| `firmware/display.h` / `leds.h` / `buttons.h` / `battery.h` | `include/display.h`, `include/chaser.h`, `include/buttons.h` + `src/block1/*` |
| `firmware/wifi_manager.h` / `dashboard_api.h` | `src/block4/*` — SoftAP, AsyncWebServer, AsyncWebSocket |
| `data/index.html`, `app.css`, `app.js` | `data/index.html` (self-contained) + `data/messages.js` |
| `data/secret.html` | keepsake letters + music box — inline in `data/index.html` |
| `MESSAGES.md` | `data/messages.js` — the 30 daily messages |
| `README.md` | `README.md` (overview) + `docs/USER_MANUAL.md` (the card) + `docs/PIN_DIAGRAM.md` |

---

## Out of scope (explicitly)

- OTA firmware updates (per Asif's choice — *since reversed: OTA is enabled in Block 4 over the `Aurora` AP, token-secured via `AURORA_OTA_KEY`*)
- Ambient sensor (per Asif's choice)
- Mobile app (the dashboard is a web app, no native app)
- Cloud sync / accounts (everything is local)
- Multiple languages (English only, written for her)
- Touch interface (buttons only, per Asif's choice — *since evolved: the shipped build IS a touch interface, a single Warm-Touch button on GPIO0*)
- 3D-printed enclosure (per Asif's choice)

---

## Open questions (none remaining)

All design decisions were locked. Implementation began immediately after Asif approved this spec and is now shipped.

> ★ **Evolved during the build:** the date-gated `/secret` single note became a keepsake **letter jar + music box** (richer, lower-pressure); the 3-button + NeoPixel ring became the **touch button + breathing LED** deskmate; OTA went from "disabled" to **enabled** (Block 4, token-secured over the `Aurora` AP). The emotional payload and build intent from below are unchanged.

---

## A note on the human context (for the record)

This spec was originally drafted around a romantic-admiration emotional core. After reflection, Asif shared that the recipient does not have romantic feelings for him, and his own feelings have cooled. He still wants to express care, gratitude, and a one-shot honest confession before the two graduate and go separate ways.

The spec has been revised to reflect this:
- The 30 daily messages are warm, caring, and identity-affirming — but not romantic, not possessive, not future-presuming. They are her *daily companion*, not a slow-motion ask.
- The /secret page is a single, honest, low-pressure note. One-shot closure. No implied ask, no negotiation, no "maybe you'll change your mind."

The engineering is unchanged. The 5-day timeline is unchanged. The hardware, firmware, dashboard, and enclosure are unchanged. What changed is the *emotional payload* of the words inside.

The build itself is still a real, impressive, hand-crafted thing — independent of any response. If the recipient never opens the /secret page, the gift is still a beautiful object. If she does open it, she finds honesty without pressure. Either way, Asif gets to say what he wanted to say, with grace, in a form that takes real effort and skill.

---

## References

- [feasibility-analysis.md](feasibility-analysis.md) — the deep technical feasibility pass that informed this spec
- Web research notes — see the build chat history for the supporting web searches (ESP32-C3 compatibility, memory budgets, async-server reliability, ArduinoJson best practices)
