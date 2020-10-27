#include "project.h"
#include "audio/audio_out.h"
#include "usb/usb.h"
#include "sync/sync.h"
#include "comm/comm.h"

#define ENABLE_BOOTLOAD (0u)
#define N_BYTES (3u)

#define ever    (;;)

#define RX_BUF_SIZE         (1024u)
#define RX_TRANSFER_SIZE    (255u)
#define TX_BUF_SIZE         (1024u)
#define TX_TRANSFER_SIZE    (255u)

volatile uint8_t mute_toggle = 1;
volatile comm comm_main;

CY_ISR_PROTO(mute_button);
CY_ISR_PROTO(bootload_isr);
CY_ISR_PROTO(txdoneisr);
CY_ISR_PROTO(spyisr);

// Converts a signed 24bit number into a signed 32bit number.
int32_t get_sample(uint8_t *buf)
{
    /* LSB will be 0. This probably isn't the right
     * way to go about resampling to 32bit. I would
     * imagine we need to do some kind of interpolation?
     * But I don't know much about resampling yet.
     */
    int32_t sample = 0;
    
    /* Dump bytes into a 32bit integer. Left justified will preserve sign.
     * This means we've effectively multiplied our value by 256. But we will
     * end up dividing our sample by 256 when we repack it.
    */
    sample = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | (uint32_t)(buf[2] << 8);
    
    return sample;
}
//
void put_sample_back_where_you_found_it_please(int32_t sample, uint8_t *buf)
{
    // Bottom 8bits don't matter.
    buf[0] = (sample >> 24) & 0xFF;
    buf[1] = (sample >> 16) & 0xFF;
    buf[2] = (sample >> 8) & 0xFF;
}

int main(void)
{
    CyGlobalIntEnable;
    // Enable mute button ISR. Configuring audio will automatically unmute as well.
    mute_isr_StartEx(mute_button);
    mute_Write(mute_toggle);
    
    // DMA Channels for audio out process.
    uint8_t usb_dma_ch, bs_dma_ch, i2s_dma_ch;
    usb_dma_ch = DMA_USB_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    bs_dma_ch = DMA_BS_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    i2s_dma_ch = DMA_I2S_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    
    // Buffer, dma, and register setup constants.
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
    
    // buffers used by comm library
    uint8_t rx_buf[RX_BUF_SIZE];
    uint8_t tx_buf[TX_BUF_SIZE];

    // Comm dma initialization.
    uint8_t uart_tx_dma_ch = DMATxUart_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    uint8_t uart_rx_dma_ch = DMARxUART_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_PERIPH_BASE));
    uint8_t spy_dma_ch = DMASpy_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    // Comm configuration settings.
    comm_config uart_config = {
        .uart_tx_ch = uart_tx_dma_ch,
        .uart_tx_n_td = COMM_MAX_TX_TD,
        .uart_tx_td_termout_en = DMATxUart__TD_TERMOUT_EN,
        .uart_tx_fifo = UART_TXDATA_PTR,
        .tx_buffer = tx_buf,
        .tx_capacity = TX_BUF_SIZE,
        .tx_transfer_size = TX_TRANSFER_SIZE,
        .uart_rx_ch = uart_rx_dma_ch,
        .uart_rx_fifo = UART_RXDATA_PTR,
        .spy_ch = spy_dma_ch,
        .spy_n_td = COMM_MAX_RX_TD,
        .rx_buffer = rx_buf,
        .rx_capacity = RX_BUF_SIZE,
        .rx_transfer_size = RX_TRANSFER_SIZE,
        .spy_service = rx_spy_service,
        .spy_resume = rx_spy_resume,
        .spy_fifo_in = rx_spy_fifo_in_ptr,
        .spy_fifo_out = rx_spy_fifo_out_ptr
    };
    
    uint8_t int_status;
    uint16_t log_dat;
    uint16_t read_ptr = 0;
    int32_t sample;
//    uint8_t error = 0;
//    uint8_t rx_status = 0;
//    uint16_t rx_size = 0, packet_size = 0;

    comm_main = comm_create(uart_config);
    
    UART_Start();
    tx_isr_StartEx(txdoneisr);
    spy_isr_StartEx(spyisr);
    
    rx_spy_start(COMM_DELIM, RX_TRANSFER_SIZE);
    comm_start(comm_main);
    
    bs_isr_StartEx(bs_done_isr);
    i2s_isr_StartEx(i2s_done_isr);
    audio_out_init(config);
    audio_out_start();
    
    boot_isr_StartEx(bootload_isr);

//    sync_init();
    usb_start();
    
    for ever
    {
        // USB Handler
        if (USBFS_GetConfiguration()) {
            usb_service();
        }
        // Feedback was updated. Send telemetry.
        if (fb_update_flag && audio_out_active) {
            fb_update_flag = 0;
            // Audio out buffer size is modified in an isr,
            // so we'll just grab a quick copy
            int_status = CyEnterCriticalSection();
            log_dat = audio_out_buffer_size;
            CyExitCriticalSection(int_status);
            // Send the data over the UART
            comm_send(comm_main, (uint8_t*)&log_dat, sizeof(log_dat));
        }
        /* Byte swap transfer finished. Ready to process data.
         * audio_out_shadow is how many bytes need processing.
         * When you remove data update the read ptr and shadow
         * to account for processed bytes.
         * Whatever operation you perform must finish processing the data
         * within 1ms or we'll have an overrun issue.
         */
        if (audio_out_update_flag) {
            audio_out_update_flag = 0;
            /* Loop through each audio sample. and apply a volume filter.
             * audio_out_shadow will always be a multiple of 6, and our buffer
             * size should always be a multiple of 6 as well. So we can take some
             * shortcuts with the read ptr update by updating it only once per sample
             * instead of once per byte. Makes it easier.
             */
            for (uint16_t i = 0; i < (audio_out_shadow / 3); i++) {
                // Convert bytes into signed integer.
                sample = get_sample(&audio_out_buf[read_ptr]);
                // divide volume by 16
                sample >>= 4;
                // Unpack modified sample into the buffer.
                put_sample_back_where_you_found_it_please(sample, &audio_out_buf[read_ptr]);
                // Update read pointer
                read_ptr += N_BYTES;
                read_ptr = read_ptr == AUDIO_OUT_BUF_SIZE ? 0u : read_ptr;
            }
            audio_out_shadow = 0;
        }
    }
}

// Toggle the mute state whenever it is pushed.
CY_ISR(mute_button)
{
    mute_Write(mute_toggle ^= 1);
}

CY_ISR(bootload_isr)
{
    #if ENABLE_BOOTLOAD
    Bootloadable_Load();
    #endif
}

// Packet finished transmitting isr.
CY_ISR(txdoneisr)
{
    comm_tx_isr(comm_main);
}

// Delim, complete transfer, or flush isr.
CY_ISR(spyisr)
{   
    comm_rx_isr(comm_main);
}
