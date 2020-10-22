#ifndef SYNC_H
#define SYNC_H

#include "CyLib.h"
#include <stdint.h>

#define SYNC_TRANSFER_SIZE  (128u)

#define SYNC_STS_ACTIVE     (0x01u)
#define SYNC_STS_NEW_FB     (0x02u)

extern uint8_t sync_dma_ch;
extern uint8_t sync_dma_td[1];
extern uint16_t byte_count_buf[SYNC_TRANSFER_SIZE];
extern uint8_t sync_status;
extern uint32_t sync_new_feedback;

void sync_init(void);
void sync_enable(void);
void sync_disable(void);

CY_ISR_PROTO(sync_isr);

#endif
