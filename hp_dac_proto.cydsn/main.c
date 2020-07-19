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
#include "project.h"

#define USBFS_AUDIO_DEVICE  (0u)
#define AUDIO_INTERFACE     (1u)
#define OUT_EP_NUM          (1u)
#define USB_MAX_PKT         (288u)

#define TRANSFER_SIZE   (4u)
#define NUM_OF_BUFFERS  (8u)
#define BUFFER_SIZE     (TRANSFER_SIZE * NUM_OF_BUFFERS)

uint8_t sound_buf[BUFFER_SIZE];
volatile uint16_t out_index = 0u;
volatile uint16_t in_index = 0u;

volatile uint8_t sync_dma = 0u;
volatile uint8_t sync_dma_counter = 0u;

uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[NUM_OF_BUFFERS];

uint8_t usb_state = 0;
uint8_t usb_active = 0;

#define I2S_DMA_BYTES_PER_BURST     (1u)
#define I2S_DMA_REQUEST_PER_BURST   (1u)
#define I2S_DMA_TD_TERMOUT_EN       (DMA_I2S__TD_TERMOUT_EN)
#define I2S_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define I2S_DMA_SRC_BASE            (sound_buf)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)


static void dma_tx_config(void);
void service_usb(void);
void handle_usb_host(void);
CY_ISR_PROTO(i2s_dma_done);

int main(void)
{
    
    uint16_t count = 0;
    I2S_Start();
    dma_tx_config();
    
    CyGlobalIntEnable;
    
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration());
    
    for(;;)
    {
        if (USBFS_GetConfiguration() != 0) {
            service_usb();
        }
        // Configuration changed.
        if (0u != USBFS_IsConfigurationChanged()) {
            // Check active interface setting
            if ((0u != USBFS_GetConfiguration()) && (0u != USBFS_GetInterfaceSetting(AUDIO_INTERFACE)))
            {
                // Alt setting 1: stream
                in_index = 0u;
                out_index = 0u;
                sync_dma = 0u;
                sync_dma_counter = 0u;
                USBFS_EnableOutEP(OUT_EP_NUM);
                mute_Write(1u);
            } else {
                // Mute?
                mute_Write(0u);
            }
        }
        
        (void)count;
        // Received data.
        if (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(OUT_EP_NUM)) {
            USBFS_ReadOutEP(OUT_EP_NUM, &sound_buf[in_index * TRANSFER_SIZE], TRANSFER_SIZE);
            while (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(OUT_EP_NUM));  // Wait.
            dma_done_isr_Disable();
            in_index++;
            in_index = (in_index >= NUM_OF_BUFFERS) ? 0u : in_index;
            dma_done_isr_Enable();
            sync_dma_counter++;
            USBFS_EnableOutEP(OUT_EP_NUM);
        }
        // Dma transfers when half full...half empty?
        if ((sync_dma == 0u) && (sync_dma_counter == (NUM_OF_BUFFERS / 2u)))
        {
            CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
            sync_dma = 1u;
        }
    }
}

static void dma_tx_config(void)
{
    uint8_t i = 0;
    
    // Init dma channel
    i2s_out_dma_ch = DMA_I2S_DmaInitialize(I2S_DMA_BYTES_PER_BURST, I2S_DMA_REQUEST_PER_BURST,
                                            HI16(CYDEV_SRAM_BASE), HI16(I2S_DMA_DST_BASE));
    // Allocate transfer descriptors
    for (i = 0; i < NUM_OF_BUFFERS; i++) {
        i2s_out_dma_td[i] = CyDmaTdAllocate();
    }
    // Configure TD
    for (i = 0; i < (NUM_OF_BUFFERS - 1); i++) {
        CyDmaTdSetConfiguration(i2s_out_dma_td[i], TRANSFER_SIZE, i2s_out_dma_td[(i + 1) % NUM_OF_BUFFERS], (TD_INC_SRC_ADR | I2S_DMA_TD_TERMOUT_EN));
    }
    
    for (i = 0; i < NUM_OF_BUFFERS; i++) {
        CyDmaTdSetAddress(i2s_out_dma_td[i], LO16((uint32_t)&sound_buf), LO16((uint32_t)I2S_TX_CH0_F0_PTR)); // Why do I cast like this?
    }
    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
    dma_done_isr_StartEx(i2s_dma_done);
}

void service_usb(void)
{
    uint16_t i = 0;
    
    // Inactive
    if (usb_state == 0) {
        usb_state = 1;
    }
    // initialize
    if (usb_state == 1) {
        usb_state = 2;
        for (i = 0; i < USB_MAX_PKT; i++) {
            sound_buf[i] = 0;
        
}

void handle_usb_host(void)
{
    if (!USBFS_initVar) {   // no idea what that is.
        USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
        usb_state = 1;
        usb_active = 1;
    }
}

CY_ISR_PROTO(i2s_dma_done)
{
    out_index++;
    out_index = (out_index >= NUM_OF_BUFFERS) ? 0u : out_index;
    
    if (out_index == in_index) {
        CyDmaChDisable(i2s_out_dma_ch);
        sync_dma = 0u;
        sync_dma_counter = 0u;
    }
}

/* [] END OF FILE */
