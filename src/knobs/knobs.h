#ifndef KNOBS_H
#define KNOBS_H

#include <stdint.h>
#include "CyLib.h"

#define N_KNOBS (3u)
#define KNOB_STS_NEW    (0x01u)

// When KNOB_STS_NEW is set in knob_status, knobs will have been updated to the latest value.
extern volatile uint16_t knobs[N_KNOBS];
extern volatile uint8_t knob_status;

void knobs_start(void);
CY_ISR_PROTO(adcdone);

#endif
