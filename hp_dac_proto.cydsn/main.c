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
#include "dds/dds.h"
#include <math.h>

#define ever    (;;)
/* Each audio sample needs to be converted into a stream of bytes like so:
 * L-MSB, L-1, L-LSB, R-MSB, R-1, R-LSB.
 * I2S DMA request is on FIFO not-full, so we just need to pack all the bytes into it.
 * don't worry about dividing it up into samples, the I2S component figures it out.
 *
 * For this example, we're just looping through a single cycle of a 440Hz sine wave.
 * The transfer size is the number of bytes in the sin wave lookup table. The DMA resets
 * back to the beginning after it completes so it plays infinitely.
 *
 * When USB is integrated, we'll need something a little better like an actual circular buffer.
 *
 */

#define I2S_DMA_BYTES_PER_BURST     (1u)
#define I2S_DMA_REQUEST_PER_BURST   (1u)
#define I2S_DMA_DST_BASE            (CYDEV_PERIPH_BASE)
#define I2S_DMA_SRC_BASE            (CYDEV_SRAM_BASE)
#define I2S_DMA_TRANSFER_SIZE       (288u)
#define I2S_DMA_TD_COUNT            (8u)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)

#define USBFS_AUDIO_DEVICE  (0u)
#define AUDIO_INTERFACE     (1u)
#define USB_OUT_EP_NUM      (1u)

#define VOLUME  (200000)

static void dma_tx_config(void);
static void dma_tx_config2(void);

CY_ISR_PROTO(mute_button);
CY_ISR_PROTO(i2s_dma_done_isr);
CY_ISR_PROTO(onems_update_isr);

uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];

uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

volatile uint8_t mute_toggle = 1;
volatile uint32_t out_index = 0;
volatile uint32_t in_index = 0;
volatile uint32_t sync_dma = 0;
volatile uint32_t sync_dma_counter = 0;
volatile uint32_t dma_done = 0;
volatile uint8_t skip_next_out = 0;

uint16_t count = 0;
volatile uint32_t audio_index = 0;
volatile int32_t amplitude = VOLUME;
volatile uint8_t refresh_flag = 0;

int main(void)
{
    uint32_t i = 0, j = 0, k = 0;
    int32_t sin_table[109];
    int32_t sample;
    uint8_t temp[I2S_DMA_TRANSFER_SIZE];
    
    for (i = 0; i < 109; i++) {
        sin_table[i] = amplitude * sin((2*M_PI*i)/109);
    }
    
    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(mute_toggle);
    
    // Compute the lookup table for the sine wave
    audio_table(VOLUME);
    
    dma_done_isr_StartEx(i2s_dma_done_isr);
    I2S_Start();
    dma_tx_config2();
    
    CyGlobalIntEnable;
    
    // Start everything.
//    update_isr_StartEx(onems_update_isr);
//    Refresh_Timer_Start();
//    
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration());
    
    for ever
    {
        // USB Handler
        if (0u != USBFS_IsConfigurationChanged()) {
            // Check alt setting
            if ((0u != USBFS_GetConfiguration()) && (0u != USBFS_GetInterfaceSetting(AUDIO_INTERFACE))) {
                // Reset variables.
                in_index = 0u;
                out_index = 0u;
                sync_dma = 0u;
                skip_next_out = 0u;
                sync_dma_counter = 0u;
                USBFS_EnableOutEP(USB_OUT_EP_NUM);
            }
        }
        // New data
        if (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(USB_OUT_EP_NUM)) {
            // Read packet into buf
            USBFS_ReadOutEP(USB_OUT_EP_NUM, temp, I2S_DMA_TRANSFER_SIZE);
            while (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(USB_OUT_EP_NUM));
            
            for (i = 0; i < 48; i++) {
                for (j = 0; j < 2; j++) {
                    for (k = 0; k < 3; k++) {
                        samples[in_index * I2S_DMA_TRANSFER_SIZE + i*6 + j*3 + k] = temp[i*6 + j*3 + (2-k)];
                    }
                }
            }
            
            // Nevermind
            // Calculate next table entry. Simulate 48 samples
//            for (i = 0; i < 48; i++) {
//                sample = sin_table[audio_index];
//                audio_index++;
//                audio_index = (audio_index >= 109) ? 0u : audio_index;
//                for (j = 0; j < 2; j++) {
//                    for (k = 0; k < 3; k++) {
//                        temp[i*6 + j*3 + k] = 0xFF & ((sample) >> (8 * (2 - k)));
//                    }
//                }
//            }
//            memcpy(&samples[in_index * I2S_DMA_TRANSFER_SIZE], temp, I2S_DMA_TRANSFER_SIZE);
            
            // Move write pointer
            dma_done_isr_Disable();
            in_index++;
            in_index = (in_index >= I2S_DMA_TD_COUNT) ? 0u : in_index;
            dma_done_isr_Enable();
            sync_dma_counter++;
            
            USBFS_EnableOutEP(USB_OUT_EP_NUM);
        }

        // 440Hz tone
//        if (refresh_flag) {
//            refresh_flag = 0;
//            // Calculate next table entry. Simulate 48 samples
//            for (i = 0; i < 48; i++) {
//                sample = sin_table[audio_index];
//                audio_index++;
//                audio_index = (audio_index >= 109) ? 0u : audio_index;
//                for (j = 0; j < 2; j++) {
//                    for (k = 0; k < 3; k++) {
//                        temp[i*6 + j*3 + k] = 0xFF & ((sample) >> (8 * (2 - k)));
//                    }
//                }
//            }
//            memcpy(&samples[in_index * I2S_DMA_TRANSFER_SIZE], temp, I2S_DMA_TRANSFER_SIZE);
//            dma_done_isr_Disable();
//            in_index++;
//            in_index = (in_index >= I2S_DMA_TD_COUNT) ? 0u : in_index;
//            dma_done_isr_Enable();
//            sync_dma_counter++;
//        }
        // Half full
        if ((0u == sync_dma) && (sync_dma_counter == (I2S_DMA_TD_COUNT / 2u))) {
            CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
            sync_dma = 1;
            I2S_EnableTx();
        }
    }
}

static void dma_tx_config(void)
{
    // Every time the FIFO is not full, transfer one byte from SRAM to the I2S peripheral. 
    // DMA initialization
    i2s_out_dma_ch = DMA_I2S_DmaInitialize(I2S_DMA_BYTES_PER_BURST, I2S_DMA_REQUEST_PER_BURST, HI16(I2S_DMA_SRC_BASE), HI16(I2S_DMA_DST_BASE));
    i2s_out_dma_td[0] = CyDmaTdAllocate();
    // Set dma to increment through the LUT.
    CyDmaTdSetConfiguration(i2s_out_dma_td[0], I2S_DMA_TRANSFER_SIZE, i2s_out_dma_td[0], TD_INC_SRC_ADR);
    // DMA source is the byte buffer for samples, DMA dst is the I2S FIFO
    CyDmaTdSetAddress(i2s_out_dma_td[0], LO16((uint32)sample_byte_buf), LO16((uint32)I2S_TX_CH0_F0_PTR));
    // Start DMA
    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
    CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
}

// Chained DMA TD. Dump the full LUT into one half of the buffer while the
// destination pointer is incrementing through the other half.
static void dma_tx_config2(void)
{
    uint16_t i = 0;
    
    i2s_out_dma_ch = DMA_I2S_DmaInitialize(I2S_DMA_BYTES_PER_BURST, I2S_DMA_REQUEST_PER_BURST, HI16(I2S_DMA_SRC_BASE), HI16(I2S_DMA_DST_BASE));
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        i2s_out_dma_td[i] = CyDmaTdAllocate();
    }
    // Link buffers into a circular buffer.
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetConfiguration(i2s_out_dma_td[i], I2S_DMA_TRANSFER_SIZE, i2s_out_dma_td[(i + 1) % I2S_DMA_TD_COUNT], (TD_INC_SRC_ADR | DMA_I2S__TD_TERMOUT_EN));
    }
    // Set source and destination addresses
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(i2s_out_dma_td[i], LO16((uint32)&samples[i * I2S_DMA_TRANSFER_SIZE]), LO16((uint32)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
//    CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
}

// Toggle the mute state whenever it is pushed.
CY_ISR(mute_button)
{
    mute_Write(mute_toggle ^= 1);
}

CY_ISR(i2s_dma_done_isr)
{
    ++out_index;
    out_index = (out_index >= I2S_DMA_TD_COUNT) ? 0u : out_index;
    if (out_index == in_index) {
        CyDmaChDisable(i2s_out_dma_ch);
        I2S_DisableTx();
        sync_dma = 0;
        sync_dma_counter = 0;
    }
}

CY_ISR(onems_update_isr)
{
    refresh_flag = 1;
}
