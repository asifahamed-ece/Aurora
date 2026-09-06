/**
 *  Aurora — Birthday Gift Firmware
 *  File: src/clock.cpp
 *  Block 4: time/date/day-of-year helpers
 *
 *  Time math is intentionally hand-rolled (no <ctime>) — the C3 is short
 *  on flash, and we only need a handful of fields, not the whole libc.
 */
#include "clock.h"

namespace aurora_clock {

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

uint16_t dayOfYear(uint32_t epoch) {
    //  Split into date + time-of-day.
    uint32_t days = epoch / 86400;
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
    uint32_t s  = epoch % 60;
    uint32_t m  = (epoch / 60) % 60;
    uint32_t h  = (epoch / 3600) % 24;
    snprintf(out, outSize, "%02lu:%02lu:%02lu",
             (unsigned long)h, (unsigned long)m, (unsigned long)s);
}

void formatDate(char* out, size_t outSize, uint32_t epoch) {
    uint32_t days = epoch / 86400;
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
    uint8_t dow = (uint8_t)((epoch / 86400 + 4) % 7);   // 1970-01-01 was Thursday (4)
    snprintf(out, outSize, "%s, %u %s",
             kDayShort[dow], (unsigned)day, kMonthShort[m]);
}

bool isBirthday(uint32_t epoch) {
    uint32_t days = epoch / 86400;
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
    uint32_t days = epoch / 86400;
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
