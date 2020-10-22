#include "comm/comm.h"
#include "comm/comm_pvt.h"
#include "comm/crc.h"
#include "CyDmac.h"

uint16_t comm_tx_buffer_size(comm self)
{
    size_t size = 0;
    uint8_t int_status = 0;

    if (self) {
        int_status = CyEnterCriticalSection();
        size = self->tx_buffer_size;
        CyExitCriticalSection(int_status);
    }

    return size;
}

uint8_t comm_tx_status(comm self)
{
    uint8_t status = 0, int_status;

    if (self) {
        int_status = CyEnterCriticalSection();
        status = self->tx_status;
        CyExitCriticalSection(int_status);
    }

    return status;
}

uint8_t comm_send(comm self, const uint8_t *buf, uint16_t size)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t free_space = 0, amount = 0, buf_index = 0;
    uint8_t int_status = 0;

    if (self && self->enabled) {
        if (buf && size > 0) {
            // Fill up dma buffer in chunks. Block until chunks are free.
            while (size > 0) {
                // buffer_size is modified in an isr.
                int_status = CyEnterCriticalSection();
                free_space = self->config.tx_capacity - self->tx_buffer_size;
                CyExitCriticalSection(int_status);
                // Write the buffer in chunks.
                amount = (size > self->config.tx_transfer_size) ? self->config.tx_transfer_size : size;
                // Block until free. Must have at least free space.
                if (free_space >= amount) {
                    // Write to the buf.
                    write_tx_buf(self, &buf[buf_index], amount);
                    buf_index += amount;
                    size -= amount;
                }
                // Flush the buffer if we're full.
                if (self->tx_status & COMM_TX_STS_FULL) {
                    // Start dma.
                    if ((self->tx_status & COMM_TX_STS_ACTIVE) == 0u) {
                        self->tx_status |= COMM_TX_STS_ACTIVE;
                        config_tx_dma(self);
                    }
                }
                // Do anything else? Timeout? Un-block condition?
            }
            // load up dma if currently not active.
            if ((self->tx_status & COMM_TX_STS_ACTIVE) == 0u) {
                self->tx_status |= COMM_TX_STS_ACTIVE;
                config_tx_dma(self);
            }
        } else {
            error = COMM_ERR_PARAM;
        }
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }

    return error;
}

uint8_t comm_tx_encode(comm self, const uint8_t *buf, uint16_t size)
{
    uint8_t error = COMM_ERR_NONE, cobs_error = COMM_ERR_NONE;
    uint16_t free_space = 0, amount = 0, buf_index = 0, copied = 0;
    uint8_t int_status = 0;

    if (self && self->enabled) {
        if (buf && size > 0) {
            // Fill up dma buffer in chunks. Block until chunks are free.
            while (size > 0) {
                // buffer_size is modified in an isr.
                int_status = CyEnterCriticalSection();
                free_space = self->config.tx_capacity - self->tx_buffer_size;
                CyExitCriticalSection(int_status);
                // Write in multiples of transfer size.
                // For COBS, tx_transfer_size must be >= 255u
                amount = (size > self->config.tx_transfer_size) ? self->config.tx_transfer_size : size;
                amount = amount > COBS_RUN_SIZE ? COBS_RUN_SIZE : amount;
                // Block until free. Must have enough free.
                if (free_space >= amount) {
                    // Write to the buf.
                    // write_tx_buf(self, &buf[buf_index], amount);
                    cobs_error = cobs_encode(self, &buf[buf_index], amount, &copied);
                    if (cobs_error == COMM_ERR_NONE) {
                        buf_index += amount;
                        size -= amount;
                        free_space -= amount;
                    }
                }
                // Flush the buffer if we're full.
                if (self->tx_status & COMM_TX_STS_FULL) {
                    // Start dma.
                    if ((self->tx_status & COMM_TX_STS_ACTIVE) == 0u) {
                        self->tx_status |= COMM_TX_STS_ACTIVE;
                        config_tx_dma(self);
                    }
                }
                // Do anything else? Timeout? Un-block condition?
            }
            // If there were exactly 254 free bytes and we encode 254 bytes, this can happen. I'm being thorough.
            // Careful this doesn't lock up though...
            while (free_space == 0) {
                int_status = CyEnterCriticalSection();
                free_space = self->config.tx_capacity - self->tx_buffer_size;
                CyExitCriticalSection(int_status);
            }

            // Append the delimeter
            self->config.tx_buffer[self->tx_write_ptr] = COMM_DELIM;
            self->tx_write_ptr++;
            self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
            int_status = CyEnterCriticalSection();
            self->tx_buffer_size++;
            CyExitCriticalSection(int_status);
            // load up dma if currently not active.
            if ((self->tx_status & COMM_TX_STS_ACTIVE) == 0u) {
                self->tx_status |= COMM_TX_STS_ACTIVE;
                config_tx_dma(self);
            }
        } else {
            error = COMM_ERR_PARAM;
        }
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }

    return error;
}

void write_tx_buf(comm self, const uint8_t *src, uint16_t size)
{
    uint16_t remain = 0;
    uint8_t int_status = 0;
    
    // rollover
    if (self->tx_write_ptr + size > self->config.tx_capacity) {
        remain = self->config.tx_capacity - self->tx_write_ptr;
        memcpy(&self->config.tx_buffer[self->tx_write_ptr], src, remain);
        memcpy(&self->config.tx_buffer[0], &src[remain], size - remain);
        self->tx_write_ptr = size - remain;
        // buffer_size is modified in the interrupt so this needs to be atomic.
        int_status = CyEnterCriticalSection();
        self->tx_buffer_size += size;
        remain = self->tx_buffer_size;
        CyExitCriticalSection(int_status);
    } else {
        // No need for rollover
        memcpy(&self->config.tx_buffer[self->tx_write_ptr], src, size);
        self->tx_write_ptr += size;
        self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
        // see above note.
        int_status = CyEnterCriticalSection();
        self->tx_buffer_size += size;
        remain = self->tx_buffer_size;
        CyExitCriticalSection(int_status);
    }
    // You will need to call config_tx_dma if it's full to flush the buffer out.
    if (remain + self->config.tx_transfer_size >= self->config.tx_capacity) {
        self->tx_status |= COMM_TX_STS_FULL;
    }
}

/* Loads up the dma with whatever is in the buffer.
 * By design, the tx_isr should never happen when this function is called.
 * So I did not include any critical sections here.
 */
void config_tx_dma(comm self)
{
    uint16_t shadow = 0, temp_read_ptr = 0, remain = 0, buf_size = 0;
    uint8_t next_td = 0, i;

    if(self->tx_buffer_size > 0) {
        // Create dma chain. remain now means "remaining bytes in buffer"
        shadow = self->tx_buffer_size;
        buf_size = self->tx_buffer_size;
        // Used to walkthrough where the TD boundaries are without affecting the read index.
        temp_read_ptr = self->tx_read_ptr;
        i = 0;
        while (shadow > 0) {
            remain = self->config.tx_capacity - temp_read_ptr;
            /* If we run out of tds, we need to end before we can clear out
             * the buffer. The remaining will be picked up by the next chain.
             */
            if (i == (self->config.uart_tx_n_td - 1)) {
                next_td = CY_DMA_DISABLE_TD;
            } else {
                next_td = self->uart_tx_td[i + 1];
            }
            // memory boundary
            if (shadow > remain && remain < self->config.tx_transfer_size) {
                CyDmaTdSetConfiguration(self->uart_tx_td[i], remain, next_td, (TD_INC_SRC_ADR | self->config.uart_tx_td_termout_en));
                CyDmaTdSetAddress(self->uart_tx_td[i], LO16((uint32_t)&self->config.tx_buffer[temp_read_ptr]), LO16((uint32_t)self->config.uart_tx_fifo));
                shadow -= remain;
                temp_read_ptr = 0;
            } else if (shadow <= self->config.tx_transfer_size) {
                // Last packet
                CyDmaTdSetConfiguration(self->uart_tx_td[i], shadow, CY_DMA_DISABLE_TD, (TD_INC_SRC_ADR | self->config.uart_tx_td_termout_en));
                CyDmaTdSetAddress(self->uart_tx_td[i], LO16((uint32_t)&self->config.tx_buffer[temp_read_ptr]), LO16((uint32_t)self->config.uart_tx_fifo));
                temp_read_ptr += shadow;
                shadow = 0;
            } else {
                // Full transfer
                CyDmaTdSetConfiguration(self->uart_tx_td[i], self->config.tx_transfer_size, next_td, (TD_INC_SRC_ADR | self->config.uart_tx_td_termout_en));
                CyDmaTdSetAddress(self->uart_tx_td[i], LO16((uint32_t)&self->config.tx_buffer[temp_read_ptr]), LO16((uint32_t)self->config.uart_tx_fifo));
                shadow -= self->config.tx_transfer_size;
                temp_read_ptr += self->config.tx_transfer_size;
            }
            i++;
            // This was the last td.
            if (i == self->config.uart_tx_n_td) {
                // Leave the while loop early.
                break;
            }
        }
        // The shadow is the total number of bytes in the dma chain.
        self->tx_shadow = buf_size - shadow;
        CyDmaChSetInitialTd(self->config.uart_tx_ch, self->uart_tx_td[0]);
        CyDmaChEnable(self->config.uart_tx_ch, 1u);
    }
}

/* This isr is at the end of every DMA transfer.
 * We can determine what the last packet size is
 * using the current read pointer and the shadow size.
 */
void comm_tx_isr(comm self)
{
    size_t remain = 0;

    // succesful transfer of a packet
    if (self) {
        // Remaining bytes in the buffer before we need to reset the read index.
        remain = self->config.tx_capacity - self->tx_read_ptr;

        if (self->tx_shadow > remain && remain < self->config.tx_transfer_size) {
            // This means this is a short packet at the end of the buffer.
            self->tx_buffer_size -= remain;
            // self->read_index += remain;
            self->tx_read_ptr = 0;
            self->tx_shadow -= remain;
        } else if (self->tx_shadow <= self->config.tx_transfer_size) {
            // This is the last packet in the chain.
            self->tx_buffer_size -= self->tx_shadow;
            self->tx_read_ptr += self->tx_shadow;
            self->tx_read_ptr = self->tx_read_ptr == self->config.tx_capacity ? 0u : self->tx_read_ptr;
            self->tx_shadow = 0;
        } else {
            // This was a full transfer in the middle of a chain and not at a memory boundary
            self->tx_buffer_size -= self->config.tx_transfer_size;
            // I'm brave enough to not obsessively check if this will access bad memory. Trust logic.
            self->tx_read_ptr += self->config.tx_transfer_size;
            self->tx_shadow -= self->config.tx_transfer_size;
        }
        // Set/clear full flag.
        if (self->tx_buffer_size + self->config.tx_transfer_size >= self->config.tx_capacity) {
            self->tx_status |= COMM_TX_STS_FULL;
        } else {
            self->tx_status &= ~COMM_TX_STS_FULL;
        }
        // If this was the last packet in a chain
        if (self->tx_shadow == 0u) {
            // Do we have bytes waiting to send?
            if (self->tx_buffer_size > 0) {
                self->tx_status |= COMM_TX_STS_ACTIVE;
                config_tx_dma(self);
            } else {
                // Disable DMA?
                self->tx_status &= ~COMM_TX_STS_ACTIVE;
            }
        }
    }
}
