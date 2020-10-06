#ifndef AUDIO_H
#define AUDIO_H

/*
 * All of the buffers and dma config for the 
 * audio pipeline.
 */
#include <USBFS.h>
#include <stdint.h>
#define I2S_DMA_BYTES_PER_BURST     (1u)
#define I2S_DMA_REQUEST_PER_BURST   (1u)
#define I2S_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define I2S_DMA_SRC_BASE            (CYDEV_SRAM_BASE)
#define I2S_DMA_TRANSFER_SIZE       (294u)
#define I2S_DMA_TD_COUNT            (4u)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)
    
#define HALF            ((I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT) / 2)
#define MOSTLY          ((3 * I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT) / 4)
    
extern uint8_t i2s_out_dma_ch;
extern uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];
extern uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

extern uint32_t sample_index;
extern uint32_t i2s_index;
extern uint32_t in_index;
extern uint32_t sync_dma;
extern int32_t level;

// Debugging
extern uint32_t audio_out_count;

void audio_out(void);
void dma_tx_config(void);

#endif
