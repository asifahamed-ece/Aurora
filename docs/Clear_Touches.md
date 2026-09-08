# Clear Touches Count (NVS Reset)

The Aurora deskmate stores the lifetime `Warm Touches` counter in ESP32-C3 NVS flash under namespace `"aurora"`, key `"touches"`. Below are methods to reset it.

---

## Option 1: Serial Monitor Command

Add this block to `loop()` in `src/main.cpp`:

```cpp
if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r') {
        Preferences prefs;
        prefs.begin("aurora", false);
        prefs.putUInt("touches", 0);
        prefs.end();
        AuroraState::instance().begin(); // reload state
        DBG_PRINTLN("[NVS] Touches reset to 0");
    }
}
```

**Usage:**
1. Upload firmware with the above code.
2. Open serial monitor: `pio device monitor -b 115200`
3. Send `r` and press Enter.
4. Touches counter resets to 0.

---

## Option 2: Erase Entire NVS Namespace

Flash a minimal sketch that wipes all keys in the `"aurora"` namespace:

```cpp
#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    Preferences prefs;
    prefs.begin("aurora", false);
    prefs.clear(); // wipes ALL keys: touches, last_touch, epoch, boot_cnt, mid_ack
    prefs.end();
    Serial.println("[NVS] All aurora keys wiped!");
}

void loop() {}
```

**Note:** This also clears `epoch`, `boot_cnt`, and `midnightAckDoy`. The device will re-seed the clock from compile time on next boot.

---

## Option 3: Erase NVS Partition via esptool

Erase the entire NVS partition (20 KB at offset `0x9000`) without reprogramming firmware:

```bash
esptool.py --port /dev/ttyACM0 erase_region 0x9000 0x5000
```

**Partition layout reference (from `partitions_aurora.csv`):**

| Name   | Offset  | Size    |
|--------|---------|---------|
| nvs    | 0x9000  | 0x5000  |
| otadata| 0xe000  | 0x2000  |
| app0   | 0x10000 | 0x140000|

---

## Option 4: Reset Only Touches via Arduino IDE / PlatformIO

Use the ESP32 Preferences API in a standalone sketch:

```cpp
#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    Preferences prefs;
    prefs.begin("aurora", false);

    // Reset touches to 0
    prefs.putUInt("touches", 0);

    // Optionally reset last touch timestamp too
    prefs.putULong("last_touch", 0);

    prefs.end();
    Serial.println("[NVS] Touches reset to 0");
}

void loop() {}
```

---

## Option 5: Hardware Long-Press Reset (Future Enhancement)

Reserve a 5-second hold on the touch button (GPIO0) to reset touches without a computer. Implementation sketch:

```cpp
static uint32_t touchPressStart = 0;
static bool touchHeld = false;

// In loop(), after buttons.update():
if (events & BTN_TOUCH) {
    touchPressStart = millis();
    touchHeld = true;
}
if (touchHeld && digitalRead(AURORA_BTN_TOUCH_PIN) == LOW) {
    if (millis() - touchPressStart >= 5000) {
        Preferences prefs;
        prefs.begin("aurora", false);
        prefs.putUInt("touches", 0);
        prefs.end();
        AuroraState::instance().begin();
        display.popup("Touches Reset!", 2000);
        DBG_PRINTLN("[NVS] Touches reset via long-press");
        touchHeld = false;
    }
} else {
    touchHeld = false;
}
```

---

## Key Names in NVS Namespace `"aurora"`

| Key          | Type       | Description                          |
|--------------|------------|--------------------------------------|
| `touches`    | `uint`     | Lifetime warm touch counter          |
| `last_touch` | `ulong`    | millis() timestamp of last touch     |
| `epoch`      | `ulong`    | UTC epoch seconds                    |
| `boot_cnt`   | `uint`     | Boot counter                         |
| `mid_ack`    | `int`      | Day-of-year of last midnight ack     |

---

## Quick Reference

| Method              | Touches Only | Full Wipe | Needs Reboot | Needs Flash |
|---------------------|:------------:|:---------:|:------------:|:-----------:|
| Serial `r` command  | Yes          | No        | No           | Yes (1x)    |
| `prefs.clear()`     | No           | Yes       | Yes          | Yes (1x)    |
| `esptool erase`     | No           | Yes       | Yes          | No          |
| `putUInt("touches",0)` | Yes      | No        | No           | Yes (1x)    |
| Long-press (5s)     | Yes          | No        | No           | Yes (1x)    |
