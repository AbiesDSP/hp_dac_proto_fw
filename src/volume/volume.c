#include "volume/volume.h"
#include "knobs/knobs.h"
#include "project.h"

uint8_t volume_multiplier;
uint16_t knob_buckets[257];

void volume_start(void)
{
    //fill knob bucket array for reference later
//    uint16_t i = 0;
//    uint32_t k;
//
//    for (i=0; i<257; i++)
//    {
//        //look up table to compare the knobs value to
//        //256 buckets for volume steps
//        k = i * KNOBS_MAX;
//        knob_buckets[i] = k >> 8;
//    }
    
    // Start at zero? Knob should update audiomatically anyways.
    set_volume_multiplier(0);
}

void set_volume_multiplier(int16_t knob_value)
{
    // No negative volumes allowed
    if (knob_value < 0) {
        knob_value = 0;
    }
    // Reduce ADC resolution to 8 bits from the native ADC resolution.
    volume_multiplier = knob_value >> (ADC_CFG1_RESOLUTION - VOL_N_BITS);
    
//    uint16_t i = 0;
//
//    //find the i to multiply the sample by
//    for(i = 0; i<257; i++)
//    {
//        if (knobs[VOLUME_KNOB] <= knob_buckets[i])
//        {
//            //find the multiplier based ont he knob value
//            volume_multiplier = i;
//            break;
//        }
//    }
}

//inline int32_t apply_volume_filter_to_sample(int32_t sample)
//{
//    //multiply the sample by the volume multiplier
//    //divide by 256
//    int64_t unprocessed_sample;
//    uint32_t processed_sample;
//    
//    unprocessed_sample = sample * volume_multiplier;
//    processed_sample = unprocessed_sample >> 8;
//
//    return processed_sample;
//}