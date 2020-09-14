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

#ifndef `$INSTANCE_NAME`_H
#define `$INSTANCE_NAME`_H
    
#include <stdint.h>

void `$INSTANCE_NAME`_start(void);
void `$INSTANCE_NAME`_stop(void);
// Read the last counter value. Cleared on read so don't lose it!
uint16_t `$INSTANCE_NAME`_read(void);

#endif
    
/* [] END OF FILE */
