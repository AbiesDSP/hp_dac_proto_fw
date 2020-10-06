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
    #include "audio/audio.h"
    #include "usb/usb.h"
    
    /*Define your macro callbacks here */
    /*For more information, refer to the Writing Code topic in the PSoC Creator Help.*/
    #define USBFS_EP_1_ISR_ENTRY_CALLBACK
    #define USBFS_EP_1_ISR_EntryCallback()  audio_out()
    
    #define USBFS_EP_3_ISR_ENTRY_CALLBACK
    #define USBFS_EP_3_ISR_EntryCallback()  usb_feedback()
    
    #define USBFS_SOF_ISR_ENTRY_CALLBACK	
	#define USBFS_SOF_ISR_EntryCallback()	usb_sof()
    
#endif /* CYAPICALLBACKS_H */   
/* [] */
