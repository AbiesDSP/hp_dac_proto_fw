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

#ifndef COBS_H
#define COBS_H

#include "comm/comm.h"
#include <stdint.h>
#include <stdlib.h>

#define COBS_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)            ((SRC_LEN) + (((SRC_LEN) + 253u)/254u))
#define COBS_DECODE_DST_BUF_LEN_MAX(SRC_LEN)            (((SRC_LEN) == 0) ? 0u : ((SRC_LEN) - (((SRC_LEN) + 253u)/254u)))

#define COBS_RUN_SIZE   (254u)

/* Returns error code.
 * Error if available buffer size is too small.
 */
uint8_t cobs_encode(comm self, const uint8_t *src, uint16_t src_len, uint16_t *copied);

/* Returns error code. Will start copying and encoding until it finds a delim.
 * Error if invalid encoding. Will still have copied/removed copied some bytes from rx_buf.
 * Ideally, call comm_rx_nextdelim, then use the location of the delimeter as the dst_size.
 * Error if it does not find delimiter within dst_size. It will still have
 *  copied and removed bytes from the receive buffer.
 * Assumes req_size >= dst_size
 */
uint8_t cobs_decode(comm self, uint8_t *dst, const uint16_t dst_size, uint16_t *copied);
// The size of the post-decode buffer. Since we don't allow implicit delimeters, overhead should be
// exactly one byte for every 254 bytes.
uint8_t cobs_decodesize(uint16_t src_len);

#endif
