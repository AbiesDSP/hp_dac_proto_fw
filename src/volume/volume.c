#include "volume/volume.h"
#include "knobs/knobs.h"
#include "project.h"

volatile uint16_t volume_multiplier;
uint16_t knob_buckets[257];

void volume_start(void)
{
    //fill knob bucket array for reference later
    uint16_t i = 0;
    uint32_t k;

    for (i=0; i<257; i++)
    {
        //look up table to compare the knobs value to
        //256 buckets for volume steps
        k = i * KNOBS_MAX;
        knob_buckets[i] = k >> 8;
    }
    
    //set starting volume based on initial knob setting
    set_volume_multiplier();
}

void set_volume_multiplier(void)
{
    uint16_t i = 0;

    //find the i to multiply the sample by
    for(i = 0; i<257; i++)
    {
        if (knobs[VOLUME_KNOB] <= knob_buckets[i])
        {
            //find the multiplier based ont he knob value
            volume_multiplier = i;
            break;
        }
    }
}

int32_t apply_volume_filter_to_sample(int32_t sample)
{
    //multiply the sample by the volume multiplier
    //divide by 256
    uint32_t unprocessed_sample;
    uint32_t processed_sample;
    
    unprocessed_sample = sample * volume_multiplier;
    processed_sample = unprocessed_sample >> 8;

    return processed_sample;
}