#include "usb/usb.h"
#include "audio/audio_out.h"
#include "sync/sync.h"
#include "project.h"

#define FB_MIN  (784384)
#define FB_MAX  (788480)
#define FB_HYST_DEAD    (0x00)
#define FB_HYST_INC     (0x01)
#define FB_HYST_DEC     (0x02)

const uint16_t fb_default = 0x0C00;

uint8_t fb_data[3] = {0x00, 0x00, 0x0C};
uint8_t fb_updated = 0;
uint32_t sample_rate_feedback = 0;

uint8_t usb_out_buf[USB_BUF_SIZE];

uint8_t usb_status = 0;
uint8_t usb_alt_setting[USB_NO_STREAM_IFACE] = {0xFF, 0xFF};

volatile uint8_t fb_update_flag;

void usb_start(void)
{
    usb_status = 0;
    fb_update_flag = 0;
    // Start and enumerate USB.
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration());
}

void usb_sof(void)
{
    uint16_t new_fb = 0, size = 0;
    uint8_t int_status = 0;
    static uint8_t fb_hyster = FB_HYST_DEAD;

    int_status = CyEnterCriticalSection();
    size = audio_out_buffer_size;
    CyExitCriticalSection(int_status);
    
    // // Old samples value. (default)
     new_fb = ((uint16_t)fb_data[2] << 8) | fb_data[1];
    
    // if (fb_updated == 0) {
        // We are in the deadband.
    //     if (fb_hyster == FB_HYST_DEAD) {
    //         // start decreasing or increasing when we're out of range.
    //         if (size > (AUDIO_OUT_ACTIVE_LIMIT + USB_FB_RANGE)) {
    //             new_fb -= USB_FB_INC;
    //             fb_hyster = FB_HYST_DEC;
    //         } else if (size < (AUDIO_OUT_ACTIVE_LIMIT - USB_FB_RANGE)) {
    //             new_fb += USB_FB_INC;
    //             fb_hyster = FB_HYST_INC;
    //         }
    //     } else if (fb_hyster == FB_HYST_INC) {
    //         // Stop increasing when we hit half.
    //         if (size <= AUDIO_OUT_ACTIVE_LIMIT) {
    //             fb_hyster = FB_HYST_DEAD;
    //         } else {
    //             // Stay increasing.
    //             new_fb += USB_FB_INC;
    //         }
    //     } else if (fb_hyster == FB_HYST_DEC) {
    //         if (size >= AUDIO_OUT_ACTIVE_LIMIT) {
    //             fb_hyster = FB_HYST_DEAD;
    //         } else {
    //             new_fb -= USB_FB_INC;
    //         }
    //     } else {
    //         // error.
    //         fb_hyster = FB_HYST_DEAD;
    //     }
    //     sample_rate_feedback = new_fb;
    //     fb_updated = 1;
    // } 
    if (fb_updated == 0) {
        if (size < (AUDIO_OUT_ACTIVE_LIMIT - USB_FB_RANGE)) {
//            new_fb = sync_new_feedback >> 8;
            new_fb += USB_FB_INC;
            fb_updated = 1;
        } else if (size > (AUDIO_OUT_ACTIVE_LIMIT + USB_FB_RANGE)) {
//            new_fb = sync_new_feedback >> 8;
            new_fb -= USB_FB_INC;
            fb_updated = 1;
        }
    }

    fb_data[2] = HI8(new_fb);
    fb_data[1] = LO8(new_fb);

    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY)) {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
}

// This is called whenever the host requests feedback. Not sure what we need to do here.
void usb_feedback(void)
{
    fb_data[2] = 0x0C;
    fb_data[1] = 0x00;
    fb_data[0] = 0x00;
    fb_updated = 0;
    fb_update_flag = 1;
}

void usb_service(void)
{
    uint16_t i;
    if (usb_status == USB_STS_INACTIVE) {
        usb_status = USB_STS_INIT;
    }
    if (usb_status == USB_STS_INIT) {
        usb_status = USB_STS_ENUM;
        // Initialize buffers.
        for (i = 0; i < USB_BUF_SIZE; i++) {
            usb_out_buf[i] = 0u;
        }
    }

    if (USBFS_IsConfigurationChanged()) {
        if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USBFS_GetInterfaceSetting(1)) {
            usb_alt_setting[USB_OUT_IFACE_INDEX] = USBFS_GetInterfaceSetting(1);
            audio_out_disable();
            I2S_Stop();
            I2S_Start();
            if (usb_alt_setting[USB_OUT_IFACE_INDEX] != USB_ALT_ZEROBW) {
                USBFS_ReadOutEP(AUDIO_OUT_EP, &usb_out_buf[0], USB_BUF_SIZE);
                USBFS_EnableOutEP(AUDIO_OUT_EP);
                USBFS_LoadInEP(AUDIO_FB_EP, fb_data, 3);
                USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
            }
            fb_data[2] = 0x0C;
            fb_data[1] = 0x00;
            fb_data[0] = 0x00;
        }
        if (usb_alt_setting[USB_IN_IFACE_INDEX] != USBFS_GetInterfaceSetting(2)) {
            usb_alt_setting[USB_IN_IFACE_INDEX] = USBFS_GetInterfaceSetting(2);
            // Audio in stuff.
        }
    }
}

void usb_fs_change(void)
{
    if ((USBFS_TRANS_STATE_IDLE == USBFS_transferState) && USBFS_frequencyChanged) {

    }
}
