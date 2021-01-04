#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>

#define FILT_RES     (8u)

extern uint8_t LPfilter_coef_mulitplier;

void filters_start();
int32_t LPF19523(int32_t sample, int32_t prev_sample);
int32_t apply_lpf_to_sample(int32_t sample, int32_t prev_sample);
int32_t multiply_by_coef(int32_t input);
void set_LPFilt_coef_multiplier(int16_t knob_value);

#endif