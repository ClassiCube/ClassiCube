/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#include "wifi_debug.h"

#include <nds.h>
#include <nds/interrupts.h>
#include <stdio.h>
#include <stdarg.h>

#include "dsiwifi_cmds.h"

char _print_buffer[0x7C] = {0};
extern int wifi_printf_allowed;

void wifi_printf(char* fmt, ...)
{
    if (!wifi_printf_allowed) return;
    
    int lock = enterCriticalSection();
    va_list args;
    va_start(args, fmt);
    vsnprintf(_print_buffer, 0x7C-1, fmt, args);
    va_end(args);

    wifi_ipcSendStringAlt(_print_buffer);
    leaveCriticalSection(lock);
}

void wifi_printlnf(char* fmt, ...)
{
    if (!wifi_printf_allowed) return;
    
    int lock = enterCriticalSection();
    va_list args;
    va_start(args, fmt);
    vsnprintf(_print_buffer, 0x7C-2, fmt, args);
    strcat(_print_buffer, "\n");
    va_end(args);

    wifi_ipcSendStringAlt(_print_buffer);
    leaveCriticalSection(lock);
}

void wifi_ipcSendStringAlt(char* ptr) {
    if (!wifi_printf_allowed) return;

    Wifi_FifoMsgExt msg;
    msg.cmd = WIFI_IPCINT_DBGLOG;
    for (int i = 0; i <= strlen(ptr); i += sizeof(msg.log_str)-1)
    {
        strncpy(msg.log_str, &ptr[i], sizeof(msg.log_str));
        msg.log_str[sizeof(msg.log_str) - 1] = 0;

        fifoSendDatamsg(FIFO_DSWIFI, sizeof(msg), (u8*)&msg);
    }
}
