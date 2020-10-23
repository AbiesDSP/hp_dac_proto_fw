#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_defs.h"
#include "CyDmac.h"

#define `$INSTANCE_NAME`_CTL_STALL  (0x01u)
#define `$INSTANCE_NAME`_CTL_EN     (0x02u)
 
const reg8 *`$INSTANCE_NAME`_fifo_in_ptr = `$INSTANCE_NAME`_spy_F0_PTR;
const reg8 *`$INSTANCE_NAME`_fifo_out_ptr = `$INSTANCE_NAME`_spy_F1_PTR;
const reg8 *`$INSTANCE_NAME`_last_byte_ptr = `$INSTANCE_NAME`_spy_A0_PTR;
const reg8 *`$INSTANCE_NAME`_tx_size_ptr = `$INSTANCE_NAME`_spy_A1_PTR;
const reg8 *`$INSTANCE_NAME`_delim_ptr = `$INSTANCE_NAME`_spy_D0_PTR;
const reg8 *`$INSTANCE_NAME`_max_tx_size_ptr = `$INSTANCE_NAME`_spy_D1_PTR;

static uint8_t last_xfer_size = 0x00u;
static uint8_t started = 0x00u;
static void `$INSTANCE_NAME`_enable(void);
static void `$INSTANCE_NAME`_disable(void);

void `$INSTANCE_NAME`_start(uint8_t delimeter, uint8_t first_xfer_size)
{
    // Always stall and enable after setting the transfer size register (D1)
    CY_SET_REG8(`$INSTANCE_NAME`_max_tx_size_ptr, first_xfer_size);
    CY_SET_REG8(`$INSTANCE_NAME`_delim_ptr, delimeter);
    last_xfer_size = first_xfer_size;
    `$INSTANCE_NAME`_ExtTimer_Start();
    `$INSTANCE_NAME`_enable();
}

void `$INSTANCE_NAME`_stop(void)
{
    `$INSTANCE_NAME`_disable();
    `$INSTANCE_NAME`_ExtTimer_Stop();
}

static void `$INSTANCE_NAME`_enable(void)
{
    uint8_t control;
    
    control = `$INSTANCE_NAME`_ctl_Read() | `$INSTANCE_NAME`_CTL_EN;
    `$INSTANCE_NAME`_ctl_Write(control);
    started = `$INSTANCE_NAME`_CTL_EN;
}

static void `$INSTANCE_NAME`_disable(void)
{
    uint8_t control;
    
    control = `$INSTANCE_NAME`_ctl_Read() & ~`$INSTANCE_NAME`_CTL_EN;
    `$INSTANCE_NAME`_ctl_Write(control);
    started = 0u;
}

uint8_t `$INSTANCE_NAME`_service(uint8_t *last_byte)
{
    uint8_t transfer_size;
    /* The transfer size is how many bytes were actually transferred
     * and not how many were requested. If there was a timeout or we
     * received a delimeter, we will have fewer bytes. This is
     * expected.
     */
    *last_byte = CY_GET_REG8(`$INSTANCE_NAME`_last_byte_ptr);
    transfer_size = last_xfer_size - CY_GET_REG8(`$INSTANCE_NAME`_tx_size_ptr);

    return transfer_size;
}

void `$INSTANCE_NAME`_resume(uint8_t next_xfer_size)
{
    /* Update D1. If you want to update your next transfer size
     * based on the size of the last transfer, call service in the
     * isr first, then call resume.
     */
    if (next_xfer_size != last_xfer_size) {
        CY_SET_REG8(`$INSTANCE_NAME`_max_tx_size_ptr, next_xfer_size);
        last_xfer_size = next_xfer_size;
    }
    // Clear the stall.
    `$INSTANCE_NAME`_ctl_Write(`$INSTANCE_NAME`_CTL_STALL | started);
}
