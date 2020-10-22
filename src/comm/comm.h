#ifndef COMM_H
#define COMM_H

#include <stdint.h>
#include <stdlib.h>
#include <cytypes.h>

#define COMM_MAX_TRANSFER_SIZE  (4095u)
#define COMM_DELIM              (0x00u)
#define COMM_MAX_TX_TD          (128u)
#define COMM_MAX_RX_TD          (17u)
    
#define COMM_ERR_NONE           (0x0u)  // No error.
#define COMM_ERR_ALLOC          (0x1u)  // TD Allocation failed, or the comm ref is invalid.
#define COMM_ERR_PARAM          (0x2u)  // Alternate implementation. Not used.
#define COMM_ERR_TX_FULL        (0x3u)  // Not enough room in the tx_buffer
#define COMM_ERR_COBS           (0x4u)  // General cobs error
#define COMM_ERR_DISABLED       (0x5u) // App requested either a receive or send while it was stopped.
#define COMM_ERR_NODATA         (0x6u) // App requested a delimited packet, but there was none ready.
#define COMM_ERR_DELIM          (0x7u) // Status reported a delimeter, but none was found. Hopefully never happens.
#define COMM_ERR_INCOMPLETE     (0x8u) // Incomplete transfer. Buffer wasn't large enough to hold all data available.
    
#define COMM_TX_STS_ACTIVE      (0x01u) // Dma chains are currently active. It will terminate with an isr.
#define COMM_TX_STS_FULL        (0x02u)
#define COMM_TX_STS_OVERFLOW    (0x04u)

#define COMM_RX_STS_FULL        (0x01u) /* The rx_buffer will be full if a full transfer arrives before being cleared.*/
#define COMM_RX_STS_OVERFLOW    (0x02u) /* If the buffer is actually full, we have to stall until the rx_buffer is cleared.*/
                                        /* rx_spy will stall until it is read out, and eventually that will probably trigger
                                         * a uart error if it doesn't resume and more bytes arrive. Clear the buffer when it's
                                         * full if you do not want to lose data.
                                         */
#define COMM_RX_STS_NEW_PACKET  (0x04u) /* There are delimeters still in the buffer. If this is still asserted*/
                                        /* after calling receive_delim, it means there are more packets available.
                                         */
typedef struct comm_struct * comm;

/* DMA transfers are limited to 4095 bytes, so that's a hard limit.
 * tx_transfers may use up to 4095 byte transfers, but smaller transfer
 * sizes will update the buffer size more frequently. If the extra isrs
 * are okay, then smaller transfer sizes are better.
 * rx_transfers have a maximum of 256 bytes. Since packets are delimited
 * and flushed, there isn't a reason to use a smaller transfer size.
 * Unless you need to conserve TDs, use the maximum n.
 * 
 * The maximum buffer size is limited to a 16bit address for no reason in particular.
 * To transmit or receive the max packet size (65535u) would take 17 chained
 * tds, so that is our maximum allowed. If your sends are taking more than 17
 * tds because you have a small tx_transfer size, you may
 * need to make a larger transfer size.
 *
 *
 * The remaining parameters should be intuitive.
 */
typedef struct {
    // tx config
    uint8_t uart_tx_ch; // initialized dma channel for SRAM -> UART
    uint8_t uart_tx_n_td; // the number of tds to use for transmitting. <= MAX_TD
    uint8_t uart_tx_td_termout_en; // copy the macro definition from dma component.
    reg8 *uart_tx_fifo; // UART_TXDATA_PTR
    uint8_t *tx_buffer; // the SRAM buffer to use for transmitting. Nothing else can access this buffer.
    uint16_t tx_capacity; // size of tx_buffer
    uint16_t tx_transfer_size; // maximum transfer size for tx dma transfers.
    // rx config
    uint8_t uart_rx_ch; // dma channel for UART -> rx_spy
    reg8 *uart_rx_fifo; // UART_RXDATA_PTR
    uint8_t spy_ch; // dma channel for rx_spy -> SRAM
    uint8_t spy_n_td; //number of tds to use for rx_transfers. buffer_size >= 4095 * n_td
    uint8_t *rx_buffer; // sram buffer used for rx bytes.
    uint16_t rx_capacity; // size of rx_buffer.
    uint8_t rx_transfer_size; // maximum transfer size. Explained in detail above.
    uint8_t (*spy_service)(uint8_t*);
    void (*spy_resume)(uint8_t);
    const reg8 *spy_fifo_in;
    const reg8 *spy_fifo_out;
} comm_config;

// Initialize the DMA channels and make sure config is complete before creating.
comm comm_create(comm_config config);
void comm_destroy(comm self);
void comm_start(comm self);
void comm_stop(comm self);

// Buffer Monitoring
uint16_t comm_tx_buffer_size(comm self);
uint8_t comm_tx_status(comm self);
uint16_t comm_rx_buffer_size(comm self);
uint8_t comm_rx_status(comm self);
uint8_t comm_rx_clear(comm self, uint16_t amount);

// Number of bytes before the next delimeter
uint8_t comm_rx_next_pkt_size(comm self, uint16_t *size);
/* This is a blocking function. It will wait until the buffer
 * has enough space. Check the buffer size before calling
 * this function if you don't want it to hang.
 */
uint8_t comm_send(comm self, const uint8_t *buf, uint16_t size);

/* Similar to comm_send, but this will encode the buffer into
 * COBS and appends a delimeter before sending. Blocks.*/
uint8_t comm_tx_encode(comm self, const uint8_t *buf, uint16_t size);

/* Receive up the specified amount. This function will
 * block until the specified bytes arrive. If you do not want
 * this function to block, call rx_buffer_size before this
 * function and use that as the amount parameter.
 * This function assumes amount < buf_size. That's on you.
 */
uint8_t comm_receive(comm self, uint8_t *buf, uint16_t amount);

/* This function will transfer all bytes in the rx_buffer,
 * then will write the amount copied into *amount.
 * If buf_size < amount, this will return an incomplete error,
 * and will only copy buf_size bytes. You may need to call this
 * a few times if buf is smaller than the rx_buffer size.
 */
uint8_t comm_receive_all(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount);

/* rx_status COMM_RX_STS_NEW_PACKET must be set before calling this function.
 * It will return an error if there is no new packet in the buffer.
 *
 * If COMM_RX_STS_NEW_PACKET is set but there is no delimeter in the buffer (rare bug),
 * this will copy as many bytes as it can, then return an error and the amount.
 * 
 * This function will start copying bytes through the first delimeter in
 * the rx_buffer. *amount is the size of the packet including the delimeter.
 *
 * If there are more bytes before the delimeter than can fit in buf_size,
 * this will also return an error and will only copy buf_size bytes. It will
 * not decrement the delimeter_count unless a delimeter is removed from the buffer.
 */
uint8_t comm_receive_delim(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount);

/* Similar to receive delim, but this will COBS decode the result into the dst buffer
 */
uint8_t comm_rx_decode(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount);

// Call this function in tx dma done isr.
void comm_tx_isr(comm self);
// This is the rx_spy done isr
void comm_rx_isr(comm self);

#endif
