/**
 *  Aurora — Birthday Gift Firmware
 *  File: include/messages.h
 *  Block 4: 30 daily messages (firmware mirror of data/messages.js)
 *
 *  The browser holds a copy in messages.js; the firmware holds a copy here
 *  so the /api/state response can include msg_idx even if the JS bundle
 *  has not yet finished loading. Both arrays MUST stay in sync — Asif
 *  edits one, Asif edits the other. Order is significant.
 */
#ifndef AURORA_BLOCK4_MESSAGES_H
#define AURORA_BLOCK4_MESSAGES_H

#include <Arduino.h>

#define AURORA_NUM_MESSAGES 30

namespace aurora_messages {

extern const char* const kMessages[AURORA_NUM_MESSAGES];

//  Pick a message index from a day-of-year. Mirrors the JS fallback rule:
//  msg_idx = (doy - 1) % 30, clamped to [0, 29].
inline uint8_t indexForDoy(uint16_t doy) {
    if (doy == 0) doy = 1;
    return (uint8_t)((doy - 1) % AURORA_NUM_MESSAGES);
}

} // namespace aurora_messages

#endif // AURORA_BLOCK4_MESSAGES_H
