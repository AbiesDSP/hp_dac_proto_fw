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
#define I2S_DMA_TRANSFER_SIZE       (N_BYTES)
#define I2S_DMA_TD_COUNT            (2u)
#define I2S_DMA_TD_TERMOUT_EN       (DMA_I2S__TD_TERMOUT_EN)
#define I2S_DMA_ENABLE_PRESERVE_TD  (1u)

#define VOLUME  (200000)

static void dma_tx_config(void);
static void dma_tx_config2(void);

CY_ISR_PROTO(mute_button);
CY_ISR_PROTO(i2s_dma_done_isr);

uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[I2S_DMA_TD_COUNT];

uint8_t samples[I2S_DMA_TRANSFER_SIZE * I2S_DMA_TD_COUNT];

volatile uint8_t mute_toggle = 0;
volatile uint8_t out_index = 0;
volatile uint8_t in_index = 0;
volatile uint8_t sync_dma = 0;
volatile uint8_t sync_dma_counter = 0;
volatile uint8_t dma_done = 0;

int main(void)
{
    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(0);
    
    // Compute the lookup table for the sine wave
    audio_table(VOLUME);
    
    dma_tx_config2();
    // Start everything.
    I2S_Start();
    I2S_EnableTx();
    
    CyGlobalIntEnable;
    
    for ever
    {
        // Halfway point. Copy buffer to new location
        if (dma_done) {
            dma_done = 0;
            dma_done_isr_Disable();
            memcpy(&samples[in_index * I2S_DMA_TRANSFER_SIZE], sample_byte_buf, I2S_DMA_TRANSFER_SIZE);
            in_index++;
            in_index = (in_index >= I2S_DMA_TD_COUNT) ? 0u : in_index;
            dma_done_isr_Enable();
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
    CyDmaChEnable(i2s_out_dma_ch, 1u);
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
        CyDmaTdSetConfiguration(i2s_out_dma_td[i], I2S_DMA_TRANSFER_SIZE, i2s_out_dma_td[(i + 1) % I2S_DMA_TD_COUNT], (TD_INC_SRC_ADR | I2S_DMA_TD_TERMOUT_EN));
    }
    // Set source and destination addresses
    for (i = 0; i < I2S_DMA_TD_COUNT; i++) {
        CyDmaTdSetAddress(i2s_out_dma_td[i], LO16((uint32)&samples[i * I2S_DMA_TRANSFER_SIZE]), LO16((uint32)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_out_dma_ch, i2s_out_dma_td[0]);
    CyDmaChEnable(i2s_out_dma_ch, I2S_DMA_ENABLE_PRESERVE_TD);
    dma_done_isr_StartEx(i2s_dma_done_isr);
}

// Toggle the mute state whenever it is pushed.
CY_ISR(mute_button)
{
    mute_Write(mute_toggle ^= 1);
}

CY_ISR(i2s_dma_done_isr)
{
    dma_done = 1;
    ++out_index;
    out_index = (out_index >= I2S_DMA_TD_COUNT) ? 0u : out_index;
}
