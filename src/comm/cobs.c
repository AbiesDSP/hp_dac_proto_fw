/*
    ----------------------------------------------------------------------------
    Copyright (c) 2010 Craig McQueen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    ----------------------------------------------------------------------------
*/

#include "comm/comm_pvt.h"
#include "comm/cobs.h"
#include "CyDmac.h"
#include <stdlib.h>
#include <stdio.h>

/* COBS-encode a string of input bytes.
 *
 * Higher level functions will validate if there is available space in the buffer
 */
uint8_t cobs_encode(comm self, const uint8_t *src, uint16_t src_len, uint16_t *copied)
{
    uint16_t read_index = 0, buf_code_index, buffer_size, free_space, flush_amt;
    uint8_t src_byte = 0, code = 1, int_status, error = COMM_ERR_NONE;

    // Don't allow packets greater than 254 bytes.
    if (copied == NULL || src == NULL) {
        error = COMM_ERR_ALLOC;
        if (copied) {
            *copied = 0;
        }
        return error;
    }
    int_status = CyEnterCriticalSection();
    buffer_size = self->tx_buffer_size;
    CyExitCriticalSection(int_status);
    free_space = self->config.tx_capacity - buffer_size;

    // Not enough available space. Don't even try. 
    if (free_space < COBS_ENCODE_DST_BUF_LEN_MAX(src_len)) {
        error = COMM_ERR_TX_FULL;
        *copied = 0;
        return error;
    }

    // need extra code_index pointer. Initialize all pointers.
    buf_code_index = self->tx_write_ptr;
    self->tx_write_ptr++;
    self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
    *copied = 1;
    flush_amt = 1;

    while (read_index < src_len) {
        src_byte = src[read_index++];
        if (src_byte == COMM_DELIM) {
            self->config.tx_buffer[buf_code_index] = code;
            buf_code_index = self->tx_write_ptr;
            self->tx_write_ptr++;
            self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
            (*copied)++;
            flush_amt++;
            code = 1;
        } else {
            self->config.tx_buffer[self->tx_write_ptr] = src_byte;
            self->tx_write_ptr++;
            self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
            (*copied)++;
            flush_amt++;
            code++;
            // Update code. finished run.
            if (code == 0xFF) {
                self->config.tx_buffer[buf_code_index] = code;
                buf_code_index = self->tx_write_ptr;
                code = 1;
                /* If this was the last byte, we shouldn't udpate the write pointer.
                 * Otherwise, this means we need an extra byte of overhead for a new run.
                 */
                if (read_index < src_len) {
                    // The last 254 bytes can be flushed out.
                    int_status = CyEnterCriticalSection();
                    self->tx_buffer_size += flush_amt;
                    CyExitCriticalSection(int_status);
                    flush_amt = 0u;
                    self->tx_write_ptr++;
                    self->tx_write_ptr = self->tx_write_ptr == self->config.tx_capacity ? 0u : self->tx_write_ptr;
                    (*copied)++;
                    flush_amt++;
                }
            }
        }
    }
    
    self->config.tx_buffer[buf_code_index] = code;
    // Update buffer size.
    int_status = CyEnterCriticalSection();
    self->tx_buffer_size += flush_amt;
    buffer_size = self->tx_buffer_size;
    CyExitCriticalSection(int_status);

    // Buffer needs to be flushed
    if (buffer_size + self->config.tx_transfer_size >= self->config.tx_capacity) {
        self->tx_status |= COMM_TX_STS_FULL;
    }
    
    return error;
}

/* Decode a COBS byte string.
 * This assumes there IS a delimeter and will copy/decode until it's done.
 * Behaves similarly to receive_delim.
 * Copied size includes delimeter since we want to remove it from the buffer.
 */
uint8_t cobs_decode(comm self, uint8_t *dst, const uint16_t dst_size, uint16_t *copied) {

    uint16_t write_index = 0, buf_size, remain, dumped = 0;
    uint8_t src_byte, i, code, int_status, error = COMM_ERR_NONE, transfer_size = 0;

    if (dst == NULL || copied == NULL) {
        error = COMM_ERR_ALLOC;
        *copied = 0;
        return error;
    }
    int_status = CyEnterCriticalSection();
    buf_size = self->rx_buffer_size;
    CyExitCriticalSection(int_status);

    while (1) {
        // code = src[read_index++];
        code = self->config.rx_buffer[self->rx_read_ptr];
        self->rx_read_ptr++;
        self->rx_read_ptr = self->rx_read_ptr == self->config.rx_capacity ? 0u : self->rx_read_ptr;
        dumped++;
        buf_size--;

        // Invalid code location. This could imply an unexpected 0.
        if (code == COMM_DELIM) {
            // error.
            error = COMM_ERR_DELIM;
            break;
        }
        code--;

        remain = dst_size - write_index;
        // Not enough bytes remaining in dst buffer to decode. Invalid code.
        if (code > remain) {
            error = COMM_ERR_COBS;
            break;
        }
        // Not enough bytes remaining in rx_buffer to decode. Invalid code, or this was called too soon.
        if (code > buf_size) {
            error = COMM_ERR_COBS;
            break;
        }

        // Copy the next run of all non-zero bytes
        for (i = code; i != 0; i--) {
            // src_byte = src[read_index++];
            src_byte = self->config.rx_buffer[self->rx_read_ptr];
            self->rx_read_ptr++;
            self->rx_read_ptr = self->rx_read_ptr == self->config.rx_capacity ? 0u : self->rx_read_ptr;
            dumped++;
            buf_size--;
            // Received a delimeter when we shouldn't have.
            if (src_byte == COMM_DELIM) {
                error = COMM_ERR_DELIM;
                break;
            }
            dst[write_index++] = src_byte;
        }
        if (error) {
            // need to break out of the for and while loop.
            break;
        }
        // If the next byte is a delimeter, then we're done.
        src_byte = self->config.rx_buffer[self->rx_read_ptr];
        if (src_byte == COMM_DELIM) {
            // Remove the delimeter from the packet as well, but don't copy it.
            self->rx_read_ptr++;
            self->rx_read_ptr = self->rx_read_ptr == self->config.rx_capacity ? 0u : self->rx_read_ptr;
            dumped++;
            buf_size--;
            break;
        }

        // next zero.
        if (code != 0xFE) {
            if (write_index >= dst_size) {
                error = COMM_ERR_COBS;
                break;
            }
            dst[write_index++] = 0u;
        }
        // Free this run from the buffer.
        int_status = CyEnterCriticalSection();
        self->rx_buffer_size -= dumped;
        CyExitCriticalSection(int_status);
        dumped = 0;
    }

    int_status = CyEnterCriticalSection();
    self->rx_buffer_size -= dumped;
    remain = self->config.rx_capacity - self->rx_buffer_size;
    CyExitCriticalSection(int_status);
    *copied = write_index;
    // A 0 byte packet is an error.
    if (*copied == 0 && error == COMM_ERR_NONE) {
        error = COMM_ERR_COBS;
    }
    
    // We've cleared bytes, so if we're overflowed, we can stop.
    if (self->rx_status & COMM_RX_STS_OVERFLOW) {
        self->rx_status &= ~COMM_RX_STS_OVERFLOW;
        if (remain > self->config.rx_transfer_size) {
            transfer_size = self->config.rx_transfer_size;
        } else {
            transfer_size = remain;
        }
        // Clear the stall.
        self->rx_status &= ~COMM_RX_STS_OVERFLOW;
        self->config.spy_resume(transfer_size);
    }

    return error;
}
