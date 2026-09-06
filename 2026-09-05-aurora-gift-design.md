---
title: "Aurora — A Birthday Gift Design Spec"
date: 2026-09-05
status: Approved for implementation
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

## High-level concept

A small hand-built box containing:
- An ESP32-C3 Supermini
- A 12-pixel WS2812B NeoPixel ring
- A 0.96" SSD1306 monochrome OLED display
- 3 tactile buttons (Up, Select, Down)
- A 3.7V LiPo + TP4056 charging module + slide switch
- 470 µF cap + 10kΩ pull-up + 470Ω resistor (fail-safe components)

The device operates in three modes navigable by buttons:
1. **Clock mode** (default) — shows day, date, live time, and one of 30 daily calming/supportive messages. LEDs breathe gently.
2. **Interaction mode** — Up/Down navigate special screens (e.g., "Days I've been with you", "Stats", "Surprise me"). Select triggers animations.
3. **WiFi mode** (entered by long-pressing Select) — C3 broadcasts `Aurora-Setup`, captive portal asks for home WiFi credentials, then device joins her home WiFi and serves a beautiful local dashboard at `http://aurora.local/`.

A **date-gated secret page** (`/secret`) is hidden in the UI before Sept 10 and returns 404 if accessed directly. After Sept 10, it reveals a personal letter written by Asif.

---

## Hardware

### Bill of materials

| Component | Source | Status |
|---|---|---|
| ESP32-C3 Supermini | Owned | ✅ |
| 12× WS2812B ring (or 8-pixel if that's what he has) | Owned | ✅ |
| 0.96" SSD1306 OLED (I2C) | Owned | ✅ |
| 3× 6mm tactile buttons | Owned | ✅ |
| 3.7V LiPo (500-1000 mAh) | Owned | ✅ |
| TP4056 charging module | Owned | ✅ |
| 470 µF electrolytic capacitor | Buy tomorrow (₹10) | ⏳ |
| 10 kΩ resistor (for GPIO0 pull-up) | Buy tomorrow (₹2) | ⏳ |
| 470 Ω resistor (for WS2812B data line) | Buy tomorrow (₹2) | ⏳ |
| Slide switch (SPST) | Buy tomorrow (₹15) | ⏳ |
| 4× M3 brass standoffs (15mm) | Buy tomorrow (₹40) | ⏳ |
| 3mm clear acrylic sheet (200mm × 150mm) | Buy tomorrow (₹100) | ⏳ |
| Black acrylic or thin plywood (for box body) | Buy tomorrow (₹100) | ⏳ |
| M3 screws (8×) | Buy tomorrow (₹20) | ⏳ |
| Perfboard (5×7 cm or similar) | Owned or buy (₹30) | ⏳ |

**Total cost of new components: ~₹300** (~$3.50).

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

---

## Firmware architecture

### Tech stack
- **Framework:** Arduino (ESP32 Arduino core 2.0.x or later)
- **Libraries:**
  - `Adafruit_NeoPixel` (LED ring)
  - `Adafruit_SSD1306` + `Adafruit_GFX` (OLED)
  - `ESPAsyncWebServer` + `AsyncTCP` (web server)
  - `DNSServer` (captive portal)
  - `ArduinoJson` v7 (state API)
  - `WiFi` (built-in)
  - `Preferences` (built-in, for saving WiFi credentials)
  - `NTPClient` + `time.h` (time sync)
  - `esp_task_wdt` (watchdog)
- **Storage:** LittleFS for dashboard HTML/CSS/JS files
- **OTA:** Disabled (per Asif's choice)

### Three operating modes

#### Mode 1: Clock / Daily Message (default on boot)
- OLED line 1: Day, Date (e.g., "Friday, Sept 10")
- OLED line 2: Live time (HH:MM:SS, NTP-synced)
- OLED line 3: Today's calming message (index = `dayOfYear % 30`)
- LED animation: slow breathing, 5-second period, warm color (amber blend)
- Wakes on any button press → enters Mode 2

#### Mode 2: Interaction
- **Up button:** cycle forward through special screens
- **Down button:** cycle backward
- **Select button:** trigger current screen's "show me" action (e.g., rainbow chase, days-together animation)
- Screens:
  1. "Stats" — button presses today, total button presses, battery voltage
  2. "Days together" — `(today - giftDate)` days, with celebration animation at milestones (30, 100, 365)
  3. "Mood" — a 1-5 mood scale she can set with Up/Down, persisted in Preferences
  4. "Surprise" — a one-time random animation: rainbow chase, sparkle, or color wave
- 30 seconds of no button press → return to Mode 1

#### Mode 3: WiFi Setup (entered via long-press Select from Mode 1)
- C3 broadcasts `Aurora-Setup` as open AP for 60 seconds
- After 60s, becomes password-protected (`aurora1234`)
- Captive portal: any HTTP request → redirected to `192.168.4.1/setup`
- Setup page form: WiFi SSID + password
- On submit: C3 saves credentials via `Preferences`, restarts in station mode
- Station mode: joins saved WiFi, starts mDNS as `aurora.local`, serves dashboard at port 80
- On boot in station mode: tries saved WiFi for 15s, falls back to AP mode if fails

### Daily message library
- 30 messages stored as a `const char* messages[30]` array in PROGMEM
- Selected by `dayOfYear % 30`
- Tone: CSE-flavored, calming, supportive, identity-affirming — **the kind of things a person who genuinely cares about her would want her to hear on a hard day.** Warm but not presumptuous. They can show care and even affection, but they don't assume a future together.
- Length: ~10-20 words each
- Asif drafts these in-session on Day 3 (see Build Timeline)

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

**The point:** the 30 messages are *her daily companion* for as long as she keeps Aurora on her desk. They should feel like a steady, warm presence — not like a slow-motion ask. The confession happens once, in the /secret page, on Sept 10. Everything else just says *"you matter, and I'm glad I knew you."*

### Watchdog
- ESP32 hardware watchdog enabled, 30-second timeout
- `loop()` must call `esp_task_wdt_reset()` every cycle
- Auto-recovery from any lockup

### Battery monitoring
- ADC read on a voltage divider (2× 100kΩ) from battery+ → GPIO3 (ADC1_CH3)
- If V_batt < 3.3V, OLED shows "Low battery" warning
- If V_batt < 3.0V, LEDs dim to 10% to extend runtime
- If V_batt < 2.8V, device enters deep-sleep and refuses to boot until charged

### Power-on behavior
- Slide switch controls battery line
- Boot time: < 2 seconds to clock display
- All previous state (WiFi creds, mood, stats) persists in Preferences (NVS)

---

## Web dashboard (served from C3's LittleFS)

### File structure

```
/data/
├── index.html       (main dashboard, ~25 KB)
├── app.css          (~8 KB)
├── app.js           (~12 KB)
├── secret.html      (date-gated personal letter, ~5 KB)
└── error.html       (404 page for /secret before unlock)
```

Total: ~50 KB. Trivially fits in 4 MB flash.

### Visual design language

- **Theme:** dark by default (background: `#0e0e10`, foreground: `#e8d9c0` cream)
- **Color palette:** warm cream + soft amber + deep charcoal (matches the LED color, intentional)
- **Typography:** system font stack with a custom-display font for headings
- **Layout:** single column on mobile, max-width 720px centered
- **Background:** subtle CSS-only starfield animation (very cheap, very pretty)

### Dashboard components (`index.html`)

1. **Header card** — greeting ("Hi [her name]") + live time + day-of-year progress bar
2. **Today's message card** — current daily message, large and centered
3. **Stats row** — 3 small cards: button presses today, days together, last interaction
4. **Network status** — IP address, mDNS name, signal strength
5. **Footer** — small "made with care" credit + version number

### API endpoints

```
GET  /api/state         → JSON: { time, message, stats, battery, network }
GET  /api/message?day=N → JSON: { day, message } (for previewing)
POST /api/secret-unlock-test → returns 404 unless date >= 2026-09-10
GET  /secret            → HTML: secret page, only if date >= 2026-09-10
```

### Polling strategy
- Dashboard polls `/api/state` every 2 seconds via `fetch()`
- LED state shown in real-time on the dashboard (e.g., "Breathing", "Sparkle", "Setup mode")

### Date-gating the secret page
Two layers of protection:
1. **Client-side:** the dashboard's main UI queries `/api/state`, which returns `unlocked: false` if `now < 2026-09-10`. The dashboard's footer link to `/secret` is hidden via CSS.
2. **Server-side:** the `/secret` route handler checks the system date on every request. If `now < 2026-09-10`, returns 404 (with a generic "Not found" page, no hint that it exists).

Both layers must pass for the secret page to be visible.

### Content of the /secret page (soft-confession)
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
- **Top panel:** 3mm clear acrylic, laser-cut or hand-scored, with a rectangular cutout for the OLED window (~28mm × 16mm) and circular cutouts for the 3 buttons (~6mm diameter each)
- **Side panels:** black acrylic (3mm) or thin plywood, cut to 80mm × 30mm × 4 pieces, glued or screwed together
- **Bottom panel:** black acrylic, with a cutout for the slide switch and a micro-USB access hole
- **Standoffs:** 4× M3 brass standoffs (15mm) screwed between top and bottom panels, with the perfboard mounted on top of the standoffs

### Mounting strategy
- **OLED:** hot-glued to underside of top panel, aligned with the cutout
- **Buttons:** soldered to perfboard, button caps protrude through top panel cutouts
- **LED ring:** mounted on perfboard, positioned under the top panel with a small cutout (or visible through a translucent diffuser if Asif adds one)
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

### Risk mitigation
- Day 5 is buffer. No firmware changes after Day 4.
- Soldering happens on Day 3-4 in parallel with firmware — they meet on Day 4 evening.
- If enclosure is harder than expected, fall back to a simpler design (acrylic top + 4-sided wrap of cardboard + bottom).

---

## Verification (how to test end-to-end)

Before gifting, Asif must verify:

1. **Power on from battery:** device boots to Mode 1 within 2s, shows correct time (after NTP sync), shows today's message.
2. **Button navigation:** all 3 buttons respond, no false triggers, no missed presses.
3. **LED animations:** breathing is smooth, surprise animations complete cleanly, no flicker.
4. **WiFi setup flow:** reset device (long-press Select), AP `Aurora-Setup` appears, captive portal loads, can submit home WiFi creds, device restarts and joins home WiFi.
5. **Dashboard:** from phone on same WiFi, `http://aurora.local/` loads within 3 seconds, shows live time, today's message, updates every 2 seconds.
6. **Secret page before unlock:** direct navigation to `http://aurora.local/secret` returns 404. Dashboard footer doesn't show the link.
7. **Secret page after unlock (simulated):** set device clock to Sept 10+, secret page loads with the personal letter.
8. **OTA** (n/a, skipped per design).
9. **Battery life:** fully charged, WiFi connected, LEDs at 30% brightness, dashboard refreshing → device should run ≥8 hours.
10. **Brownout test:** rapid button presses during WiFi reconnection → device must not crash, watchdog must recover.
11. **Rehearsal:** do a full unboxing rehearsal (open box, press Select, see greeting, navigate to dashboard URL, open on phone, navigate to /secret). The whole flow should take < 60 seconds and feel magical.

---

## Critical files (to be created during implementation)

- `firmware/aurora.ino` — main Arduino sketch
- `firmware/secrets.h` — WiFi credentials placeholder, message library array
- `firmware/state.h` — global state, mode management
- `firmware/display.h` — OLED rendering helpers
- `firmware/leds.h` — NeoPixel animation library
- `firmware/wifi_manager.h` — AP/station mode logic, captive portal
- `firmware/dashboard_api.h` — AsyncWebServer route handlers
- `firmware/secret_gate.h` — date-gating logic
- `firmware/battery.h` — voltage monitor, deep-sleep on low battery
- `data/index.html` — dashboard main page
- `data/app.css` — dashboard styles
- `data/app.js` — dashboard logic
- `data/secret.html` — date-gated personal letter
- `README.md` — build instructions, pin map, troubleshooting
- `MESSAGES.md` — the 30 daily messages (with index numbers for reference)

---

## Out of scope (explicitly)

- OTA firmware updates (per Asif's choice)
- Ambient sensor (per Asif's choice)
- Mobile app (the dashboard is a web app, no native app)
- Cloud sync / accounts (everything is local)
- Multiple languages (English only, written for her)
- Touch interface (buttons only, per Asif's choice)
- 3D-printed enclosure (per Asif's choice)

---

## Open questions (none remaining)

All design decisions are locked. Implementation can begin as soon as Asif approves this spec.

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
- [Web research notes](#) — see chat history for the supporting web searches (ESP32-C3 compatibility, memory budgets, WS2812B reliability, ArduinoJson + AsyncWebServer best practices)
