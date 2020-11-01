#include "volume/volume.h"
#include "knobs/knobs.h"
#include "project.h"

volatile float volume;
float volume_array[0xFF];

void volume_start(void)
{
    //fill volume multiplier array
    //array will be the length of 0xFF
    //each indecy in the array will be the volume multiplier
    uint8_t i = 0;

    for (i=0; i<0xFF; i++)
    {
        volume_array[i] = (float) i / (float) 0xFF;
    }
    
    //set starting volume based on initial knob setting
    set_volume();
}

void set_volume(void)
{
    float scaled_knob;
    uint8_t low_res_knob;
    
    scaled_knob = (float) knobs[VOLUME_KNOB] / (float) KNOBS_MAX;
    low_res_knob = scaled_knob * 0xFF;
    
    
    //index to the volume multiplier based on the knob
    volume = volume_array[low_res_knob];
}

int32_t apply_volume_filter_to_sample(int32_t sample)
{
    //multiply sample by the current volume multiplier
    int32_t volumed_sample = volume * sample;

    return volumed_sample;
}