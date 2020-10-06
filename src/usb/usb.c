#include "usb/usb.h"
#include "audio/audio.h"

uint8_t fb_data[3] = {0x00, 0x00, 0x0C};
uint8_t fb_updated = 0;
uint32_t sample_rate_feedback = 0;
uint8_t audio_active = 0;

uint8_t usb_out_buf[MAX_FRAME_SIZE];

// Debugging variables
uint32_t sof_counter = 0;
uint32_t fb_counter = 0;

void usb_sof(void)
{
    uint16_t samples = 0;
    uint32_t buf_size = 0;
    sof_counter++;
    
    samples = ((uint16_t)fb_data[2] << 8) | fb_data[1];
    buf_size = usb_buffer_size();
    
    if (fb_updated == 0) {
        samples = sample_rate_feedback >> 8;
        if (samples > 3136) {
            samples = 3136;
        } else if (samples < 3008) {
            samples = 3008;
        }
        fb_updated = 1;
    }

    fb_data[2] = HI8(samples);
    fb_data[1] = LO8(samples);
    
    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY)) {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
}

// This is called whenever the host requests feedback. Not sure what we need to do here.
void usb_feedback(void)
{
    fb_counter++;
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
            sample_index = 0u;
            i2s_index = 0;
            in_index = 0u;
            sync_dma = 0u;
            
            level = 0;
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

uint32_t usb_buffer_size(void)
{
    uint32_t size = 0;
    
    if (sample_index >= i2s_index) {
        size = sample_index - i2s_index;
    } else {
        size = (I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT) + sample_index - i2s_index;
    }
    
    return size;
}

