#include "comm/comm.h"
#include "comm/comm_pvt.h"
#include "CyDmac.h"

// Allocates all TDs and validates config parameters.
comm comm_create(comm_config config)
{
    uint8_t error = COMM_ERR_NONE;
    comm self = malloc(sizeof(*self));

    if (self) {
        // Tx config error checking
        error = check_tx_config(config);
        if (error == COMM_ERR_NONE) {
            error = check_rx_config(config);
        }
        if (error == COMM_ERR_NONE) {
            error = check_spy_config(config);
        }
        if (error == COMM_ERR_NONE) {
            self->config = config;
            error = comm_allocate(self);
        }
        // All good?
        if (error == COMM_ERR_NONE) {
            comm_init(self);
        } else {
            free(self);
            self = NULL;
        }
    }

    return self;
}

// Frees all allocated resources.
void comm_destroy(comm self)
{
    uint8_t i;

    if (self) {
        if (self->enabled) {
            comm_stop(self);
        }
        for (i = 0; i < self->config.uart_tx_n_td; i++) {
            CyDmaTdFree(self->uart_tx_td[i]);
        }
        CyDmaTdFree(self->uart_rx_td[0]);
        for (i = 0; i < self->config.spy_n_td; i++) {
            CyDmaTdFree(self->spy_td[i]);
        }
        free(self);
    }
}

// Starts up the rx_spy and enables dma channels.
void comm_start(comm self)
{
    if (self && !self->enabled) {
        // Initialize all buffer pointers
        comm_init(self);
        // Only start if it's inactive
        config_rx_dma(self);
        self->enabled = 1u;
    }
}

// Stops rx operation and disables transmits.
void comm_stop(comm self)
{
    if (self && self->enabled) {
        // Only stop if its active
        CyDmaChDisable(self->config.uart_rx_ch);
        CyDmaChDisable(self->config.spy_ch);
        self->enabled = 0u;
    }
}

// Validates tx channel parameters
uint8_t check_tx_config(comm_config config)
{
    uint8_t error = COMM_ERR_NONE;
    
    // Error checking
    if (config.uart_tx_ch == CY_DMA_INVALID_CHANNEL) {
        error = COMM_ERR_PARAM;
    }
    if (config.uart_tx_fifo == NULL || config.tx_buffer == NULL) {
        error = COMM_ERR_PARAM;
    }
    if (config.uart_tx_n_td > COMM_MAX_TX_TD || config.tx_capacity == 0 || config.tx_capacity < config.tx_transfer_size) {
        error = COMM_ERR_PARAM;
    }
    if (config.tx_transfer_size > COMM_MAX_TRANSFER_SIZE) {
        error = COMM_ERR_PARAM;
    }
    
    return error;
}

// Validates rx channel parameters.
uint8_t check_rx_config(comm_config config)
{
    uint8_t error = COMM_ERR_NONE;

    if (config.uart_rx_ch == CY_DMA_INVALID_CHANNEL || config.spy_ch == CY_DMA_INVALID_CHANNEL) {
        error = COMM_ERR_ALLOC;
    }
    if (config.uart_rx_fifo == NULL || config.rx_buffer == NULL) {
        error = COMM_ERR_PARAM;
    }
    if (config.spy_n_td > COMM_MAX_RX_TD || config.rx_capacity == 0 || config.rx_capacity < config.rx_transfer_size) {
        error = COMM_ERR_PARAM;
    }
    if (config.spy_n_td * COMM_MAX_TRANSFER_SIZE < config.rx_capacity) {
        error = COMM_ERR_PARAM;
    }
    
    return error;
}

uint8_t check_spy_config(comm_config config)
{
    uint8_t error = COMM_ERR_NONE;
    
    if (config.spy_service == NULL || config.spy_resume == NULL) {
        error = COMM_ERR_PARAM;
    }
    if (config.spy_fifo_in == NULL || config.spy_fifo_out == NULL) {
        error = COMM_ERR_PARAM;
    }
    
    return error;
}

// Allocates Tds required for DMA channels.
uint8_t comm_allocate(comm self)
{
    uint8_t error = COMM_ERR_NONE, i;
    uint16_t n_tds;
    
    // Required number for tds.
    n_tds = (self->config.tx_capacity + (self->config.tx_transfer_size - 1)) / self->config.tx_transfer_size;
    n_tds = n_tds > self->config.uart_tx_n_td ? self->config.uart_tx_n_td : n_tds;
    n_tds = n_tds > COMM_MAX_TX_TD ? COMM_MAX_TX_TD : n_tds;
    // If we're using less than requested, update the config.
    self->config.uart_tx_n_td = n_tds;
    self->uart_tx_td = malloc(sizeof(uint8_t) * n_tds);
    if (self->uart_tx_td == NULL) {
        error = COMM_ERR_ALLOC;
    }
    if (error == COMM_ERR_NONE) {
        n_tds = (self->config.rx_capacity + (self->config.rx_transfer_size - 1));   // Maximum tds required for these settings.
        n_tds = n_tds > self->config.spy_n_td ? self->config.spy_n_td : n_tds;      // Need fewer tds than requested.
        self->config.spy_n_td = n_tds;
        self->spy_td = malloc(sizeof(uint8_t) * n_tds);
        if (self->spy_td == NULL) {
            free(self->uart_tx_td);
            error = COMM_ERR_ALLOC;
        }
    }
    if (error == COMM_ERR_NONE) {
        // Allocate the dma resources.
        for (i = 0; i < self->config.uart_tx_n_td; i++) {
            self->uart_tx_td[i] = CyDmaTdAllocate();
            if (self->uart_tx_td[i] == CY_DMA_INVALID_TD) {
                error = COMM_ERR_ALLOC;
            }
        }
        for (i = 0; i < self->config.spy_n_td; i++) {
            self->spy_td[i] = CyDmaTdAllocate();
            if (self->spy_td[i] == CY_DMA_INVALID_TD) {
                error = COMM_ERR_ALLOC;
            }
        }
    }
    // Only one td needed for the uart -> spy dma.
    self->uart_rx_td[0] = CyDmaTdAllocate();
    if (self->uart_tx_td[0] == CY_DMA_INVALID_TD) {
        error = COMM_ERR_ALLOC;
    }
    
    if (error != COMM_ERR_NONE) {
        // Free the tds we may have allocated.
        for (i = 0; i < self->config.uart_tx_n_td; i++) {
            CyDmaTdFree(self->uart_tx_td[i]);
        }
        CyDmaTdFree(self->uart_rx_td[0]);
        for (i = 0; i < self->config.spy_n_td; i++) {
            CyDmaTdFree(self->spy_td[0]);
        }
    }
    
    return error;
}

// Initializes buffers and pointers.
void comm_init(comm self)
{
    uint16_t i = 0;

    // zero the buffers.
    for (i = 0; i < self->config.tx_capacity; i++) {
        self->config.tx_buffer[i] = 0u;
    }
    for (i = 0; i < self->config.rx_capacity; i++) {
        self->config.rx_buffer[i] = 0u;
    }

    self->enabled = 0u;
    self->delimeter_count = 0u;
    self->tx_write_ptr= 0u;
    self->tx_read_ptr = 0u;
    self->tx_buffer_size = 0u;
    self->tx_shadow = 0u;
    self->tx_status = 0u;
    self->rx_write_ptr = 0u;
    self->rx_read_ptr = 0u;
    self->rx_buffer_size = 0u;
    self->rx_status = 0u;
}
