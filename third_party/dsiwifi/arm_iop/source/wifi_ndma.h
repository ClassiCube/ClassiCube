/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_WIFI_NDMA_H
#define _DSIWIFI_WIFI_NDMA_H

#include <nds.h>

#ifdef ARM11
#define REG_NDMA_BASE             ((void*)0x10002000)
#else
#define REG_NDMA_BASE             ((void*)0x04004100) // DSi ARM7
#endif

// Regs
#define REG_NDMA_GLOBAL_CNT       (*(vu32*)(REG_NDMA_BASE + 0x0000))

#define REG_NDMA_SRC_ADDR(n)      (*(vu32*)(REG_NDMA_BASE + 0x0004 + ((n) * 0x1C)))
#define REG_NDMA_DST_ADDR(n)      (*(vu32*)(REG_NDMA_BASE + 0x0008 + ((n) * 0x1C)))
#define REG_NDMA_TRANSFER_CNT(n)  (*(vu32*)(REG_NDMA_BASE + 0x000C + ((n) * 0x1C)))
#define REG_NDMA_WRITE_CNT(n)     (*(vu32*)(REG_NDMA_BASE + 0x0010 + ((n) * 0x1C)))
#define REG_NDMA_BLOCK_CNT(n)     (*(vu32*)(REG_NDMA_BASE + 0x0014 + ((n) * 0x1C)))
#define REG_NDMA_FILL_DATA(n)     (*(vu32*)(REG_NDMA_BASE + 0x0018 + ((n) * 0x1C)))
#define REG_NDMA_CNT(n)           (*(vu32*)(REG_NDMA_BASE + 0x001C + ((n) * 0x1C)))

// NDMA_GLOBAL_CNT bits
#define NDMA_GLOBAL_ENABLE        (BIT(0))

#define NDMA_ROUND_ROBIN          (1 << 31)
#define NDMA_FIXED_METHOD         (0 << 31)

// NDMA_CNT bits
#define NDMA_ENABLE               (BIT(31))
#define NDMA_IRQ_ENABLE           (BIT(30))
#define NDMA_REPEATING_MODE       (BIT(29))
#define NDMA_IMMEDIATE_MODE       (BIT(28))

#define NDMA_STARTUP_MODE(n)      ((n & 0xF) << 24)
#define NDMA_BLOCK_SIZE(n)        ((n & 0xF) << 16)

#define NDMA_SRC_RELOAD           (BIT(15))
#define NDMA_SRC_MODE(n)          ((n & 3) << 13)

#define NDMA_DST_RELOAD           (BIT(12))
#define NDMA_DST_MODE(n)          ((n & 3) << 10)

// NDMA_STARTUP_MODE
#define NDMA_STARTUP_WIFI (9)

// SRC/DST_MODE
#define NDMA_MODE_INC   (0)
#define NDMA_MODE_DEC   (1)
#define NDMA_MODE_FIXED (2)
#define NDMA_MODE_FILL  (3)

// End NDMA_CNT

#define NDMA_MAX_CHANNELS (4)
#define WIFI_NDMA_CHAN (3)

//
// Functions
//
void wifi_ndma_init();
void wifi_ndma_read(void* dst, u32 len);
void wifi_ndma_write(void* src, u32 len);
void wifi_ndma_wait();

#endif // _DSIWIFI_WIFI_NDMA_H
