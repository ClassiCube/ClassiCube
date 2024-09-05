/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_WIFI_DEBUG_H
#define _DSIWIFI_WIFI_DEBUG_H

#include <stdio.h>
#include <string.h>

extern char _print_buffer[0x7C];

void wifi_printf(char* fmt, ...);
void wifi_printlnf(char* fmt, ...);

void wifi_ipcSendString(char* ptr);
void wifi_ipcSendStringAlt(char* ptr);

//#define wifi_printf(...) {snprintf(_print_buffer, 0x100-1, __VA_ARGS__); wifi_ipcSendString(_print_buffer);}
//#define wifi_printlnf(...) {snprintf(_print_buffer, 0x100-1, __VA_ARGS__); wifi_ipcSendString(_print_buffer); wifi_ipcSendString("\n");}

#endif // _DSIWIFI_WIFI_DEBUG_H
