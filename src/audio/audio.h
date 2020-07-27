#ifndef AUDIO_H
#define AUDIO_H

/*
 * All of the buffers and dma config for the 
 * audio pipeline.
 */
#include <stdint.h>
#include <cytypes.h>
    
#define USB_FRAME_SIZE              (288u)

// USB Sram buffer to byte swap component
// Need to interrupt every transfer to move pointers around.
#define BS0_DMA_BYTES_PER_BURST     (1u)
#define BS0_DMA_REQUEST_PER_BURST   (1u)
#define BS0_DMA_SRC_BASE            (CYDEV_SRAM_BASE)
#define BS0_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define BS0_DMA_TRANSFER_SIZE       (USB_FRAME_SIZE)
#define BS0_DMA_TD_COUNT            (4u)
#define BS0_DMA_ENABLE_PRESERVE_TD  (1u)
extern uint8_t bs0_dma_ch;
extern uint8_t bs0_dma_td[BS0_DMA_TD_COUNT];
extern uint8_t usb_buf[BS0_DMA_TRANSFER_SIZE * BS0_DMA_TD_COUNT];
extern uint8_t bs0_sync;
extern uint32_t bs0_sync_ctr;
extern uint32_t bs0_in_index;
extern uint32_t bs0_out_index;
void dma_byteswap0_config(void);

// Return from the byte_swap. Dump into another sram buffer
#define BS1_DMA_BYTES_PER_BURST     (1u)
#define BS1_DMA_REQUEST_PER_BURST   (1u)
#define BS1_DMA_SRC_BASE            (CYDEV_PERIPH_BASE)
#define BS1_DMA_DST_BASE            (CYDEV_SRAM_BASE)
#define BS1_DMA_TRANSFER_SIZE       (USB_FRAME_SIZE)
#define BS1_DMA_TD_COUNT            (4u)
#define BS1_DMA_ENABLE_PRESERVE_TD  (1u)
extern uint8_t bs1_dma_ch;
extern uint8_t bs1_dma_td[BS1_DMA_TD_COUNT];
extern uint8_t byte_swap_buf[BS1_DMA_TRANSFER_SIZE * BS1_DMA_TD_COUNT];
extern uint8_t bs1_sync;
extern uint32_t bs1_sync_ctr;
extern uint32_t bs1_in_index;
extern uint32_t bs1_out_index;
void dma_byteswap1_config(void);

// Byte Swap Buffer to I2S
#define I2S_DMA_BYTES_PER_BURST     (1u)
#define I2S_DMA_REQUEST_PER_BURST   (1u)
#define I2S_DMA_SRC_BASE            (CYDEV_SRAM_BASE)
#define I2S_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define I2S_DMA_TRANSFER_SIZE       (USB_FRAME_SIZE)
#define I2S_DMA_TD_COUNT            (4u)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)
extern uint8_t i2s_dma_ch;
extern uint8_t i2s_dma_td[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];
void dma_is2_config(void);

// Configure all DMAs
void audio_dma_config(void);

#endif
