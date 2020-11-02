#ifndef VOLUME_H
#define VOLUME_H

#include <stdint.h>

//volume is the current volume multiplier [0,1] based on the volume knob state
extern volatile uint16_t volume_multiplier;
// volume_array is set at initialization of volume, and contains the array of multiplier values to index to
extern uint16_t knob_buckets[257];
    
int32_t apply_volume_filter_to_sample(int32_t sample);
void set_volume_multiplier(void);
void volume_start(void);

#endif