#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "cytypes.h"
#include <stdint.h>

#define N_BYTES                 (3u)

#define AUDIO_OUT_TRANSFER_SIZE (288u)
#define AUDIO_OUT_N_TD          (32u)
#define AUDIO_OUT_BUF_SIZE      (AUDIO_OUT_N_TD * AUDIO_OUT_TRANSFER_SIZE)
#define AUDIO_OUT_PROCESS_SIZE  (294u)

#define USB_FB_RANGE            (AUDIO_OUT_TRANSFER_SIZE)
#define AUDIO_OUT_LOW_LIMIT     (USB_FB_RANGE)
#define AUDIO_OUT_HIGH_LIMIT    (AUDIO_OUT_BUF_SIZE - USB_FB_RANGE)
#define AUDIO_OUT_ACTIVE_LIMIT  (AUDIO_OUT_BUF_SIZE >> 1u)

#define AUDIO_OUT_STS_ACTIVE    (0x01u)
#define AUDIO_OUT_STS_OVERFLOW  (0x02u)
#define AUDIO_OUT_STS_UNDERFLOW (0x04u)
#define AUDIO_OUT_STS_RESET     (0x08u)

#define AUDIO_OUT_EP            (1u)
#define USB_FB_INC              (0x08u)

/* A flag set in the audio_update ISR. Indicates that
 * new data is available for processing.
 */
extern volatile uint8_t audio_out_update_flag;
    
/* This is a temporary buffer used for in-place audio processing.
 * The USB component will write each packet to this buffer and
 * set audio_out_update_flag. You can process this data in place,
 * then call audio_out_transmit() to start the DMA transaction.
 */
extern uint8_t audio_out_process[AUDIO_OUT_PROCESS_SIZE];

// The number of bytes in the audio_out_process buffer.
extern uint16_t audio_out_count;

// This is the main output buffer that the I2S DMA will read from
extern uint8_t audio_out_buf[AUDIO_OUT_BUF_SIZE];

// Current size of the main output buffer.
extern volatile uint16_t audio_out_buffer_size;

// Audio is currently playing.
extern volatile uint8_t audio_out_active;

//Status register. Updated audiomatically.
extern volatile uint8_t audio_out_status;

/* Peripheral configuration registers. Set these based on the names
 * of the TopDesign components and DMA initialization.
 */
typedef struct {
    uint8_t usb_dma_ch;
    uint8_t usb_dma_termout_en;
    uint8_t bs_dma_ch;
    uint8_t bs_dma_termout_en;
    uint8_t i2s_dma_ch;
    uint8_t i2s_dma_termout_en;
    reg8 *bs_fifo_in;
    reg8 *bs_fifo_out;
} audio_out_config;

// Initialize module with hardware configuration.
void audio_out_init(audio_out_config config);

// Start up DMA channels, I2S, and ISRs.
void audio_out_start(void);

// Gets called on audio out ep isr. Put in cyapicallbacks.h for the USBFS EP1 Entry Callback.
void audio_out_update(void);

// Send processed data to bs component DMA.
void audio_out_transmit(void);

// Start and stop audio playback and I2S.
void audio_out_enable(void);
void audio_out_disable(void);

// ISRs for TopDesign peripherals.
CY_ISR_PROTO(bs_done_isr);
CY_ISR_PROTO(i2s_done_isr);

#endif
