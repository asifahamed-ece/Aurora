/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/clock.cpp
 *  Block 4: time/date/day-of-year helpers
 *
 *  Time math is intentionally hand-rolled (no <ctime>) — the C3 is short
 *  on flash, and we only need a handful of fields, not the whole libc.
 *
 *  The dashboard and any NTP source feed us raw UTC seconds. We display
 *  everything in IST (UTC+5:30) by adding kIstOffsetSec at the boundary.
 *  Internal storage stays in UTC so a future timezone change is one constant.
 */
#include "clock.h"

namespace aurora_clock {

// IST = UTC+5:30 = 19800 seconds. Single source of truth for the offset.
static constexpr uint32_t kIstOffsetSec = 5UL * 3600UL + 30UL * 60UL;

static const char* kDayShort[7]   = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* kMonthShort[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//  Days in each month for a non-leap year.
static const uint8_t kMonthDays[12] = {
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};

static bool isLeap(uint16_t y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

//  Days from 1970-01-01 to the start of the given year.
static uint32_t daysToYearStart(uint16_t year) {
    uint32_t days = 0;
    for (uint16_t y = 1970; y < year; ++y) {
        days += isLeap(y) ? 366 : 365;
    }
    return days;
}

// Public: convert stored UTC epoch to local epoch (IST).
uint32_t toLocal(uint32_t epoch) {
    return epoch + kIstOffsetSec;
}

uint16_t dayOfYear(uint32_t epoch) {
    uint32_t local = toLocal(epoch);
    //  Split into date + time-of-day.
    uint32_t days = local / 86400;
    uint16_t y = 1970;
    while (true) {
        uint32_t yd = isLeap(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
        ++y;
    }
    //  days is now 0..(365|366)
    uint8_t m = 0;
    for (m = 0; m < 12; ++m) {
        uint8_t md = kMonthDays[m];
        if (m == 1 && isLeap(y)) md = 29;
        if (days < md) break;
        days -= md;
    }
    return (uint16_t)(days + 1);   // 1..366
}

void formatTime(char* out, size_t outSize, uint32_t epoch) {
    uint32_t local = toLocal(epoch);
    uint32_t s  = local % 60;
    uint32_t m  = (local / 60) % 60;
    uint32_t h24 = (local / 3600) % 24;
    // 12-hour format with AM/PM indicator
    bool isPm = (h24 >= 12);
    uint32_t h12 = h24 % 12;
    if (h12 == 0) h12 = 12;
    snprintf(out, outSize, "%02lu:%02lu:%02lu %s",
             (unsigned long)h12, (unsigned long)m, (unsigned long)s,
             isPm ? "PM" : "AM");
}

void formatDate(char* out, size_t outSize, uint32_t epoch) {
    uint32_t local = toLocal(epoch);
    uint32_t days = local / 86400;
    uint16_t y = 1970;
    while (true) {
        uint32_t yd = isLeap(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
        ++y;
    }
    uint8_t m = 0;
    for (m = 0; m < 12; ++m) {
        uint8_t md = kMonthDays[m];
        if (m == 1 && isLeap(y)) md = 29;
        if (days < md) break;
        days -= md;
    }
    uint8_t day = (uint8_t)(days + 1);
    uint8_t dow = (uint8_t)((local / 86400 + 4) % 7);   // 1970-01-01 was Thursday (4)
    snprintf(out, outSize, "%s, %u %s",
             kDayShort[dow], (unsigned)day, kMonthShort[m]);
}

bool isBirthday(uint32_t epoch) {
    uint32_t local = toLocal(epoch);
    uint32_t days = local / 86400;
    uint16_t y = 1970;
    while (true) {
        uint32_t yd = isLeap(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
        ++y;
    }
    uint8_t m = 0;
    for (m = 0; m < 12; ++m) {
        uint8_t md = kMonthDays[m];
        if (m == 1 && isLeap(y)) md = 29;
        if (days < md) break;
        days -= md;
    }
    uint8_t day = (uint8_t)(days + 1);
    // Chandni's birthday is September 10th (month 8, day 10)
    return (m == 8 && day == 10);
}

uint8_t birthdayAge(uint32_t epoch) {
    uint32_t local = toLocal(epoch);
    uint32_t days = local / 86400;
    uint16_t y = 1970;
    while (true) {
        uint32_t yd = isLeap(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
        ++y;
    }
    // Born September 10, 2004
    return (y >= 2004) ? (uint8_t)(y - 2004) : 0;
}

} // namespace aurora_clock
