#include "project.h"
#include "audio/audio_out.h"
#include "usb/usb.h"
#include "sync/sync.h"
#include "comm/comm.h"

#include <math.h>

#define ever    (;;)

#define TEST_BUF_SIZE       (8u)
#define RX_BUF_SIZE         (1024u)
#define RX_TRANSFER_SIZE    (255u)
#define TX_BUF_SIZE         (1024u)
#define TX_TRANSFER_SIZE    (255u)

/* Each audio sample needs to be converted into a stream of bytes like so:
 * L-MSB, L-1, L-LSB, R-MSB, R-1, R-LSB.
 * I2S DMA request is on FIFO not-full, so we just need to pack all the bytes into it.
 * don't worry about dividing it up into samples, the I2S component figures it out.
 *
 * 
 *
 */
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

volatile comm comm_main;

CY_ISR_PROTO(txdoneisr);
CY_ISR_PROTO(spyisr);

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
    
    uint8_t temp_buf[TEST_BUF_SIZE];
    // buffers used by comm library
    uint8_t rx_buf[RX_BUF_SIZE];
    uint8_t tx_buf[TX_BUF_SIZE];

    // All dma transfers are 1 byte burst and 1 byte per request.
    uint8_t uart_tx_dma_ch = DMATxUart_DmaInitialize(1u, 1u, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    uint8_t uart_rx_dma_ch = DMARxUART_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_PERIPH_BASE));
    uint8_t spy_dma_ch = DMASpy_DmaInitialize(1u, 1u, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    
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
    uint16_t buf_size;
    uint8_t error = 0;
    uint8_t rx_status = 0;
    uint16_t rx_size = 0, packet_size = 0;

    comm_main = comm_create(uart_config);
    
    UART_Start();
    tx_isr_StartEx(txdoneisr);
    spy_isr_StartEx(spyisr);
    
    rx_spy_start(COMM_DELIM, RX_TRANSFER_SIZE);
    comm_start(comm_main);
    
    i2s_isr_StartEx(i2s_done_isr);
    audio_out_init(config);
    audio_out_start();
    
    boot_isr_StartEx(bootload_isr);

    sync_init();
    usb_start();
    
    for ever
    {
        // USB Handler
        if (USBFS_GetConfiguration()) {
            usb_service();
        }
        if (audio_out_update_flag) {
            audio_out_update_flag = 0;
            int_status = CyEnterCriticalSection();
            buf_size = audio_out_buffer_size;
            CyExitCriticalSection(int_status);
            comm_send(comm_main, (uint8_t*)&buf_size, sizeof(buf_size));
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
//    Bootloadable_Load();
}

// Packet finished transmitting isr.
CY_ISR(txdoneisr)
{
    comm_tx_isr(comm_main);
}

// Delim, complete transfer, or flush isr.
CY_ISR_PROTO(spyisr)
{   
    comm_rx_isr(comm_main);
}
