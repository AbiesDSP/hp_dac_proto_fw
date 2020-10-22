#include "project.h"
#include "audio/audio_out.h"
#include "usb/usb.h"
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
volatile uint32_t waitctr = 0;
volatile int bs0flag = 0, bs1flag = 0;

int main(void)
{
    CyGlobalIntEnable;
//    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(mute_toggle);
    uint8_t usb_dma_ch, bs_dma_ch, i2s_dma_ch;
    usb_dma_ch = DMA_USB_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    bs_dma_ch = DMA_BS_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    i2s_dma_ch = DMA_I2S_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    
    audio_out_config config = {
        .usb_dma_ch = usb_dma_ch,
        .usb_dma_termout_en = DMA_USB__TD_TERMOUT_EN,
        .bs_dma_ch = bs_dma_ch,
        .bs_dma_termout_en = DMA_BS__TD_TERMOUT_EN,
        .i2s_dma_ch = i2s_dma_ch,
        .i2s_dma_termout_en = DMA_I2S__TD_TERMOUT_EN,
        .usb_buf = usb_out_buf,
        .bs_fifo_in = byte_swap_fifo_in_ptr,
        .bs_fifo_out = byte_swap_fifo_out_ptr
    };
    
    i2s_isr_StartEx(i2s_done_isr);
    audio_out_init(config);
    audio_out_start();
    
   dma_sync_config();
    
    boot_isr_StartEx(bootload_isr);
    sof_isr_StartEx(sync_isr);

    usb_start();
    
    for ever
    {
        // USB Handler
        if (USBFS_GetConfiguration()) {
            usb_service();
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

CY_ISR(bootload_isr)
{
//    Bootloadable_Load();
}

CY_ISR(sync_isr)
{
    mean_flag = 1;
    CyDmaChDisable(sync_dma_ch);
}
