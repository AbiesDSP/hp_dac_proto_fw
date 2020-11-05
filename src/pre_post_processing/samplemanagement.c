#include "pre_post_processing/samplemanagement.h"

//int32_t get_audio_sample_from_bytestream(uint8_t *buf)
//{
//    //audio sample for one channel is 3 bytes long
//    //populate these bytes into a 32 bit signed int
//
//    int32_t audio_sample;
//    uint8_t first_byte = *(buf);
//    uint8_t second_byte = *(buf+1);
//    uint8_t third_byte = *(buf+2);
//
//    audio_sample = ((uint32_t) first_byte << 24) | ((uint32_t) second_byte << 16) | ((uint32_t) third_byte << 8);
//
//    return audio_sample;
//}
//
//void return_sample_to_bytestream(int32_t sample, uint8_t *buf)
//{
//    //audio sample for one channel is 3 bytes long
//    //We'll take the top 3 bytes, the bottom byte will be discarded
//    //DAC is 24 bit
//    
//    //there's probbly a better way to do this, this certainly drops information
//    //might want to think about dithering
//
//    uint8_t first_byte = sample >> 24 & 0x000000FF;
//    uint8_t second_byte = sample >> 16 & 0x000000FF;
//    uint8_t third_byte = sample >> 8 & 0x000000FF;
//
//    *(buf) = first_byte;
//    *(buf+1) = second_byte;
//    *(buf+2) = third_byte;
//}