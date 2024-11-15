#pragma once

#include <types.h>
#include <lib/printf.h>
#include <sys/ports.h>

void dprintf(const char* fmt, ...);
void serial_init();