#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

// Calculates a crc from the supplied buffer
uint16_t crc16usb(const uint8_t buf[], size_t length);

// Calculates the crc for length - 2, and compares it to the last two bytes
// Returns 1 if it matches, 0 if not.
uint8_t crc16usb_check(const uint8_t *buf, size_t length);

#endif
