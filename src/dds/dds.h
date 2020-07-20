#ifndef DDS_H
#define DDS_H

#include <stdint.h>

// Generating a 440Hz signal.
// fs = 48k, f = 440. N_cycles = fs / f = 109 samples per one full 440Hz period.
#define N_SAMPLES   (48 * 32)
#define N_BYTES     (9216u)  // 109 samples * 3 bytes * 2 channels
#define FS          (48000)
#define F_DDS       (440)

// 440Hz sine wave lookup table.
//extern int32_t sample_buf[N_SAMPLES];
// sample_buf is converted to a two channel, 3 byte per channel buffer. 6 bytes per sample.
extern uint8_t sample_byte_buf[N_BYTES];

// Compute the sine wave table
int dds_sin_table(int32_t amplitude);
// Compute the audio buffer from the sine wave table.
int audio_table(int32_t amplitude);

#endif
