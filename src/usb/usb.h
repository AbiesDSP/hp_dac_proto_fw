#ifndef USB_H
#define USB_H
    
#include <USBFS.h>

#define USBFS_AUDIO_DEVICE  (0u)
#define AUDIO_INTERFACE     (1u)
#define AUDIO_OUT_EP    (1u)
#define AUDIO_IN_EP     (2u)
#define AUDIO_FB_EP     (3u)
#define USB_BUF_SIZE    (294u)

#define USB_STS_INACTIVE    (0x00u)
#define USB_STS_INIT        (0x01u)
#define USB_STS_ENUM        (0x02u)

#define USB_NO_STREAM_IFACE (2u)
#define USB_OUT_IFACE_INDEX (0u)
#define USB_IN_IFACE_INDEX  (1u)
#define USB_ALT_ZEROBW      (0u)
#define USB_ALT_ACTIVE_24   (2u)
#define USB_ALT_ACTIVE_16   (1u)
#define USB_ALT_INVALID     (0xFF)
    
#define USB_FB_INC      (0x20);
#define USB_FB_RANGE    (864u)

extern uint8_t usb_out_buf[USB_BUF_SIZE];
extern uint8_t fb_data[3];
extern uint8_t fb_updated;
extern uint32_t sample_rate_feedback;

extern uint8_t usb_status;
extern uint8_t usb_alt_setting[USB_NO_STREAM_IFACE];

void usb_start(void);
void usb_sof(void);
void usb_feedback(void);
void usb_service(void);
void usb_fs_change(void);

#endif
    