#ifndef SAMPLEMANAGEMENT_H
#define SAMPLEMANAGEMENT_H

#include <stdint.h>

// These can be inline. Faster this way.
extern inline int32_t get_audio_sample_from_bytestream(uint8_t *buf);
extern inline void return_sample_to_bytestream(int32_t sample, uint8_t *buf);

#endif
