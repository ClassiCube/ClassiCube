/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_HOST_H
#define _DSIWIFI_HOST_H

#include <nds.h>
#include "dsiwifi9.h"

int wifi_host_get_status(void);
u32 wifi_host_get_ipaddr(void);

void wifi_host_init();

#endif // _DSIWIFI_HOST_H
