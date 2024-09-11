/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_CMDS_H
#define _DSIWIFI_CMDS_H

// ARM9->ARM7
#define WIFI_IPCCMD_NOP             (0)
#define WIFI_IPCCMD_INIT_IOP        (1)
#define WIFI_IPCCMD_INITBUFS        (2)
#define WIFI_IPCCMD_SENDPKT         (3)
#define WIFI_IPCCMD_GET_DEVICE_MAC  (4)
#define WIFI_IPCCMD_GET_AP_MAC      (5)

// ARM7->ARM9
#define WIFI_IPCINT_NOP         (0)
#define WIFI_IPCINT_READY       (1)
#define WIFI_IPCINT_CONNECT     (2)
#define WIFI_IPCINT_DEVICE_MAC  (3)
#define WIFI_IPCINT_AP_MAC      (4)
#define WIFI_IPCINT_PKTDATA     (5)
#define WIFI_IPCINT_DBGLOG      (6)
#define WIFI_IPCINT_PKTSENT     (7)

typedef struct Wifi_FifoMsg
{
    u32 cmd;
    union
    {
        struct
        {
            u8 mac_addr[6];
            u8 pad[2];
        };
        
        struct
        {
            void* pkt_data;
            u32 pkt_len;
        };
    };
} Wifi_FifoMsg;

// Includes bytes for logging
typedef struct Wifi_FifoMsgExt
{
    u32 cmd;
    union
    {
        struct
        {
            u8 mac_addr[6];
            u8 pad[2];
        };
        
        struct
        {
            void* pkt_data;
            u32 pkt_len;
        };
        
        struct
        {
            char log_str[64-4];
        };
    };
} Wifi_FifoMsgExt;

#endif // _DSIWIFI_CMDS_H
