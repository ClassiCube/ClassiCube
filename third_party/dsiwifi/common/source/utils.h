/*
 * Copyright (c) 2021 Max Thomas
 * This file is part of DSiWifi and is distributed under the MIT license.
 * See dsiwifi_license.txt for terms of use.
 */

#ifndef _DSIWIFI_UTILS_H
#define _DSIWIFI_UTILS_H

#include "common.h"

#define round_up(x, n) ( ((x) + (n)-1)  & (~((n)-1)) ) 

static inline void ioDelay(u32 count)
{
  int i;
  for (i = 0; i < count; i++)
      __asm__ volatile ("");
}

static inline u32 getle32(const void *p)
{
    const u8 *cp = p;

    return (u32)cp[0] + ((u32)cp[1] << 8) +
        ((u32)cp[2] << 16) + ((u32)cp[3] << 24);
}

static inline u64 getle64(const u8* p)
{
    u64 n = p[0];

    n |= (u64)p[1] << 8;
    n |= (u64)p[2] << 16;
    n |= (u64)p[3] << 24;
    n |= (u64)p[4] << 32;
    n |= (u64)p[5] << 40;
    n |= (u64)p[6] << 48;
    n |= (u64)p[7] << 56;
    return n;
}

static inline u64 getbe64(const u8* p)
{
    u64 n = 0;

    n |= (u64)p[0] << 56;
    n |= (u64)p[1] << 48;
    n |= (u64)p[2] << 40;
    n |= (u64)p[3] << 32;
    n |= (u64)p[4] << 24;
    n |= (u64)p[5] << 16;
    n |= (u64)p[6] << 8;
    n |= (u64)p[7] << 0;
    return n;
}

static inline u32 getbe32(const u8* p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
}

static inline u32 getle16(const u8* p)
{
    return (p[0] << 0) | (p[1] << 8);
}

static inline u32 getbe16(const u8* p)
{
    return (p[0] << 8) | (p[1] << 0);
}

static inline void putle16(u8* p, u16 n)
{
    p[0] = n;
    p[1] = n >> 8;
}

static inline void putbe16(u8* p, u16 n)
{
    p[0] = n >> 8;
    p[1] = n;
}

static inline void putle32(u8* p, u32 n)
{
    p[0] = n;
    p[1] = n >> 8;
    p[2] = n >> 16;
    p[3] = n >> 24;
}

static inline void putbe64(u8* p, u64 n)
{
    p[0] = n >> 56;
    p[1] = n >> 48;
    p[2] = n >> 40;
    p[3] = n >> 32;
    p[4] = n >> 24;
    p[5] = n >> 16;
    p[6] = n >> 8;
    p[7] = n >> 0;
}

void hexdump(const void* data, size_t size);

#endif // _DSIWIFI_UTILS_H
