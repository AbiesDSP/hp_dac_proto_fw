#include "unity.h"
#include <stdint.h>

void setUp(void)
{

}

void tearDown(void)
{

}

// Test if foo.
void test_signed_shift(void)
{
    int32_t sample;
    int32_t result;
    int32_t expected;

    // Negative input?
    sample = -42;
    expected = -21;
    // divide by 2?
    result = sample >> 1;
    TEST_ASSERT_EQUAL_INT32(expected, result);

    // What about positive?
    sample = 42068;
    expected = 21034;
    
    result = sample >> 1;
    TEST_ASSERT_EQUAL_INT32(expected, result);

    // Okay but what about left shifts?
    sample = 69;
    expected = 138;
    result = sample << 1;
    TEST_ASSERT_EQUAL_INT32(expected, result);

    sample = -420;
    expected = -840;
    result = sample << 1;
    TEST_ASSERT_EQUAL_INT32(expected, result);
}

void test_twoscomplement(void)
{
    int16_t sample, result, expected;

    // What number is this? Is it -64?
    sample = 0xC000;
    expected = -64;
    TEST_ASSERT_NOT_EQUAL_INT16(expected, sample);
    // Nope.

    // What number is this? Is it -96?
    sample = 0xE000;
    expected = -96;
    TEST_ASSERT_NOT_EQUAL_INT16(expected, sample);
    // Nope.

    // Two's complement.
    sample = 0xFFC0;
    expected = -64;
    TEST_ASSERT_EQUAL_INT16(expected, sample);
}