#include "audio/audio.h"
#include "usb/usb.h"

#include "project.h"
uint32_t out_index = 0;
uint32_t in_index = 0;
uint32_t sync_dma = 0;
uint32_t sync_dma_counter = 0;

uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];
uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];

// Audio out callback. Called whenever a new isochronous packet arrives.
// Automatic dma transfers will put the new data into "usb_out_buf"
void audio_out(void)
{
    uint8_t i, j, k;
    uint16_t count = 0;
    
    // Check usb frame size
    count = USBFS_GetEPCount(AUDIO_OUT_EP);
    if (count != I2S_DMA_TRANSFER_SIZE) {
        i = 0;  // debugging
    }
    // Reverse byte order. This could(should) be done in hardware with a dma transfer.
    for (i = 0; i < 48; i++) {
        for (j = 0; j < 2; j++) {
            for (k = 0; k < 3; k++) {
                samples[in_index * I2S_DMA_TRANSFER_SIZE + i*6 + j*3 + k] = usb_out_buf[i*6 + j*3 + (2-k)];
            }
        }
    }
    // Update buffer indices
    in_index++;
    in_index = (in_index >= I2S_DMA_TD_COUNT) ? 0u : in_index;
    sync_dma_counter++;
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
    }
    // Set source and destination addresses
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(i2s_out_dma_td[i], LO16((uint32)&samples[i * I2S_DMA_TRANSFER_SIZE]), LO16((uint32)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
}
