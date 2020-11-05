#ifndef KNOBS_H
#define KNOBS_H

#include <stdint.h>
#include "cytypes.h"    // Change to cytypes for simpler dependency.

#define N_KNOBS (3u)
#define KNOB_STS_NEW    (0x01u)
//currently a 15 bit adc
#define KNOB_RES    (15u)

// When KNOB_STS_NEW is set in knob_status, knobs will have been updated to the latest value.
extern volatile int16_t knobs[N_KNOBS];
extern volatile uint8_t knob_status;

void knobs_start(void);
CY_ISR_PROTO(adcdone);

#endif
