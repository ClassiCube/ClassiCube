/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wifi_debug.h"

#include "dsiwifi9.h"

#include <nds.h>
#include <nds/interrupts.h>
#include <stdio.h>
#include <stdarg.h>

char __print_buffer[0x200] = {0};

void wifi_printf(char* fmt, ...)
{
    if (!DSiWifi_pfnLogHandler)
        return;

    int lock = enterCriticalSection();
    va_list args;
    va_start(args, fmt);
    vsnprintf(__print_buffer, 0x200-1, fmt, args);
    va_end(args);

    DSiWifi_pfnLogHandler(__print_buffer);
    
    leaveCriticalSection(lock);
}

void wifi_printlnf(char* fmt, ...)
{
    if (!DSiWifi_pfnLogHandler)
        return;

    int lock = enterCriticalSection();
    va_list args;
    va_start(args, fmt);
    vsnprintf(__print_buffer, 0x200-2, fmt, args);
    strcat(__print_buffer, "\n");
    va_end(args);

    DSiWifi_pfnLogHandler(__print_buffer);

    leaveCriticalSection(lock);
}
