#include "usb/usb.h"
#include "audio/audio_out.h"
#include "sync/sync.h"
#include "project.h"

#define USE_HYST    (0u)

#define FB_NOM          (3072u)
#define FB_HYST_DEAD    (0x00)
#define FB_HYST_INC     (0x01)
#define FB_HYST_DEC     (0x02)

const uint16_t fb_default = 0x0C00;

volatile uint8_t fb_data[3] = {0x00, 0x00, 0x0C};
volatile uint8_t fb_updated = 0;
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

// This is called whenever the host requests feedback. Not sure what we need to do here.
void usb_feedback(void)
{
    uint16_t new_fb = 0, size = 0;
    uint8_t int_status = 0;

    int_status = CyEnterCriticalSection();
    size = audio_out_buffer_size;
    CyExitCriticalSection(int_status);
    
    // Only making changes of +/- USB_FB_INC from default.
    // Determine the next feeback value. If we're outside the range, correct it.
    if (audio_out_active) {
        if (size < (AUDIO_OUT_ACTIVE_LIMIT - USB_FB_RANGE)) {
            new_fb = fb_default + USB_FB_INC;
        } else if (size > (AUDIO_OUT_ACTIVE_LIMIT + USB_FB_RANGE)) {
            new_fb = fb_default - USB_FB_INC;
        }
    }
    // Load new feedback into ep
    fb_data[2] = HI8(new_fb);
    fb_data[1] = LO8(new_fb);
    if ((USBFS_GetEPState(AUDIO_FB_EP) == USBFS_IN_BUFFER_EMPTY)) {
        USBFS_LoadInEP(AUDIO_FB_EP, USBFS_NULL, 3);
    }
    // Indicator to application. use as needed.
    fb_update_flag = 1;
}
// Fun fun usb stuff.
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
                USBFS_LoadInEP(AUDIO_FB_EP, (const uint8_t*)fb_data, 3);
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
