/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "`$INSTANCE_NAME`.h"
#include "`$INSTANCE_NAME`_defs.h"

void `$INSTANCE_NAME`_start(void)
{
    
}

void `$INSTANCE_NAME`_stop(void)
{
    
}

// Read the last counter value. Cleared on read so don't lose it!
uint16_t `$INSTANCE_NAME`_read(void)
{
    // +1 is to compensate for the extra 'sync' state.
    return CY_GET_REG16(`$INSTANCE_NAME`_DP_F0_PTR) + 1;
}
    
/* [] END OF FILE */
