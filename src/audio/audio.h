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
#define I2S_DMA_TRANSFER_SIZE       (288u)
#define I2S_DMA_TD_COUNT            (8u)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)
extern uint8_t i2s_out_dma_ch;
extern uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];
extern uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

extern uint32_t out_index;
extern uint32_t in_index;
extern uint32_t sync_dma;
extern uint32_t sync_dma_counter;

void audio_out(void);
void dma_tx_config(void);

#endif
