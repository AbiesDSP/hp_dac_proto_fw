#include "audio/audio.h"
#include "usb/usb.h"

#include "project.h"

uint8_t bs0_dma_ch, bs1_dma_ch;
uint8_t bs0_dma_td[BS0_DMA_TD_COUNT];
uint8_t bs1_dma_td[BS1_DMA_TD_COUNT];

uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

uint32_t write_index;
//uint32_t read_index;
uint32_t tx_buf_size;
uint32_t tx_shadow;

uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];

uint32_t sync_dma = 0;

uint16_t out_level = 0;
int reset_tx = 0;

CY_ISR_PROTO(bs1isr);
void audio_init(void)
{
    unsigned int i = 0;
    bs0_dma_ch = DMA_BS0_DmaInitialize(BS_DMA_BYTES_PER_BURST, BS_DMA_REQUEST_PER_BURST, HI16(BS0_DMA_SRC_BASE), HI16(BS0_DMA_DST_BASE));
    bs1_dma_ch = DMA_BS1_DmaInitialize(BS_DMA_BYTES_PER_BURST, BS_DMA_REQUEST_PER_BURST, HI16(BS1_DMA_SRC_BASE), HI16(BS1_DMA_DST_BASE));
    for (i = 0; i < BS0_DMA_TD_COUNT; i++) {
        bs0_dma_td[i] = CyDmaTdAllocate();
    }
    for (i = 0; i < BS1_DMA_TD_COUNT; i++) {
        bs1_dma_td[i] = CyDmaTdAllocate();
    }
    
    dma_tx_config();
    write_index = 0;
    tx_buf_size = 0;
    tx_shadow = 0;
    out_level = 0;
    reset_tx = 0;
    i2s_isr_StartEx(i2s_dma_done_isr);
    I2S_TX_CH0_ACTL_REG |= I2S_FIFO0_LVL;
//    byte_swap_DP_F0_SET_LEVEL_MID;
//    byte_swap_DP_F1_SET_LEVEL_MID;
    
    I2S_Start();
}
// Audio out callback. Called whenever a new isochronous packet arrives.
// Automatic dma transfers will put the new data into "usb_out_buf"
void audio_out(void)
{
    uint8_t int_status = 0;
    uint16_t count = 0, remain = 0;
    
    if (reset_tx) {
        write_index = 0;
        tx_buf_size = 0;
        reset_tx = 0;
    }
    // Check usb frame size
    count = USBFS_GetEPCount(AUDIO_OUT_EP);
    int_status = CyEnterCriticalSection();
    tx_buf_size += count;
    CyExitCriticalSection(int_status);
    
    // Transfer of USB to byte swap.
    CyDmaTdSetConfiguration(bs0_dma_td[0], count, CY_DMA_DISABLE_TD, (TD_INC_SRC_ADR | DMA_BS0__TD_TERMOUT_EN));
    CyDmaTdSetAddress(bs0_dma_td[0], LO16((uint32_t)&usb_out_buf[0]), LO16((uint32_t)byte_swap_fifo_in_ptr));
    CyDmaChSetInitialTd(bs0_dma_ch, bs0_dma_td[0]);
    // Transfer of byte swap to sample buffer
    // Requires two TDs
    if (write_index + count > AUDIO_BUF_SIZE) {
        remain = AUDIO_BUF_SIZE - write_index;
        CyDmaTdSetConfiguration(bs1_dma_td[0], remain, bs1_dma_td[1], (TD_INC_DST_ADR | DMA_BS1__TD_TERMOUT_EN));
        CyDmaTdSetAddress(bs1_dma_td[0], LO16((uint32_t)byte_swap_fifo_out_ptr), LO16((uint32_t)&samples[write_index]));
        CyDmaTdSetConfiguration(bs1_dma_td[1], count - remain, CY_DMA_DISABLE_TD, (TD_INC_DST_ADR | DMA_BS1__TD_TERMOUT_EN));
        CyDmaTdSetAddress(bs1_dma_td[1], LO16((uint32_t)byte_swap_fifo_out_ptr), LO16((uint32_t)&samples[0]));
        write_index = count - remain;
    } else {
        // Only need one.
        CyDmaTdSetConfiguration(bs1_dma_td[0], count, CY_DMA_DISABLE_TD, (TD_INC_DST_ADR | DMA_BS1__TD_TERMOUT_EN));
        CyDmaTdSetAddress(bs1_dma_td[0], LO16((uint32_t)byte_swap_fifo_out_ptr), LO16((uint32_t)&samples[write_index]));
        write_index += count;
        write_index = write_index == AUDIO_BUF_SIZE ? 0u : write_index;
    }
    CyDmaChSetInitialTd(bs1_dma_ch, bs1_dma_td[0]);
    // Start dma transfer
    CyDmaChEnable(bs0_dma_ch, 1u);
    CyDmaChEnable(bs1_dma_ch, 1u);
    
    if (sync_dma == 0) {
        if (tx_buf_size >= HALF) {
            sync_dma = 1;
            I2S_ClearTxFIFO();
            mute_Write(1u);
            CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
            I2S_EnableTx();
        }
    }
}

// Chained DMA TD. Dump the full LUT into one half of the buffer while the
// destination pointer is incrementing through the other half.
void dma_tx_config(void)
{
    uint16_t i = 0;
    
    i2s_out_dma_ch = DMA_I2S_DmaInitialize(I2S_DMA_BYTES_PER_BURST, I2S_DMA_REQUEST_PER_BURST, HI16(I2S_DMA_SRC_BASE), HI16(I2S_DMA_DST_BASE));
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        i2s_out_dma_td[i] = CyDmaTdAllocate();
    }
    // Link buffers into a circular buffer.
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetConfiguration(i2s_out_dma_td[i], I2S_DMA_TRANSFER_SIZE, i2s_out_dma_td[(i + 1) % I2S_DMA_TD_COUNT], (TD_INC_SRC_ADR | DMA_I2S__TD_TERMOUT_EN));
        CyDmaTdSetAddress(i2s_out_dma_td[i], LO16((uint32)&samples[i * I2S_DMA_TRANSFER_SIZE]), LO16((uint32)I2S_TX_CH0_F0_PTR));
    }

    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
}

void audio_stop(void)
{
    uint32_t i = 0;
    
    if (sync_dma) {
        I2S_DisableTx();
        CyDelayUs(20);
        CyDmaChDisable(i2s_out_dma_ch);
        mute_Write(0u);
        // Reset sync variables.
        tx_shadow = 0;
        sync_dma = 0;
        out_level = 0;
        reset_tx = 1;
        for (i = 0; i < sizeof(samples); i++) {
            samples[i] = 0;
        }
    }
}

void audio_start(void)
{
    
}

CY_ISR(i2s_dma_done_isr)
{
    uint32_t added;
    
    added = tx_buf_size - tx_shadow;
    out_level += added;
    tx_shadow = tx_buf_size;
    tx_buf_size -= I2S_DMA_TRANSFER_SIZE;
    
    // Underflow
    if (out_level <= QUAR) {
//        audio_stop();
    } else {
        out_level -= I2S_DMA_TRANSFER_SIZE;
    }
    // Overflow
    if (out_level > (AUDIO_BUF_SIZE + 6)) {
//        audio_stop();
    }
}
