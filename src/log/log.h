#ifndef LOG_H
#define LOG_H

#include "comm/comm.h"
#include <stdint.h>

void log_send(comm comm_ref, uint16_t buffer_size);

#endif
