#include "unity.h"
#include "volume/volume.h"
#include "knobs/knobs.h"

void setUp(void)
{

}

void tearDown(void)
{

}

void test_knob_bucket_max_is_KNOBS_MAX(void)
{
    volume_start();
    uint16_t expected = KNOBS_MAX;
    TEST_ASSERT_EQUAL_INT16(expected, knob_buckets[256]);
}

void test_knob_bucket_min_is_zero(void)
{
    volume_start();
    uint16_t expected = 0;
    TEST_ASSERT_EQUAL_INT16(expected, knob_buckets[0]);
}

void test_knob_bucket_half_is_half_KNOB_MAX(void)
{
    volume_start();
    uint16_t expected = KNOBS_MAX / 2;
    TEST_ASSERT_EQUAL_INT16(expected, knob_buckets[256/2]);
}

void test_min_volume_is_zero(void)
{
    volume_start();
    knobs[VOLUME_KNOB] = 0x0000;
    uint16_t expected = 0;
    TEST_ASSERT_EQUAL_INT16(expected, volume_multiplier);
}

void test_max_volume_is_256(void)
{
    volume_start();
    knobs[VOLUME_KNOB] = KNOBS_MAX;
    uint16_t expected = 256;
    TEST_ASSERT_EQUAL_INT16(expected, volume_multiplier);
}

void test_third_volume_is_third_max(void)
{
    volume_start();
    knobs[VOLUME_KNOB] = KNOBS_MAX / 3;
    uint16_t expected = 256 / 3;
    TEST_ASSERT_EQUAL_INT16(expected, volume_multiplier);
}