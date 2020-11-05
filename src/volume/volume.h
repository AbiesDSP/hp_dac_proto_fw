#ifndef VOLUME_H
#define VOLUME_H

#include "knobs/knobs.h" // include knobs here instead of in the .c so everything that includes volume will get the knobs dependencies too. Like KNOB_RES
#include <stdint.h>

// Resolution of volume control in bits.
#define VOL_RES     (8u)

//volume is the current volume multiplier [0,1] based on the volume knob state
extern uint8_t volume_multiplier;

// volume_array is set at initialization of volume, and contains the array of multiplier values to index to
//extern uint16_t knob_buckets[257];
    
inline int32_t apply_volume_filter_to_sample(int32_t sample)
{
    return (sample >> 8) * volume_multiplier;
}

void set_volume_multiplier(int16_t knob_value);
void volume_start(void);

#endif
