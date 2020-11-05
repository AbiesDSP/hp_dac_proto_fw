#include "knobs/knobs.h"
#include "project.h"

volatile int16_t knobs[N_KNOBS];
volatile uint8_t knob_status;
static uint8_t adc_dma_ch;
static uint8_t adc_dma_td[1];
static volatile uint8_t adc_ch;

void knobs_start(void)
{
    uint16_t i = 0;
    for (i = 0; i < N_KNOBS; i++) {
        knobs[i] = 0;
    }
    knob_status = 0;
    adc_ch = 0;
    // Config DMA
    adc_dma_ch = DMA_ADC_DmaInitialize(2u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    adc_dma_td[0] = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(adc_dma_td[0], 6u, adc_dma_td[0], TD_INC_DST_ADR);
    CyDmaTdSetAddress(adc_dma_td[0], LO16((uint32_t)ADC_DEC_SAMP_PTR), LO16((uint32_t)&knobs[0]));
    CyDmaChSetInitialTd(adc_dma_ch, adc_dma_td[0]);
    CyDmaChEnable(adc_dma_ch, 1u);
    // Setup ADC
    VDAC_pot_Start();
    Opamp_pot_Start();
    AMuxSeq_Start();
    AMuxSeq_Next();
    adc_isr_StartEx(adcdone);
    ADC_Start();
    ADC_StartConvert();
}

CY_ISR(adcdone)
{
    // Select next adc channel
    AMuxSeq_Next();
    adc_ch++;
    adc_ch = adc_ch == N_KNOBS ? 0u : adc_ch;
    // Finished sampling all channels, we can update the main app now.
    if (adc_ch == 0) {
        knob_status |= KNOB_STS_NEW;
    }
    // Restart the adc.
    ADC_StartConvert();
}