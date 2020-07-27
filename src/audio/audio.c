#include "audio/audio.h"
#include "project.h"

uint8_t bs0_dma_ch;
uint8_t bs0_dma_td[BS0_DMA_TD_COUNT];
uint8_t usb_buf[BS0_DMA_TRANSFER_SIZE * BS0_DMA_TD_COUNT];
uint8_t bs1_dma_ch;
uint8_t bs1_dma_td[BS1_DMA_TD_COUNT];
uint8_t byte_swap_buf[BS1_DMA_TRANSFER_SIZE * BS1_DMA_TD_COUNT];
uint8_t i2s_dma_ch;
uint8_t i2s_dma_td[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];
uint8_t bs0_sync;
uint32_t bs0_sync_ctr;
uint32_t bs0_in_index;
uint32_t bs0_out_index;
uint8_t bs1_sync;
uint32_t bs1_sync_ctr;
uint32_t bs1_in_index;
uint32_t bs1_out_index;

CY_ISR_PROTO(usb_sync_isr);
CY_ISR_PROTO(bs_sync_isr);
CY_ISR_PROTO(i2s_sync_isr);

// USB Sram => byte_swap F0
void dma_byteswap0_config(void)
{
    uint16_t i = 0;

    bs0_dma_ch = DMA_BS0_DmaInitialize(BS0_DMA_BYTES_PER_BURST, BS0_DMA_REQUEST_PER_BURST, HI16(BS0_DMA_SRC_BASE), HI16(BS0_DMA_DST_BASE));
    for (i = 0; i < BS0_DMA_TD_COUNT; i++) {
        bs0_dma_td[i] = CyDmaTdAllocate();
    }
    // Link buffers into a circular buffer.
    for (i = 0; i < BS0_DMA_TD_COUNT; i++) {
        CyDmaTdSetConfiguration(bs0_dma_td[i], BS0_DMA_TRANSFER_SIZE, bs0_dma_td[(i + 1) % BS0_DMA_TD_COUNT], (TD_INC_SRC_ADR | DMA_BS0__TD_TERMOUT_EN));
    }
    // Set source and destination addresses
    for (i = 0; i < BS0_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(bs0_dma_td[i], LO16((uint32)&usb_buf[i * BS0_DMA_TRANSFER_SIZE]), LO16((uint32)byte_swap_DP_F0_PTR));
    }
    CyDmaChSetInitialTd(bs0_dma_ch, bs0_dma_td[0]);
    
}

// byte_swap F1 => byte_swap_buf SRAM
void dma_byteswap1_config(void)
{
    uint16_t i = 0;

    bs1_dma_ch = DMA_BS1_DmaInitialize(BS1_DMA_BYTES_PER_BURST, BS1_DMA_REQUEST_PER_BURST, HI16(BS1_DMA_SRC_BASE), HI16(BS1_DMA_DST_BASE));
    for (i = 0; i < BS1_DMA_TD_COUNT; i++) {
        bs1_dma_td[i] = CyDmaTdAllocate();
    }
    // Link buffers into a circular buffer.
    for (i = 0; i < BS1_DMA_TD_COUNT; i++) {
        CyDmaTdSetConfiguration(bs1_dma_td[i], BS1_DMA_TRANSFER_SIZE, bs1_dma_td[(i + 1) % BS1_DMA_TD_COUNT], (TD_INC_DST_ADR | DMA_BS1__TD_TERMOUT_EN));
    }
    // Set source and destination addresses
    for (i = 0; i < BS1_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(bs1_dma_td[i], LO16((uint32)byte_swap_DP_F1_PTR), LO16((uint32)&byte_swap_buf[i * BS1_DMA_TRANSFER_SIZE]));
    }
    CyDmaChSetInitialTd(bs1_dma_ch, bs1_dma_td[0]);
    // Don't start it automatically. Wait until buffer fills up a bit.
}

// byte_swap sram => I2S_F0
void dma_is2_config(void)
{
    uint16_t i = 0;

    i2s_dma_ch = DMA_I2S_DmaInitialize(I2S_DMA_BYTES_PER_BURST, I2S_DMA_REQUEST_PER_BURST, HI16(I2S_DMA_SRC_BASE), HI16(I2S_DMA_DST_BASE));
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        i2s_dma_td[i] = CyDmaTdAllocate();
    }
    // Link buffers into a circular buffer.
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetConfiguration(i2s_dma_td[i], I2S_DMA_TRANSFER_SIZE, i2s_dma_td[(i + 1) % I2S_DMA_TD_COUNT], (TD_INC_SRC_ADR | DMA_I2S__TD_TERMOUT_EN));
    }
    // Set source and destination addresses
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(i2s_dma_td[i], LO16((uint32)&byte_swap_buf[i * I2S_DMA_TRANSFER_SIZE]), LO16((uint32)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_dma_ch, i2s_dma_td[0]);
    // Don't start it automatically. Wait until buffer fills up a bit.
}

// Configure all DMAs
void audio_dma_config(void);

CY_ISR(usb_sync_isr)
{
    ++bs0_out_index;
    bs0_out_index = (bs0_out_index >= BS0_DMA_TD_COUNT) ? 0u : bs0_out_index;
    // Overflow, stop dma. Bad.
    if (bs0_out_index == bs0_in_index) {
        CyDmaChDisable(bs0_dma_ch);
        bs0_sync = 0;
        bs0_sync_ctr = 0;
    }
}

CY_ISR(bs_sync_isr)
{
    ++bs1_out_index;
    bs1_out_index = (bs1_out_index >= BS1_DMA_TD_COUNT) ? 0u : bs1_out_index;
    if (bs1_out_index == bs1_in_index) {
        CyDmaChDisable(bs1_dma_ch);
        I2S_DisableTx();    // ?
        bs1_sync = 0;
        bs1_sync_ctr = 0;
    }
}

CY_ISR(i2s_sync_isr)
{
    
}
