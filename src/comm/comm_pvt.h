#ifndef COMM_PVT_H
#define COMM_PVT_H

#include "comm/comm.h"
#include "comm/cobs.h"

// private declarations
#define DELIM_NOT_FOUND (0xFFFFu)    
uint8_t check_tx_config(comm_config config);
uint8_t check_rx_config(comm_config config);
uint8_t check_spy_config(comm_config config);
uint8_t comm_allocate(comm self);
void comm_init(comm self);
void config_tx_dma(comm self);
void write_tx_buf(comm self, const uint8_t *src, uint16_t transfer_size);
void config_rx_dma(comm self);
void read_rx_buf(comm self, uint8_t *dst, uint8_t transfer_size);
uint16_t delim_location(comm self, uint16_t search_size, uint16_t *n_delim);

// Comm internals.
struct comm_struct
{
    comm_config config;
    uint8_t enabled;
    uint16_t delimeter_count;
    // Tx buffer management
    uint8_t *uart_tx_td;
    uint16_t tx_write_ptr;
    volatile uint16_t tx_read_ptr;
    volatile uint16_t tx_buffer_size;
    volatile uint16_t tx_shadow;
    volatile uint8_t tx_status;
    // Rx buffer management
    uint8_t uart_rx_td[1u];
    uint8_t *spy_td;
    volatile uint16_t rx_write_ptr;
    volatile uint16_t rx_read_ptr;
    volatile uint16_t rx_buffer_size;
    volatile uint8_t rx_status;
};

#endif
