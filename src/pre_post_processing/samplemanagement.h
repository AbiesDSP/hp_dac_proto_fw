#ifndef SAMPLEMANAGEMENT_H
#define SAMPLEMANAGEMENT_H

#include <stdint.h>

int32_t get_audio_sample_from_bytestream(uint8_t *buf);
void return_sample_to_bytestream(int32_t sample, uint8_t *buf);

#endif