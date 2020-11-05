#ifndef SAMPLEMANAGEMENT_H
#define SAMPLEMANAGEMENT_H

#include <stdint.h>

// These can be inline. Faster this way.
inline __attribute__(( always_inline)) int32_t get_audio_sample_from_bytestream(uint8_t *buf)
{
    return ((uint32_t)buf[2] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[0] << 8);
}

inline __attribute__(( always_inline)) void return_sample_to_bytestream(int32_t sample, uint8_t *buf)
{
    buf[2] = (sample >> 24) & 0xFF;
    buf[1] = (sample >> 16) & 0xFF;
    buf[0] = (sample >> 8) & 0xFF;
}

#endif
