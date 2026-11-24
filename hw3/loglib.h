#ifndef LOGLIB_H
#define LOGLIB_H

#include "proclib.h"
#include <stddef.h>

void log_now_ms(char* buf, size_t len);
void log_write(const char* path, proc_id_t pid, const char* msg);

#endif