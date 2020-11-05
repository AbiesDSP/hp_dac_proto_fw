#ifndef VOLUME_H
#define VOLUME_H

#include <stdint.h>

/* Resolution of volume control in bits. 8 bits is ideal. More than 8 bits would require 
 * using a 64 bit integer to hold the multiplication result, then shift. Otherwise
 * there may be some rounding/truncating errors.
 */
#define VOL_RES     (8u)

//volume is the current volume multiplier [0,1] based on the volume knob state
extern uint8_t volume_multiplier;

// volume_array is set at initialization of volume, and contains the array of multiplier values to index to
//extern uint16_t knob_buckets[257];

/* We know the bottom 8 bits are 0. So we can shift, then multiply.
 * This saves us from needing to use a 64bit int to hold the multiply result.
 * This really only works for a volume resolution <= 8bits.
 */
inline int32_t apply_volume_filter_to_sample(int32_t sample)
{
    return (sample >> VOL_RES) * volume_multiplier;
}

/* Update the volume scalar from a signed knob value.
 * The ADC is 15 bits, but it can go negative. So
 * it's technically a signed 16 bit, but with a max value
 * of 32767, and a minimum of maybe -100 or so. You can treat
 * it like a 15 bit number if you round negative numbers to 0.
 */
void set_volume_multiplier(int16_t knob_value);

// Initialize volume scalar.
void volume_start(void);

#endif
