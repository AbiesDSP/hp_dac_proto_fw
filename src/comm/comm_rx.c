#include "comm/comm.h"
#include "comm/comm_pvt.h"
#include "CyDmac.h"

uint16_t comm_rx_buffer_size(comm self)
{
    uint16_t size = 0;
    uint8_t int_status = 0;
    
    if (self) {
        int_status = CyEnterCriticalSection();
        size = self->rx_buffer_size;
        CyExitCriticalSection(int_status);
    }
    
    return size;
}

uint8_t comm_rx_status(comm self)
{
    uint8_t status = 0xFFu, int_status;
    uint16_t buf_size;
    
    if (self) {
        int_status = CyEnterCriticalSection();
        status = self->rx_status;
        buf_size = self->rx_buffer_size;
        CyExitCriticalSection(int_status);
        
        if (self->delimeter_count == 0u) {
            status &= ~COMM_RX_STS_NEW_PACKET;
        } else {
            status |= COMM_RX_STS_NEW_PACKET;
        }
        if (buf_size == self->config.rx_capacity) {
            status |= COMM_RX_STS_OVERFLOW;
        }
        if ((self->config.rx_capacity - buf_size) <= self->config.rx_transfer_size) {
            status |= COMM_RX_STS_FULL;
        } else {
            status &= ~COMM_RX_STS_FULL;
        }
        int_status = CyEnterCriticalSection();
        self->rx_status = status;
        CyExitCriticalSection(int_status);
    }
    
    return status;
}

uint8_t comm_rx_clear(comm self, uint16_t amount)
{
    uint8_t error = COMM_ERR_NONE, int_status, transfer_size = 0;
    uint16_t buf_size, remain, location, n_delim;

    if (self) {
        if (amount == 0u) {
            // Not an error, techinically.
            return error;
        }
        int_status = CyEnterCriticalSection();
        buf_size = self->rx_buffer_size;
        CyExitCriticalSection(int_status);
        remain = self->config.rx_capacity - self->rx_read_ptr;
        // Can't clear more than what's in the buffer.
        amount = amount > buf_size ? buf_size : amount;
        if (remain < amount) {
            self->rx_read_ptr = amount - remain;
        } else {
            self->rx_read_ptr += amount;
            self->rx_read_ptr = self->rx_read_ptr == self->config.rx_capacity ? 0u : self->rx_read_ptr;
        }
        int_status = CyEnterCriticalSection();
        self->rx_buffer_size -= amount;
        buf_size = self->rx_buffer_size;
        CyExitCriticalSection(int_status);

        // We've cleared bytes. Open up.
        if (self->rx_status & COMM_RX_STS_OVERFLOW) {
            remain = self->config.rx_capacity - buf_size;
            if (remain > self->config.rx_transfer_size) {
                transfer_size = self->config.rx_transfer_size;
            } else {
                transfer_size = remain;
            }
            // Clear the stall.
            self->rx_status &= ~COMM_RX_STS_OVERFLOW;
            self->config.spy_resume(transfer_size);
        }
        // Check if we've cleared delimeters.
        location = delim_location(self, DELIM_NOT_FOUND, &n_delim);
        if (location == DELIM_NOT_FOUND) {
            self->delimeter_count = 0u;
            self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
        } else {
            self->delimeter_count = n_delim;
            self->rx_status |= COMM_RX_STS_NEW_PACKET;
        }
    } else {
        error = COMM_ERR_ALLOC;
    }

    return error;
}

uint8_t comm_rx_next_pkt_size(comm self, uint16_t *size)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t location, n_delim = 0;

    if (self && size) {
        location = delim_location(self, DELIM_NOT_FOUND, &n_delim);
        if (location == DELIM_NOT_FOUND) {
            // Let's just clear these obsessively.
            self->delimeter_count = 0u;
            self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
            error = COMM_ERR_DELIM;
            *size = 0;
        } else {
            *size = location + 1u;
            self->delimeter_count = n_delim;
            self->rx_status |= COMM_RX_STS_NEW_PACKET;
        }
    } else {
        error = COMM_ERR_ALLOC;
    }

    return error;
}


/* Receive up the specified amount. This function will
 * block until the specified bytes arrive. If you do not want
 * this function to block, call rx_buffer_size before this
 * function and use that as the amount parameter.
 * This function assumes amount < buf_size. That's on you.
 */
uint8_t comm_receive(comm self, uint8_t *buf, uint16_t amount)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t buffer_size, buffer_index = 0;
    uint8_t transfer_size = 0;

    if (self && self->enabled) {
        if (buf == NULL || amount == 0) {
            error = COMM_ERR_PARAM;
        }
        // Copy the contents
        if (error == COMM_ERR_NONE) {
            while (amount > 0) {
                /* If the buffer is empty, block until it has content.
                 * If you want non-blocking, use receive_all or check the
                 * status for received delimeters.
                 */
                buffer_size = comm_rx_buffer_size(self);
                // Transfer in chunks.
                if (amount > self->config.rx_transfer_size && buffer_size >= self->config.rx_transfer_size) {
                    transfer_size = self->config.rx_transfer_size;
                } else if (amount <= buffer_size) {
                    // Finished reading out bytes
                    transfer_size = amount;
                } else {
                    // Need to wait for more bytes
                    transfer_size = buffer_size;
                }
                read_rx_buf(self, &buf[buffer_index], transfer_size);
                buffer_index += transfer_size;
                amount -= transfer_size;
            }
        }
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }
    
    return error;
}

/* This function will transfer all bytes in the rx_buffer,
 * then will write the amount copied into *amount.
 * If buf_size < amount, this will return an incomplete error,
 * and will only copy buf_size bytes. You may need to call this
 * a few times if buf is smaller than the rx_buffer size.
 */
uint8_t comm_receive_all(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t remain, buffer_index = 0;
    uint8_t int_status, transfer_size;
    
    if (self && self->enabled) {
        if (buf == NULL || amount == 0) {
            error = COMM_ERR_PARAM;
        }
        // Copy the contents
        if (error == COMM_ERR_NONE) {
            int_status = CyEnterCriticalSection();
            remain = self->rx_buffer_size;
            CyExitCriticalSection(int_status);
            
            // Not enough room to copy entire available buffer.
            if (remain > buf_size) {
                error = COMM_ERR_INCOMPLETE;
                remain = buf_size;
            }
            
            *amount = remain;
            while (remain > 0) {
                // Transfer in chunks.
                if (remain > self->config.rx_transfer_size) {
                    transfer_size = self->config.rx_transfer_size;
                } else {
                    transfer_size = remain;
                }
                read_rx_buf(self, &buf[buffer_index], transfer_size);
                buffer_index += transfer_size;
                remain -= transfer_size;
            }
            // Clear new packet flags.
            self->delimeter_count = 0u;
            self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
        }
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }
    
    return error;
}

/* rx_status COMM_RX_STS_NEW_PACKET must be set before calling this function.
 * It will return an error if there is no delimeter.
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
uint8_t comm_receive_delim(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t remain, buffer_index = 0, location, n_delim = 0;
    uint8_t int_status, transfer_size;
    
    if (self && self->enabled) {
        if (buf == NULL || amount == 0) {
            error = COMM_ERR_PARAM;
        }
        // Have not received delimeters.
        if (self->delimeter_count == 0) {
            *amount = 0;
            error = COMM_ERR_NODATA;
        }
        // Copy the contents
        if (error == COMM_ERR_NONE) {
            int_status = CyEnterCriticalSection();
            remain = self->rx_buffer_size;
            CyExitCriticalSection(int_status);
            error = COMM_ERR_DELIM;

            while (remain > 0) {
                // Transfer in chunks.
                if (remain > self->config.rx_transfer_size) {
                    transfer_size = self->config.rx_transfer_size;
                } else {
                    transfer_size = remain;
                }
                location = delim_location(self, transfer_size, &n_delim);
                if (location == DELIM_NOT_FOUND) {
                    if (buffer_index + transfer_size > buf_size) {
                        error = COMM_ERR_INCOMPLETE;
                        transfer_size = buf_size - buffer_index;
                        remain = 0; // We're done here.
                    }
                    read_rx_buf(self, &buf[buffer_index], transfer_size);
                    buffer_index += transfer_size;
                    remain -= transfer_size;
                } else {
                    transfer_size = location + 1u;
                    // Not enough room.
                    if ((location + 1u) > (buf_size - buffer_index)) {
                        transfer_size = buf_size - buffer_index;
                        error = COMM_ERR_INCOMPLETE;
                    }
                    // Only copy up through the delim
                    read_rx_buf(self, &buf[buffer_index], transfer_size);
                    buffer_index += transfer_size;
                    remain -= transfer_size;
                    self->delimeter_count = self->delimeter_count > 0 ? self->delimeter_count - 1 : 0u;
                    if (self->delimeter_count == 0u) {
                        self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
                    }
                    error = COMM_ERR_NONE;
                    // We can break early.
                    break;
                }
            }
            // There were more bytes in the buffer than buf_size.
            if (error == COMM_ERR_INCOMPLETE) {
                *amount = buf_size;
            } else {
                *amount = buffer_index;
            }
            /* If somehow delimiter_count is set erroneously, this ends up
             * means no delimeter was found. What to do? Clear buffer? */
            if (error == COMM_ERR_DELIM) {
                self->delimeter_count = 0u;
                self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
            }
        }
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }
    
    return error;
}

uint8_t comm_rx_decode(comm self, uint8_t *buf, const uint16_t buf_size, uint16_t *amount)
{
    uint8_t error = COMM_ERR_NONE;
    uint16_t remain, location, copied = 0, search_size = 0, n_delim = 0;
    uint8_t int_status;
    
    if (self && self->enabled) {
        if (buf == NULL || amount == NULL || buf_size == 0u) {
            error = COMM_ERR_PARAM;
        }
        int_status = CyEnterCriticalSection();
        remain = self->rx_buffer_size;
        CyExitCriticalSection(int_status);

        // Have not received delimeters.
        if (self->delimeter_count == 0) {
            error = COMM_ERR_NODATA;
        }
        search_size = remain <= buf_size ? remain : buf_size;
        location = delim_location(self, search_size, &n_delim);
        // Requested size not available.
        if (remain < location || buf_size < location || n_delim == 0u) {
            error = COMM_ERR_DELIM;
        }
        // Copy the contents
        if (error == COMM_ERR_NONE) {
            error = cobs_decode(self, buf, buf_size, &copied);
            if (error == COMM_ERR_NONE) {
                self->delimeter_count--;
                if (self->delimeter_count == 0u) {
                    self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
                }
                // If you sent a single byte.
            } else {
                // Found a delim we weren't expecting
                if (error == COMM_ERR_DELIM) {
                    self->delimeter_count = self->delimeter_count != 0u ? self->delimeter_count - 1u : 0u;
                    if (self->delimeter_count == 0u) {
                        self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
                    }
                } else {
                    /* Depending on how wrong things were, we have an unknown
                     * number of bytes to clear. Clear through the next delimeter
                     * unless we already popped the delimeter when decoding.
                     */
                    location = delim_location(self, DELIM_NOT_FOUND, &n_delim);
                    if (location == DELIM_NOT_FOUND) {
                        // Just clear the whole thing.
                        comm_rx_clear(self, search_size);
                        self->delimeter_count = 0u;
                        self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
                    } else {
                        comm_rx_clear(self, location + 1u);
                        self->delimeter_count = self->delimeter_count != 0u ? self->delimeter_count - 1u : 0u;
                        if (self->delimeter_count == 0u) {
                            self->rx_status &= ~COMM_RX_STS_NEW_PACKET;
                        }
                    }
                }
            }
        }
        // Copied will either be 0 or accurate.
        *amount = copied;
    } else {
        error = self ? COMM_ERR_ALLOC : COMM_ERR_DISABLED;
    }
    
    return error;
}

void read_rx_buf(comm self, uint8_t *dst, uint8_t transfer_size)
{
    uint16_t remain = 0;
    uint8_t int_status;
    
    // Transfer across memory boundary
    if (self->rx_read_ptr + transfer_size > self->config.rx_capacity) {
        remain = self->config.rx_capacity - self->rx_read_ptr;
        memcpy(dst, &self->config.rx_buffer[self->rx_read_ptr], remain);
        memcpy(&dst[remain], &self->config.rx_buffer[0], transfer_size - remain);
        self->rx_read_ptr = transfer_size - remain;
        int_status = CyEnterCriticalSection();
        self->rx_buffer_size -= transfer_size;
        CyExitCriticalSection(int_status);
    } else {
        memcpy(dst, &self->config.rx_buffer[self->rx_read_ptr], transfer_size);
        self->rx_read_ptr += transfer_size;
        self->rx_read_ptr = self->rx_read_ptr == self->config.rx_capacity ? 0u : self->rx_read_ptr;
        int_status = CyEnterCriticalSection();
        self->rx_buffer_size -= transfer_size;
        CyExitCriticalSection(int_status);
    }
    
    // We've cleared bytes. Open up.
    if (self->rx_status & COMM_RX_STS_OVERFLOW) {
        int_status = CyEnterCriticalSection();
        remain = self->config.rx_capacity - self->rx_buffer_size;
        CyExitCriticalSection(int_status);
        if (remain > self->config.rx_transfer_size) {
            transfer_size = self->config.rx_transfer_size;
        } else {
            transfer_size = remain;
        }
        // Clear the stall.
        self->rx_status &= ~COMM_RX_STS_OVERFLOW;
        self->config.spy_resume(transfer_size);
    }
}
// Look for location for first delimeter.
uint16_t delim_location(comm self, uint16_t search_size, uint16_t *n_delim)
{
    uint16_t i, temp_read_address, buf_size, location = DELIM_NOT_FOUND;
    uint8_t int_status;
    
    int_status = CyEnterCriticalSection();
    buf_size = self->rx_buffer_size;
    CyExitCriticalSection(int_status);
    // Can't search beyond the buffer size.
    search_size = search_size > buf_size ? buf_size : search_size;

    *n_delim = 0;
    temp_read_address = self->rx_read_ptr;
    for (i = 0; i < search_size; i++) {
        if (self->config.rx_buffer[temp_read_address] == COMM_DELIM) {
            // Delimeter match! index relative to starting point
            if (*n_delim == 0) {
                location = i;
            }
            (*n_delim)++;
        }
        temp_read_address++;
        temp_read_address = temp_read_address == self->config.rx_capacity ? 0u : temp_read_address;
    }
    
    // 0xFFFF means not found in this chunk.
    return location;
}

void config_rx_dma(comm self)
{
    uint8_t i = 0, next_td;
    uint16_t remain, transfer_size;
    // UART -> Spy transfer. Just transfer 1 byte at a time forever.
    self->uart_rx_td[0] = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(self->uart_rx_td[0], 0u, self->uart_rx_td[0], 0u);
    CyDmaTdSetAddress(self->uart_rx_td[0], LO16((uint32_t)self->config.uart_rx_fifo), LO16((uint32_t)self->config.spy_fifo_in));
    CyDmaChSetInitialTd(self->config.uart_rx_ch, self->uart_rx_td[0]);
    /* Spy -> SRAM transfer. A looping transfer as large as the buffer.
     * chain together as many tds as it takes.
     */
    remain = self->config.rx_capacity;
    while (remain > 0) {
        if (remain > COMM_MAX_TRANSFER_SIZE) {
            transfer_size = COMM_MAX_TRANSFER_SIZE;
            next_td = self->spy_td[i + 1];
        } else {
            transfer_size = remain;
            next_td = self->spy_td[0];
        }
        CyDmaTdSetConfiguration(self->spy_td[i], transfer_size, next_td, TD_INC_DST_ADR);
        CyDmaTdSetAddress(self->spy_td[i], LO16((uint32_t)self->config.spy_fifo_out), LO16((uint32_t)&self->config.rx_buffer[self->config.rx_capacity - remain]));
        remain -= transfer_size;
    }
    CyDmaChSetInitialTd(self->config.spy_ch, self->spy_td[0]);
    CyDmaChEnable(self->config.uart_rx_ch, 1u);
    CyDmaChEnable(self->config.spy_ch, 1u);
}

void comm_rx_isr(comm self)
{
    uint8_t last_byte, transfer_size;
    uint16_t remain;
    
    if (self) {
        /* If our buffer is almost full, we need to adjust
         * our transfer size. If we are full, don't resume
         * right away. Set the overflow flag, and resume 
         * when the buffer is cleared.
         */
        transfer_size = self->config.spy_service(&last_byte);
        self->rx_buffer_size += transfer_size;
        remain = self->config.rx_capacity - self->rx_buffer_size;
        /* Regardless of what happened last isr, if we have space,
         * we can resume and clear flags.
         */
        if (remain > self->config.rx_transfer_size) {
            // Not full. Resume and clear any overflow flags
            self->rx_status &= ~(COMM_RX_STS_FULL | COMM_RX_STS_OVERFLOW);
            self->config.spy_resume(self->config.rx_transfer_size);
        } else if (remain > 0) {
            // We're full
            self->rx_status |= COMM_RX_STS_FULL;
            self->rx_status &= ~COMM_RX_STS_OVERFLOW;
            self->config.spy_resume(remain);
        } else {
            /* Overflow! We can't resume yet. Resume when we read
             * out the buffer. We haven't lost anything yet, but
             * eventually the UART RX FIFO will overflow and lose
             * data.
             */
            self->rx_status |= COMM_RX_STS_FULL;
            self->rx_status |= COMM_RX_STS_OVERFLOW;
        }
        
        // memory boundary.
        if (self->rx_write_ptr + transfer_size > self->config.rx_capacity) {
            remain = self->config.rx_capacity - self->rx_write_ptr;
            self->rx_write_ptr = transfer_size - remain;
        } else {
            self->rx_write_ptr += transfer_size;
            self->rx_write_ptr = self->rx_write_ptr == self->config.rx_capacity ? 0u : self->rx_write_ptr;
        }

        // Received a delimiter.
        if (last_byte == COMM_DELIM) {
            self->delimeter_count++;
            self->rx_status |= COMM_RX_STS_NEW_PACKET;
        }
    }
}
