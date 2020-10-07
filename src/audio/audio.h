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
#define I2S_DMA_TRANSFER_SIZE       (144u)
#define I2S_DMA_TD_COUNT            (8u)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)

#define AUDIO_BUF_SIZE  (I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT)

#define QUAR    (AUDIO_BUF_SIZE / 4)
#define HALF    (AUDIO_BUF_SIZE / 2)
#define MOST    (QUAR + HALF)
    
extern uint8_t i2s_out_dma_ch;
extern uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];
extern uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

extern uint32_t sample_index;
extern uint32_t i2s_index;
extern uint32_t in_index;
extern uint32_t sync_dma;
extern int32_t level;

void audio_out(void);
void dma_tx_config(void);

void audio_stop(void);
void audio_start(void);

#endif
