/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_HTC_H
#define _DSIWIFI_HTC_H

#include "common.h"

#define HTC_MSG_READY             (1)
#define HTC_MSG_CONN_SVC          (2)
#define HTC_MSG_CONN_SVC_RESP     (3)
#define HTC_MSG_SETUP_COMPLETE    (4)
#define HTC_MSG_SETUP_COMPLETE_EX (5)

#define HTC_MSG_UNK_0201 (0x0201)
#define HTC_MSG_UNK_0401 (0x0401)

void htc_handle_pkt(u16 pkt_cmd, u8* pkt_data, u32 len, u32 ack_len);
void htc_send_pkt(u16 htc_type, u8 ack_type, const void* data, u16 len);

#endif // _DSIWIFI_HTC_H
