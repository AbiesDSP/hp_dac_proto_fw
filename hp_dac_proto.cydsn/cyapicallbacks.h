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
    
    extern void update_feedback(void);
    extern void audio_out(void);
    /*Define your macro callbacks here */
    /*For more information, refer to the Writing Code topic in the PSoC Creator Help.*/
    #define USBFS_EP_1_ISR_ENTRY_CALLBACK
    #define USBFS_EP_1_ISR_EntryCallback()  audio_out();
    
    #define USBFS_EP_3_ISR_ENTRY_CALLBACK
    #define USBFS_EP_3_ISR_EntryCallback()  update_feedback();
    
#endif /* CYAPICALLBACKS_H */   
/* [] */
