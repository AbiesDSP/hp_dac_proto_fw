#ifndef VOLUME_H
#define VOLUME_H

#include <stdint.h>

//volume is the current volume multiplier [0,1] based on the volume knob state
extern volatile float volume;
// volume_array is set at initialization of volume, and contains the array of multiplier values to index to
extern float volume_array[0xFF];
    
int32_t apply_volume_filter_to_sample(int32_t sample);
void set_volume(void);
void volume_start(void);

#endif