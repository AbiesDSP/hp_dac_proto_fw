#include "dds/dds.h"
#include <math.h>
#define SAMPLE_RATE (48000)
#define CONCERT_A   (440)

int32_t sample_buf[N_SAMPLES];
uint8_t sample_byte_buf[N_BYTES];

// There are 109 samples in one period of a 440Hz sine wave sampled at 48kHz.
int dds_sin_table(int32_t amplitude)
{
    uint32_t i = 0;

    for (i = 0; i < N_SAMPLES; i++) {
        sample_buf[i] = amplitude * sin((2 * M_PI * i)/(SAMPLE_RATE / CONCERT_A));
    }
    
    return 0;
}

// Convert sin table into audio table with lr,
int audio_table(int32_t amplitude)
{
    uint32_t i = 0, j = 0, k = 0;
    int32_t samp = 0;
    
    dds_sin_table(amplitude);
    for (i = 0; i < N_SAMPLES; i++) {
        samp = sample_buf[i];
        // 2 channels
        for (j = 0; j < 2; j++) {
            // 3 bytes per sample. Both channels are identical.
            for (k = 0; k < 3; k++) {
                sample_byte_buf[i*6 + j*3 + k] = 0xFF & ((samp) >> (8 * (2 - k)));
            }
        }
    }
    
    return 0;
}
