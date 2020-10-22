#include "sync/sync.h"
#include "CyDmac.h"
#include "project.h"

#define SYNC_INIT       (0x0C0000)

uint8_t sync_dma_ch;
uint8_t sync_dma_td[1];
uint16_t byte_count_buf[SYNC_TRANSFER_SIZE];
uint8_t sync_status;
uint32_t sync_new_feedback = SYNC_INIT;

void sync_init(void)
{
    uint16_t i = 0;
    sync_status = 0;
    sync_new_feedback = SYNC_INIT;
    for (i = 0; i < SYNC_TRANSFER_SIZE; i++) {
        byte_count_buf[i] = 0;
    }

    sync_dma_ch = DMA_Sync_DmaInitialize(2u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    sync_dma_td[0] = CyDmaTdAllocate();

    CyDmaTdSetConfiguration(sync_dma_td[0], 2u * SYNC_TRANSFER_SIZE, sync_dma_td[0], (TD_INC_DST_ADR | DMA_Sync__TD_TERMOUT_EN));
    CyDmaTdSetAddress(sync_dma_td[0], LO16((uint32_t)sync_counter_DP_F0_PTR), LO16((uint32_t)byte_count_buf));
    CyDmaChSetInitialTd(sync_dma_ch, sync_dma_td[0]);
    CyDmaChEnable(sync_dma_ch, 1u);

    // Start isr.
    byte_count_isr_StartEx(sync_isr);
}

void sync_enable(void)
{
    uint16_t i = 0;
    for (i = 0; i < SYNC_TRANSFER_SIZE; i++) {
        byte_count_buf[i] = 0;
    }
    sync_status |= SYNC_STS_ACTIVE;
}

void sync_disable(void)
{
    sync_status &= ~SYNC_STS_ACTIVE;
}

CY_ISR(sync_isr)
{
    uint8_t int_status;
    uint16_t i;
    uint32_t sum = 0;

    for (i = 0; i < SYNC_TRANSFER_SIZE; i++) {
        sum += byte_count_buf[i];
    }
    int_status = CyEnterCriticalSection();
    sync_new_feedback = (sum << 2) / 3;
    sync_status |= SYNC_STS_NEW_FB;
    CyExitCriticalSection(int_status);
}
