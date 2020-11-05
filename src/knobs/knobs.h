#ifndef KNOBS_H
#define KNOBS_H

#include <stdint.h>
#include "cytypes.h"    // Change to cytypes for simpler dependency.

#define N_KNOBS (3u)
#define KNOB_STS_NEW    (0x01u)
#define VOLUME_KNOB     (0u)
//currently a 15 bit adc
#define KNOBS_MAX       (0x7FFF)

// When KNOB_STS_NEW is set in knob_status, knobs will have been updated to the latest value.
extern volatile int16_t knobs[N_KNOBS];
extern volatile uint8_t knob_status;

void knobs_start(void);
CY_ISR_PROTO(adcdone);

#endif
