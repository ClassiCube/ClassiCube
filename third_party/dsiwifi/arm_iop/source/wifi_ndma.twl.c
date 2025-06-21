/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */
 
#include "wifi_ndma.h"

#include "wifi_card.h"
#include "wifi_sdio.h"

void wifi_ndma_init()
{
    REG_NDMA_CNT(WIFI_NDMA_CHAN) = 0;

    REG_NDMA_GLOBAL_CNT = NDMA_GLOBAL_ENABLE | NDMA_FIXED_METHOD;
}

void wifi_ndma_read(void* dst, u32 len)
{
    REG_NDMA_SRC_ADDR(WIFI_NDMA_CHAN) = (u32)(REG_SDIO_BASE + WIFI_SDIO_OFFS_DATA32_FIFO);
    REG_NDMA_DST_ADDR(WIFI_NDMA_CHAN) = (u32)(dst);
    REG_NDMA_TRANSFER_CNT(WIFI_NDMA_CHAN) = len / 4;
    REG_NDMA_WRITE_CNT(WIFI_NDMA_CHAN) = 0x80 / 4; // logical block size, set to SDIO block len
    
    // Physical block size is also set to 0x80 bytes, there's probably a better value
    // depending on what memory we're transferring to
    REG_NDMA_CNT(WIFI_NDMA_CHAN) = NDMA_ENABLE | NDMA_BLOCK_SIZE(0x80 / 4) | NDMA_STARTUP_MODE(NDMA_STARTUP_WIFI) | NDMA_SRC_MODE(NDMA_MODE_FIXED) | NDMA_DST_MODE(NDMA_MODE_INC);
}

void wifi_ndma_write(void* src, u32 len)
{
    while (REG_NDMA_CNT(WIFI_NDMA_CHAN) & NDMA_ENABLE);

    REG_NDMA_SRC_ADDR(WIFI_NDMA_CHAN) = (u32)(src);
    REG_NDMA_DST_ADDR(WIFI_NDMA_CHAN) = (u32)(REG_SDIO_BASE + WIFI_SDIO_OFFS_DATA32_FIFO);
    REG_NDMA_TRANSFER_CNT(WIFI_NDMA_CHAN) = len / 4;
    REG_NDMA_WRITE_CNT(WIFI_NDMA_CHAN) = 0x80 / 4; // logical block size, set to SDIO block len
    
    // Physical block size is also set to 0x80 bytes, there's probably a better value
    // depending on what memory we're transferring to
    REG_NDMA_CNT(WIFI_NDMA_CHAN) = NDMA_ENABLE | NDMA_BLOCK_SIZE(0x80 / 4) | NDMA_STARTUP_MODE(NDMA_STARTUP_WIFI) | NDMA_SRC_MODE(NDMA_MODE_INC) | NDMA_DST_MODE(NDMA_MODE_FIXED);
}

void wifi_ndma_wait()
{
    while (REG_NDMA_CNT(WIFI_NDMA_CHAN) & NDMA_ENABLE);
}

