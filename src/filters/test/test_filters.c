#include "unity.h"
#include "filters/filters.h"

void setUp(void)
{

}

void tearDown(void)
{

}

void test_half_knob(void)
{
    //a = knobs_value/32768
    //y[i] = ax[i] + (1-a)y[i-1]
    int32_t sample = 200;
    int32_t prev_sample = 5000;
    int32_t expected_output = (0.5*sample) + (0.5*prev_sample);

    int32_t output;
    int16_t knobs_value = 0x7FFF/2;

    set_LPFilt_coef_multiplier(knobs_value);
    output = apply_lpf_to_sample(sample, prev_sample);

    TEST_ASSERT_EQUAL_INT32(expected_output, output);
}

void test_three_fourths_knob(void)
{
    //a = knobs_value/32768
    //y[i] = ax[i] + (1-a)y[i-1]
    int32_t sample = 200;
    int32_t prev_sample = 5000;
    int32_t expected_output = (0.75*sample) + (0.25*prev_sample);

    int32_t output;
    int16_t knobs_value = (0x7FFF/4)*3;

    set_LPFilt_coef_multiplier(knobs_value);
    output = apply_lpf_to_sample(sample, prev_sample);

    TEST_ASSERT_EQUAL_INT32(expected_output, output);
}