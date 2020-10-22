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

void foo();

#define BS_DMA_BYTES_PER_BURST      (1u)
#define BS_DMA_REQUEST_PER_BURST    (1u)
#define BS0_DMA_TD_COUNT            (1u)
#define BS0_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define BS0_DMA_SRC_BASE            (CYDEV_SRAM_BASE)
#define BS1_DMA_TD_COUNT            (2u)
#define BS1_DMA_DST_BASE            (CYDEV_SRAM_BASE)
#define BS1_DMA_SRC_BASE            (CYDEV_PERIPH_BASE)

extern uint8_t bs0_dma_ch, bs1_dma_ch;
extern uint8_t bs0_dma_td[BS0_DMA_TD_COUNT];
extern uint8_t bs1_dma_td[BS1_DMA_TD_COUNT];

extern uint8_t i2s_out_dma_ch;
extern uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];
extern uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

extern uint32_t write_index;
extern uint32_t tx_buf_size;
extern uint32_t tx_shadow;
extern uint16_t out_level;

extern uint32_t sync_dma;

void audio_init(void);
void audio_out(void);
void dma_tx_config(void);

void audio_stop(void);
void audio_start(void);

CY_ISR_PROTO(i2s_dma_done_isr);

#endif
