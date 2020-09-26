#include "usb/usb.h"
#include "audio/audio.h"

uint8_t fb_data[3] = {0x00, 0x00, 0x0C};
uint8_t fb_updated = 0;
uint32_t sample_rate_feedback = 0;

uint8_t usb_out_buf[MAX_FRAME_SIZE];

void feedback_count(void)
{
    uint16_t samples = 0;
    
    samples = ((uint16_t)fb_data[2] << 8) | fb_data[1];
    if (fb_updated == 0) {
        // Less than half full
        if (sync_dma_counter < (I2S_DMA_TD_COUNT / 2)) {
//            samples += 8;
            fb_updated = 1;
        }
        // Between 3/4 and full.
        else if (sync_dma_counter > 6) {
//            samples -= 8;
            fb_updated = 1;
        }
    }
    fb_data[2] = HI8(samples);
    fb_data[1] = LO8(samples);
    
    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY)) {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
}

void async_transfer(void)
{
    sample_rate_feedback = 48 << 14;
    fb_data[2] = 0x0C;
    fb_data[1] = 0x00;
    fb_data[0] = 0x00;
    fb_updated = 0;
}

void service_usb(void)
{
    if (0u != USBFS_IsConfigurationChanged()) {
        // Check alt setting
        if ((0u != USBFS_GetConfiguration()) && (0u != USBFS_GetInterfaceSetting(AUDIO_INTERFACE))) {
            // Reset sync variables.
            in_index = 0u;
            out_index = 0u;
            sync_dma = 0u;
            sync_dma_counter = 0u;
            sample_rate_feedback = 48 << 14;
            fb_data[2] = 0x0C;
            fb_data[1] = 0x00;
            fb_data[0] = 0x00;
            USBFS_ReadOutEP(AUDIO_OUT_EP, usb_out_buf, I2S_DMA_TRANSFER_SIZE);
            USBFS_EnableOutEP(AUDIO_OUT_EP);
            USBFS_LoadInEP(AUDIO_FB_EP, fb_data, 3);
            USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
        } else {
            // mute? idk.
        }
    }
}


