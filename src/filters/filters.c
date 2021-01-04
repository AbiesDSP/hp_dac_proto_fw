#include "filters/filters.h"
#include "knobs/knobs.h"

uint8_t LPfilter_coef_mulitplier;

void filters_start()
{
    set_LPFilt_coef_multiplier(0);
}

extern int32_t LPF19523(int32_t sample, int32_t prev_sample)
{
    //THIS DOESN'T WORK
    sample = sample >> 8;
    //y[i] = ax[i] + (1-a)y[i-1]
    //a = 0.713228279
    //a*(1<<15) ~= 23371
    sample = (23371*sample) + (9397*prev_sample);
    return sample>>7;
}

extern int32_t apply_lpf_to_sample(int32_t sample, int32_t prev_sample)
{
    //a = knobs_value/32768
    int32_t filt_sample = multiply_by_coef(sample);
    int32_t filt_prev_sample = multiply_by_coef(prev_sample);
    //y[i] = ax[i] + (1-a)y[i-1]
    return filt_sample + prev_sample - filt_prev_sample;
}

int32_t multiply_by_coef(int32_t input)
{
    return (input >> 8) * LPfilter_coef_mulitplier;
}

void set_LPFilt_coef_multiplier(int16_t knob_value)
{
    // No negative volumes allowed
    if (knob_value < 0) {
        knob_value = 0;
    }
    // Reduce ADC resolution to 8 bits from the native ADC resolution.
    // Note, we lose the bottom 7 bits of information on the knob or ~4 percent of the knob
    LPfilter_coef_mulitplier = knob_value >> (KNOB_RES - FILT_RES);
}