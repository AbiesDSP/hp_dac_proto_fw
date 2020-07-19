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

#define VOLUME  (200000)

static void dma_tx_config(void);
CY_ISR_PROTO(mute_button);

uint8_t i2s_out_dma_ch;
uint8_t i2s_out_dma_td[1];

volatile uint8_t mute_toggle = 0;

int main(void)
{
    // Enable mute button ISR
    mute_isr_StartEx(mute_button);
    mute_Write(0);
    
    // Compute the lookup table for the sine wave
    audio_table(VOLUME);
    
    // Start everything.
    I2S_Start();
    dma_tx_config();
    I2S_EnableTx();
    
    CyGlobalIntEnable;
    
    for ever
    {
        // Everything is handled in ISRs and DMA. Just do nothing for the endless eternity.
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

// Toggle the mute state whenever it is pushed.
CY_ISR(mute_button)
{
    mute_Write(mute_toggle ^= 1);
}
