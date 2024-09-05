/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_MBOX_H
#define _DSIWIFI_MBOX_H

#include "common.h"

#define MBOXPKT_HTC    (0)
#define MBOXPKT_WMI    (1)
#define MBOXPKT_DATA   (2)
#define MBOXPKT_DATA_4 (4)

#define MBOXPKT_NOACK  (0)
#define MBOXPKT_REQACK (1)
#define MBOXPKT_RETACK (2)

#endif // _DSIWIFI_MBOX_H
