#include "unity.h"
#include "pre_post_processing/samplemanagement.h"

void setUp(void)
{

}

void tearDown(void)
{

}

void test_sample_pulled_correctly(void)
{
    //single sample
    uint8_t buffer[3] = {0xC9, 0x62, 0x77};
    int32_t expected;
    int32_t result;

    expected = 0x7762C900;
    result = get_audio_sample_from_bytestream(&buffer[0]);
    TEST_ASSERT_EQUAL_INT32(expected, result);
}

void test_stereo_samples_handled(void)
{
    //stereo sample
    uint8_t buffer[6] = {0x01, 0xFF, 0xC4, 0x01, 0xFF, 0xC4};
    int32_t expected;
    int32_t result;
    
    expected = 0xC4FF0100;
    //right
    result = get_audio_sample_from_bytestream(&buffer[0]);
    TEST_ASSERT_EQUAL_INT32(expected, result);
    //left
    result = get_audio_sample_from_bytestream(&buffer[3]);
    TEST_ASSERT_EQUAL_INT32(expected, result);
}

void test_negative_and_positive(void)
{
    //single sample
    uint8_t buffer[3];
    int32_t expected;
    int32_t result;

    //positive
    buffer[2] = 0x00;
    buffer[1] = 0x14;
    buffer[0] = 0xA0;
    expected = 1351680;
    result = get_audio_sample_from_bytestream(&buffer[0]);
    TEST_ASSERT_EQUAL_INT32(expected, result);

    //negative
    buffer[2] = 0xFF;
    buffer[1] = 0xFD;
    buffer[0] = 0xC9;
    expected = -145152;
    result = get_audio_sample_from_bytestream(&buffer[0]);
    TEST_ASSERT_EQUAL_INT32(expected, result);
}

void test_put_sample_back(void)
{
    int32_t sample;
    uint8_t expected[3] = {0xCE, 0xA0, 0x22};
    uint8_t result[3];

    sample = 580963975;
    return_sample_to_bytestream(sample, &result[0]);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, result, 3);
}