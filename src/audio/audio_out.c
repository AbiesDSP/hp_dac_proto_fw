#include "audio/audio_out.h"
#include "sync/sync.h"
#include "CyDmac.h"
#include "byte_swap_defs.h"
#include "USBFS.h"
#include "I2S.h"
#include "mute.h"

#define MAX_TRANSFER_SIZE   (4095u)
#define FIFO_HALF_FULL_MASK (0x0C)
#define N_BS_TD             ((AUDIO_OUT_BUF_SIZE + (MAX_TRANSFER_SIZE - 1))/MAX_TRANSFER_SIZE)

uint8_t audio_out_buf[AUDIO_OUT_BUF_SIZE];
volatile uint16_t audio_out_buffer_size;
volatile uint8_t audio_out_status;

volatile uint8_t audio_out_update_flag;

static uint8_t usb_dma_td[1];
static uint8_t bs_dma_td[N_BS_TD];
static uint8_t i2s_dma_td[AUDIO_OUT_N_TD];

static uint8_t usb_dma_ch;
static uint8_t usb_dma_termout_en;
static uint8_t bs_dma_ch;
static uint8_t bs_dma_termout_en;
static uint8_t i2s_dma_ch;
static uint8_t i2s_dma_termout_en;
static uint8_t *usb_buf;
static reg8 *bs_fifo_in;
static reg8 *bs_fifo_out;

static void bs_dma_config(void);
static void i2s_dma_config(void);

void audio_out_init(audio_out_config config)
{
    uint16_t i;

    usb_dma_ch = config.usb_dma_ch;
    usb_dma_termout_en = config.usb_dma_termout_en;
    bs_dma_ch = config.bs_dma_ch;
    bs_dma_termout_en = config.bs_dma_termout_en;
    i2s_dma_ch = config.i2s_dma_ch;
    i2s_dma_termout_en = config.i2s_dma_termout_en;
    usb_buf = config.usb_buf;
    bs_fifo_in = config.bs_fifo_in;
    bs_fifo_out = config.bs_fifo_out;

    // Allocate all TDs
    usb_dma_td[0] = CyDmaTdAllocate();
    for (i = 0; i < N_BS_TD; i++) {
        bs_dma_td[i] = CyDmaTdAllocate();
    }
    for (i = 0; i < AUDIO_OUT_N_TD; i++) {
        i2s_dma_td[i] = CyDmaTdAllocate();
    }

    // Initiliaze the buffer.
    for (i = 0; i < AUDIO_OUT_BUF_SIZE; i++) {
        audio_out_buf[i] = 0;
    }
    // Initialize buffer management.
    audio_out_buffer_size = 0;
    audio_out_status = 0;
    audio_out_update_flag = 0u;
    byte_swap_DP_F0_SET_LEVEL_MID;
    byte_swap_DP_F1_SET_LEVEL_MID;
    I2S_TX_AUX_CONTROL_REG = I2S_TX_AUX_CONTROL_REG | FIFO_HALF_FULL_MASK;
}

void audio_out_start(void)
{
    bs_dma_config();
    i2s_dma_config();
    // BS channel is always on. Other channels are intermittent.
    CyDmaChEnable(bs_dma_ch, 1u);
    I2S_Start();
}

void audio_out_update(void)
{
    uint8_t int_status;
    uint16_t count, buf_size;
    // We've received bytes from USB. We need to transfer to byte_swap.
    // Reset here for any reason?

    if (audio_out_status & AUDIO_OUT_STS_RESET) {
        audio_out_status &= ~AUDIO_OUT_STS_RESET;
        audio_out_buffer_size = 0;
    }

    count = USBFS_GetEPCount(AUDIO_OUT_EP);
    // Start a dma transaction
    CyDmaTdSetConfiguration(usb_dma_td[0], count, CY_DMA_DISABLE_TD, (TD_INC_SRC_ADR | usb_dma_termout_en));
    CyDmaTdSetAddress(usb_dma_td[0], LO16((uint32_t)&usb_buf[0]), LO16((uint32_t)bs_fifo_in));
    CyDmaChSetInitialTd(usb_dma_ch, usb_dma_td[0]);
    CyDmaChEnable(usb_dma_ch, 1u);
    
    int_status = CyEnterCriticalSection();
    audio_out_buffer_size += count;
    buf_size = audio_out_buffer_size;
    CyExitCriticalSection(int_status);

    // If it's off and we're over half full, start it up.
    if ((audio_out_status & AUDIO_OUT_STS_ACTIVE) == 0u && buf_size >= AUDIO_OUT_ACTIVE_LIMIT) {
        audio_out_enable();
    }
    audio_out_update_flag = 1u;
}

void audio_out_enable(void)
{
    // Only enable if we're currently enabled
    if ((audio_out_status & AUDIO_OUT_STS_ACTIVE) == 0u) {
        audio_out_status |= AUDIO_OUT_STS_ACTIVE;
        mute_Write(1u);
        CyDmaChEnable(i2s_dma_ch, 1u);
        sync_enable();
        I2S_EnableTx();
    }
}

void audio_out_disable(void)
{
    // Only disable if active
    if (audio_out_status & AUDIO_OUT_STS_ACTIVE) {
        sync_disable();
        I2S_DisableTx();
        CyDelayUs(20);  // Delays in isr? That's a paddlin.
        CyDmaChDisable(i2s_dma_ch);
        audio_out_status &= ~AUDIO_OUT_STS_ACTIVE;
        audio_out_buffer_size = 0;
    }
}

static void bs_dma_config(void)
{
    uint16_t i = 0;
    uint16_t remain, transfer_size;

    remain = AUDIO_OUT_BUF_SIZE;
    while (remain > 0) {
        transfer_size = remain > MAX_TRANSFER_SIZE ? MAX_TRANSFER_SIZE : remain;
        CyDmaTdSetConfiguration(bs_dma_td[i], transfer_size, bs_dma_td[(i + 1) % N_BS_TD], (TD_INC_DST_ADR | bs_dma_termout_en));
        CyDmaTdSetAddress(bs_dma_td[i], LO16((uint32_t)bs_fifo_out), LO16((uint32_t)&audio_out_buf[i * MAX_TRANSFER_SIZE]));
        i++;
        remain -= transfer_size;
    }
    CyDmaChSetInitialTd(bs_dma_ch, bs_dma_td[0]);
}

static void i2s_dma_config(void)
{
    uint16_t i = 0;
    // Set up chained tds to loop around entire audio out buffer.
    for (i = 0; i < AUDIO_OUT_N_TD; i++) {
        CyDmaTdSetConfiguration(i2s_dma_td[i], AUDIO_OUT_TRANSFER_SIZE, i2s_dma_td[(i + 1) % AUDIO_OUT_N_TD], (TD_INC_SRC_ADR | i2s_dma_termout_en));
        CyDmaTdSetAddress(i2s_dma_td[i], LO16((uint32_t)&audio_out_buf[i * AUDIO_OUT_TRANSFER_SIZE]), LO16((uint32_t)I2S_TX_CH0_F0_PTR));
    }
    CyDmaChSetInitialTd(i2s_dma_ch, i2s_dma_td[0]);
}

// This isr is when the I2S transfer completes.
CY_ISR(i2s_done_isr)
{
    audio_out_buffer_size -= AUDIO_OUT_TRANSFER_SIZE;
    // About to underflow. Oh nooooo.
    if (audio_out_buffer_size <= AUDIO_OUT_LOW_LIMIT) {
        // What do?
        audio_out_status |= AUDIO_OUT_STS_UNDERFLOW;
        audio_out_status |= AUDIO_OUT_STS_RESET;
        audio_out_disable();
    } else if (audio_out_buffer_size >= AUDIO_OUT_HIGH_LIMIT) {
        audio_out_status |= AUDIO_OUT_STS_OVERFLOW;
        audio_out_status |= AUDIO_OUT_STS_RESET;
        audio_out_disable();
    }
}
