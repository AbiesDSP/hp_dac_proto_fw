/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#ifndef CYAPICALLBACKS_H
#define CYAPICALLBACKS_H
    #include "audio/audio_out.h"
    #include "usb/usb.h"
    
    /*Define your macro callbacks here */
    /*For more information, refer to the Writing Code topic in the PSoC Creator Help.*/
    #define USBFS_EP_1_ISR_ENTRY_CALLBACK
    #define USBFS_EP_1_ISR_EntryCallback()  audio_out_update()
    
    #define USBFS_EP_3_ISR_ENTRY_CALLBACK
    #define USBFS_EP_3_ISR_EntryCallback()  usb_feedback()
    
#endif /* CYAPICALLBACKS_H */   
/* [] */
