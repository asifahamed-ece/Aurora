/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/clock.h
 *  Block 4: time/date/day-of-year helpers
 *
 *  Block 4 does not yet have NTP. Until Block 2 wires that in, the epoch
 *  is just the firmware's compile-time timestamp. The dashboard is still
 *  useful (counters, battery, messages all work) — the clock is the only
 *  thing that needs NTP to be accurate.
 */
#ifndef AURORA_BLOCK4_CLOCK_H
#define AURORA_BLOCK4_CLOCK_H

#include <Arduino.h>

namespace aurora_clock {

//  Convert a stored UTC epoch to local (IST) epoch. All display helpers apply
//  this internally; callers that do their own hour math should use it too.
uint32_t toLocal(uint32_t epoch);

//  Day-of-year for an epoch (1..366). Uses local time (IST) so the OLED and
//  dashboard both show Chandni's Sep 10 birthday on the right day.
uint16_t dayOfYear(uint32_t epoch);

//  12-hour time as "HH:MM:SS AM/PM" (11 chars + NUL). Local time (IST).
void     formatTime(char* out, size_t outSize, uint32_t epoch);

//  Short date as "Sat, 5 Sep" (12 chars + NUL). Local date (IST).
void     formatDate(char* out, size_t outSize, uint32_t epoch);

//  Checks if the local date falls on Chandni's birthday (Sep 10)
bool     isBirthday(uint32_t epoch);

//  Calculates the age Chandni enters on Sep 10 of this epoch's year (born Sep 10, 2004)
uint8_t  birthdayAge(uint32_t epoch);

} // namespace aurora_clock

#endif // AURORA_BLOCK4_CLOCK_H
