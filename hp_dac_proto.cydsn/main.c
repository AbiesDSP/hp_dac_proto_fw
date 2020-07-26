#include "project.h"
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
static void dma_tx_config(void);
uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];

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

#define USBFS_AUDIO_DEVICE  (0u)
#define AUDIO_INTERFACE     (1u)
#define USB_OUT_EP_NUM      (1u)

#define SAMPLE_RATE_BUF_SIZE    (256u)
volatile uint16_t sample_rate_buf[SAMPLE_RATE_BUF_SIZE];

CY_ISR_PROTO(mute_button);
CY_ISR_PROTO(i2s_dma_done_isr);
CY_ISR_PROTO(bootload_isr);
CY_ISR_PROTO(sync_isr);

uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

volatile uint8_t mute_toggle = 1;
volatile uint32_t out_index = 0;
volatile uint32_t in_index = 0;
volatile uint32_t sync_dma = 0;
volatile uint32_t sync_dma_counter = 0;
volatile uint8_t sync_flag;
volatile uint32_t mean = 0;
volatile uint8_t mean_flag = 0;

int main(void)
{
    uint32_t i = 0, j = 0, k = 0, z = 0;
    uint8_t temp[I2S_DMA_TRANSFER_SIZE];
    
    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(mute_toggle);
    
    dma_done_isr_StartEx(i2s_dma_done_isr);
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
        if (mean_flag) {
            mean_flag = 0;
            mean = 0;
            for (z = 0; z < SAMPLE_RATE_BUF_SIZE; z++) {
                mean += sample_rate_buf[z];
            }
            // Oversampling
            mean >>= 4;
            for (z = 0; z < SAMPLE_RATE_BUF_SIZE; z++) {
                sample_rate_buf[z] = 0;
            }
            CyDmaChEnable(sync_dma_ch, SYNC_ENABLE_PRESERVE_TD);
            (void)mean;
        }
        
        // USB Handler
        if (0u != USBFS_IsConfigurationChanged()) {
            // Check alt setting
            if ((0u != USBFS_GetConfiguration()) && (0u != USBFS_GetInterfaceSetting(AUDIO_INTERFACE))) {
                // Reset sync variables.
                in_index = 0u;
                out_index = 0u;
                sync_dma = 0u;
                sync_dma_counter = 0u;
                USBFS_EnableOutEP(USB_OUT_EP_NUM);
            } else {
                // mute? idk.
            }
        }
        // New data
        if (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(USB_OUT_EP_NUM)) {
            // TODO: Still need to make sure the computer data rate
            // matches the PSoC data rate..not totally done yet.
            
            // Read packet into buf. This initiates a dma transfer so
            // you could be doing something useful in the while loop.
            USBFS_ReadOutEP(USB_OUT_EP_NUM, temp, I2S_DMA_TRANSFER_SIZE);
            while (USBFS_OUT_BUFFER_FULL == USBFS_GetEPState(USB_OUT_EP_NUM));
            
            // Reverse byte order. This could(should) be done in hardware with a dma transfer.
            for (i = 0; i < 48; i++) {
                for (j = 0; j < 2; j++) {
                    for (k = 0; k < 3; k++) {
                        samples[in_index * I2S_DMA_TRANSFER_SIZE + i*6 + j*3 + k] = temp[i*6 + j*3 + (2-k)];
                    }
                }
            }
            
            // Move write pointer
            dma_done_isr_Disable();
            in_index++;
            in_index = (in_index >= I2S_DMA_TD_COUNT) ? 0u : in_index;
            dma_done_isr_Enable();
            sync_dma_counter++;
            
            USBFS_EnableOutEP(USB_OUT_EP_NUM);
        }

        // Half full
        if ((0u == sync_dma) && (sync_dma_counter == (I2S_DMA_TD_COUNT / 2u))) {
            CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
            sync_dma = 1;
            I2S_EnableTx();
        }
    }
}

// Chained DMA TD. Dump the full LUT into one half of the buffer while the
// destination pointer is incrementing through the other half.
static void dma_tx_config(void)
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
}

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
    ++out_index;
    out_index = (out_index >= I2S_DMA_TD_COUNT) ? 0u : out_index;
    if (out_index == in_index) {
        CyDmaChDisable(i2s_out_dma_ch);
        I2S_DisableTx();
        sync_dma = 0;
        sync_dma_counter = 0;
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
