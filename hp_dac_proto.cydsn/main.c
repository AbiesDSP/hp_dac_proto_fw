#include "project.h"
#include <math.h>

#define ever    (;;)
/* Each audio sample needs to be converted into a stream of bytes like so:
 * L-MSB, L-1, L-LSB, R-MSB, R-1, R-LSB.
 * I2S DMA request is on FIFO not-full, so we just need to pack all the bytes into it.
 * don't worry about dividing it up into samples, the I2S component figures it out.
 *
 * 
 *
 */

#define SYNC_BYTES_PER_BURST    (2u)
#define SYNC_REQUEST_PER_BURST  (1u)
#define SYNC_SRC_BASE           (CYDEV_PERIPH_BASE)
#define SYNC_DST_BASE           (CYDEV_SRAM_BASE)
#define SYNC_TRANSFER_SIZE      (SAMPLE_RATE_BUF_SIZE * SYNC_BYTES_PER_BURST)
#define SYNC_TD_COUNT           (1u)
#define SYNC_ENABLE_PRESERVE_TD (1u)
static void dma_sync_config(void);
uint8_t sync_dma_ch;
uint8_t sync_dma_td[SYNC_TD_COUNT];

#define SAMPLE_RATE_BUF_SIZE    (128u)
volatile uint16_t sample_rate_buf[SAMPLE_RATE_BUF_SIZE];

CY_ISR_PROTO(mute_button);
CY_ISR_PROTO(i2s_dma_done_isr);
CY_ISR_PROTO(bootload_isr);
CY_ISR_PROTO(sync_isr);

volatile uint8_t mute_toggle = 1;
volatile uint8_t sync_flag;
volatile uint32_t mean = 0;
volatile uint8_t mean_flag = 0;
volatile uint16_t usb_read_size = 0;
volatile uint8_t readflag = 0;

volatile uint8_t bugflag = 0;
volatile uint32_t usbbufsize = 0;

int main(void)
{
    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(mute_toggle);
    uint32_t z = 0;
    
    i2s_isr_StartEx(i2s_dma_done_isr);
    I2S_Start();
    dma_tx_config();
    dma_sync_config();
    
    boot_isr_StartEx(bootload_isr);
    sof_isr_StartEx(sync_isr);
    
    CyGlobalIntEnable;
    
    // Start and enumerate USB.
    USBFS_Start(USBFS_AUDIO_DEVICE, USBFS_DWR_VDDD_OPERATION);
    while (0u == USBFS_GetConfiguration());
    
    for ever
    {
        // This is to measure differences between usb and local clock.
        if (mean_flag) {
            mean_flag = 0;
            mean = 0;
            for (z = 0; z < SAMPLE_RATE_BUF_SIZE; z++) {
                mean += sample_rate_buf[z];
            }
            // Format feedback as 
            sample_rate_feedback = mean / 6;
            
            // Oversampling. Preserve decimal.
            mean >>= 4;
            for (z = 0; z < SAMPLE_RATE_BUF_SIZE; z++) {
                sample_rate_buf[z] = 0;
            }
            CyDmaChEnable(sync_dma_ch, SYNC_ENABLE_PRESERVE_TD);
        }
        
        // USB Handler
        if (USBFS_GetConfiguration()) {
            service_usb();
        }
    }
}

// For the sof measurements.
static void dma_sync_config(void)
{
    sync_dma_ch = DMA_Sync_DmaInitialize(SYNC_BYTES_PER_BURST, SYNC_REQUEST_PER_BURST, HI16(SYNC_SRC_BASE), HI16(SYNC_DST_BASE));
    sync_dma_td[0] = CyDmaTdAllocate();
    
    CyDmaTdSetConfiguration(sync_dma_td[0], SYNC_TRANSFER_SIZE, sync_dma_td[0], (TD_INC_DST_ADR | DMA_Sync__TD_TERMOUT_EN));
    CyDmaTdSetAddress(sync_dma_td[0], LO16((uint32)sync_counter_DP_F0_PTR), LO16((uint32)sample_rate_buf));
    CyDmaChSetInitialTd(sync_dma_ch, sync_dma_td[0]);
    CyDmaChEnable(sync_dma_ch, SYNC_ENABLE_PRESERVE_TD);
}

// Toggle the mute state whenever it is pushed.
CY_ISR(mute_button)
{
    mute_Write(mute_toggle ^= 1);
}

CY_ISR(i2s_dma_done_isr)
{
    uint32_t added;
    
    added = out_usb_count - out_usb_shadow;
    out_level += added;
    out_usb_shadow = out_usb_count;
    
    // Underflow
    if (out_level <= QUAR) {
//        audio_stop();
    } else {
        out_level -= I2S_DMA_TRANSFER_SIZE;
    }
    // Overflow
    if (out_level > (AUDIO_BUF_SIZE + 6)) {
        audio_stop();
    }
}

CY_ISR(bootload_isr)
{
//    Bootloadable_Load();
}

CY_ISR(sync_isr)
{
    mean_flag = 1;
    CyDmaChDisable(sync_dma_ch);
}
