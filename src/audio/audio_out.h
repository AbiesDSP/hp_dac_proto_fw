#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "CyLib.h"
#include "cytypes.h"
#include <stdint.h>

#define AUDIO_OUT_TRANSFER_SIZE (288u)
#define AUDIO_OUT_N_TD          (64u)
#define AUDIO_OUT_BUF_SIZE      (AUDIO_OUT_N_TD * AUDIO_OUT_TRANSFER_SIZE)

#define AUDIO_OUT_LOW_LIMIT     (USB_FB_RANGE)
#define AUDIO_OUT_HIGH_LIMIT    (AUDIO_OUT_BUF_SIZE - USB_FB_RANGE)
#define AUDIO_OUT_ACTIVE_LIMIT  (AUDIO_OUT_BUF_SIZE >> 1u)

#define AUDIO_OUT_STS_ACTIVE    (0x01u)
#define AUDIO_OUT_STS_OVERFLOW  (0x02u)
#define AUDIO_OUT_STS_UNDERFLOW (0x04u)
#define AUDIO_OUT_STS_RESET     (0x08u)

#define AUDIO_OUT_EP            (1u)
#define USB_FB_INC              (0x08)
#define USB_FB_RANGE            (AUDIO_OUT_TRANSFER_SIZE)

extern uint8_t audio_out_buf[AUDIO_OUT_BUF_SIZE];
extern volatile uint16_t audio_out_shadow;
extern volatile uint16_t audio_out_buffer_size;
extern volatile uint8_t audio_out_active;
extern volatile uint8_t audio_out_status;

extern volatile uint8_t audio_out_update_flag;

typedef struct {
    uint8_t usb_dma_ch;
    uint8_t usb_dma_termout_en;
    uint8_t bs_dma_ch;
    uint8_t bs_dma_termout_en;
    uint8_t i2s_dma_ch;
    uint8_t i2s_dma_termout_en;
    uint8_t *usb_buf;
    reg8 *bs_fifo_in;
    reg8 *bs_fifo_out;
} audio_out_config;

void audio_out_init(audio_out_config config);
void audio_out_start(void);
// Gets called on audio out ep isr. Put in cyapicallbacks.h
void audio_out_update(void);
// Start and stop audio playback and I2S.
void audio_out_enable(void);
void audio_out_disable(void);
// ISRs for these peripherals.
CY_ISR_PROTO(bs_done_isr);
CY_ISR_PROTO(i2s_done_isr);

#endif
