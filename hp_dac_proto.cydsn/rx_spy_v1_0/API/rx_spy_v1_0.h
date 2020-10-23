#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H

#include <stdint.h>
#include <cytypes.h>

extern const reg8 *`$INSTANCE_NAME`_fifo_in_ptr;
extern const reg8 *`$INSTANCE_NAME`_fifo_out_ptr;

/* Initialize the DMA channels before starting this component
 * You will also need to start the appropriate interrupts
 * and start the UART.
 */
void `$INSTANCE_NAME`_start(uint8_t delimeter, uint8_t first_xfer_size);
void `$INSTANCE_NAME`_stop(void);

/* Returns the transfer size. If the buffer was flushed (common),
 * then the last byte will be 254u for...reasons. So you
 * should only check if this is 0u or not.
 */
uint8_t `$INSTANCE_NAME`_service(uint8_t *last_byte);
void `$INSTANCE_NAME`_resume(uint8_t next_xfer_size);

#endif
