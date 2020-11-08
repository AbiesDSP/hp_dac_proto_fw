#ifndef SAMPLEMANAGEMENT_H
#define SAMPLEMANAGEMENT_H

#include <stdint.h>

/* These can be inline. Faster this way.
 * The byte order is based on how it arrives from the USB.
 */
inline int32_t get_audio_sample_from_bytestream(uint8_t *buf)
{
    return ((uint32_t)buf[2] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[0] << 8);
}
/* Converting to 32bit, we shift everything to the msbs and
 * put zeroes in the 8 lsbs. When converting back to 24bits,
 * the bottom 8 bits are truncated.
 */
inline void return_sample_to_bytestream(int32_t sample, uint8_t *buf)
{
    buf[2] = (sample >> 24) & 0xFF;
    buf[1] = (sample >> 16) & 0xFF;
    buf[0] = (sample >> 8) & 0xFF;
}

#endif
