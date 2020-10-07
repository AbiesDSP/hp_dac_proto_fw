#ifndef USB_H
#define USB_H
    
#include <USBFS.h>

#define USBFS_AUDIO_DEVICE  (0u)
#define AUDIO_INTERFACE     (1u)
#define AUDIO_OUT_EP    (1u)
#define AUDIO_IN_EP     (2u)
#define AUDIO_FB_EP     (3u)
#define MAX_FRAME_SIZE (294u)
    
extern uint8_t usb_out_buf[MAX_FRAME_SIZE];
extern uint8_t fb_data[3];
extern uint8_t fb_updated;
extern uint32_t sample_rate_feedback;

extern uint32_t out_usb_count;
extern uint32_t out_usb_shadow;
extern uint32_t out_level;

void usb_sof(void);
void usb_feedback(void);
void service_usb(void);

uint32_t usb_buffer_size(void);

#endif
    