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
