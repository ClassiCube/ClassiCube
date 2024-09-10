/*
 * Licensed under the BSD license
 *
 * debug_32x.c - Debug screen functions.
 *
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * Altered for 32X by Chilly Willy
 */

#include "32x.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int init = 0;
static unsigned short fgc = 0, bgc = 0;
static unsigned char fgs = 0, bgs = 0;

static unsigned short currentFB = 0;

void Hw32xSetFGColor(int s, int r, int g, int b)
{
    volatile unsigned short *palette = &MARS_CRAM;
    fgs = s;
    fgc = COLOR(r, g, b);
    palette[fgs] = fgc;
}

void Hw32xSetBGColor(int s, int r, int g, int b)
{
    volatile unsigned short *palette = &MARS_CRAM;
    bgs = s;
    bgc = COLOR(r, g, b);
    palette[bgs] = bgc;
}

void Hw32xInit(int vmode, int lineskip)
{
    volatile unsigned short *frameBuffer16 = &MARS_FRAMEBUFFER;
    int i;

    // Wait for the SH2 to gain access to the VDP
    while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0) ;

    if (vmode == MARS_VDP_MODE_32K)
    {
        // Set 16-bit direct mode, 224 lines
        MARS_VDP_DISPMODE = MARS_224_LINES | MARS_VDP_MODE_32K;

        // init both framebuffers

        // Flip the framebuffer selection bit and wait for it to take effect
        MARS_VDP_FBCTL = currentFB ^ 1;
        while ((MARS_VDP_FBCTL & MARS_VDP_FS) == currentFB) ;
        currentFB ^= 1;
        // rewrite line table
        for (i=0; i<224/(lineskip+1); i++)
        {
            if (lineskip)
            {
                int j = lineskip + 1;
                while (j)
                {
                    frameBuffer16[i*(lineskip+1) + (lineskip + 1 - j)] = i*320 + 0x100; /* word offset of line */
                    j--;
                }
            }
            else
            {
                if (i<200)
                    frameBuffer16[i] = i*320 + 0x100; /* word offset of line */
                else
                    frameBuffer16[i] = 200*320 + 0x100; /* word offset of line */
            }
        }
        // clear screen
        for (i=0x100; i<0x10000; i++)
            frameBuffer16[i] = 0;

        // Flip the framebuffer selection bit and wait for it to take effect
        MARS_VDP_FBCTL = currentFB ^ 1;
        while ((MARS_VDP_FBCTL & MARS_VDP_FS) == currentFB) ;
        currentFB ^= 1;
        // rewrite line table
        for (i=0; i<224/(lineskip+1); i++)
        {
            if (lineskip)
            {
                int j = lineskip + 1;
                while (j)
                {
                    frameBuffer16[i*(lineskip+1) + (lineskip + 1 - j)] = i*320 + 0x100; /* word offset of line */
                    j--;
                }
            }
            else
            {
                if (i<200)
                    frameBuffer16[i] = i*320 + 0x100; /* word offset of line */
                else
                    frameBuffer16[i] = 200*320 + 0x100; /* word offset of line */
            }
        }
        // clear screen
        for (i=0x100; i<0x10000; i++)
            frameBuffer16[i] = 0;
    }

    Hw32xSetFGColor(255,31,31,31);
    Hw32xSetBGColor(0,0,0,0);
    init = vmode;
}

void Hw32xScreenClear()
{
    int i;
    int l = (init == MARS_VDP_MODE_256) ? 320*224/2 + 0x100 : 320*200 + 0x100;
    volatile unsigned short *frameBuffer16 = &MARS_FRAMEBUFFER;

    // clear screen
    for (i=0x100; i<l; i++)
        frameBuffer16[i] = 0;

    // Flip the framebuffer selection bit and wait for it to take effect
    MARS_VDP_FBCTL = currentFB ^ 1;
    while ((MARS_VDP_FBCTL & MARS_VDP_FS) == currentFB) ;
    currentFB ^= 1;

    // clear screen
    for (i=0x100; i<l; i++)
        frameBuffer16[i] = 0;

    Hw32xSetFGColor(255,31,31,31);
    Hw32xSetBGColor(0,0,0,0);
}

void Hw32xDelay(int ticks)
{
    unsigned long ct = MARS_SYS_COMM12 + ticks;
    while (MARS_SYS_COMM12 < ct) ;
}

void Hw32xScreenFlip(int wait)
{
    // Flip the framebuffer selection bit
    MARS_VDP_FBCTL = currentFB ^ 1;
    if (wait)
    {
        while ((MARS_VDP_FBCTL & MARS_VDP_FS) == currentFB) ;
        currentFB ^= 1;
    }
}

void Hw32xFlipWait()
{
    while ((MARS_VDP_FBCTL & MARS_VDP_FS) == currentFB) ;
    currentFB ^= 1;
}


// MD Command support code ---------------------------------------------

unsigned short HwMdReadPad(int port)
{
    if (port == 0)
        return MARS_SYS_COMM8;
    else if (port == 1)
        return MARS_SYS_COMM10;
    else
        return SEGA_CTRL_NONE;
}

unsigned char HwMdReadSram(unsigned short offset)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM2 = offset;
    MARS_SYS_COMM0 = 0x0100; // Read SRAM
    while (MARS_SYS_COMM0) ;
    return MARS_SYS_COMM2 & 0x00FF;
}

void HwMdWriteSram(unsigned char byte, unsigned short offset)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM2 = offset;
    MARS_SYS_COMM0 = 0x0200 | byte; // Write SRAM
    while (MARS_SYS_COMM0) ;
}

int HwMdReadMouse(int port)
{
    unsigned int mouse1, mouse2;
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM0 = 0x0300|port; // tells 68000 to read mouse
    while (MARS_SYS_COMM0 == (0x0300|port)) ; // wait for mouse value
    mouse1 = MARS_SYS_COMM0;
    mouse2 = MARS_SYS_COMM2;
    MARS_SYS_COMM0 = 0; // tells 68000 we got the mouse value
    return (int)((mouse1 << 16) | mouse2);
}

void HwMdClearScreen(void)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM0 = 0x0400; // Clear Screen (Name Table B)
    while (MARS_SYS_COMM0) ;
}

void HwMdSetOffset(unsigned short offset)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM2 = offset;
    MARS_SYS_COMM0 = 0x0500; // Set offset (into either Name Table B or VRAM)
    while (MARS_SYS_COMM0) ;
}

void HwMdSetNTable(unsigned short word)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM2 = word;
    MARS_SYS_COMM0 = 0x0600; // Set word at offset in Name Table B
    while (MARS_SYS_COMM0) ;
}

void HwMdSetVram(unsigned short word)
{
    while (MARS_SYS_COMM0) ; // wait until 68000 has responded to any earlier requests
    MARS_SYS_COMM2 = word;
    MARS_SYS_COMM0 = 0x0700; // Set word at offset in VRAM
    while (MARS_SYS_COMM0) ;
}

void HwMdPuts(char *str, int color, int x, int y)
{
    HwMdSetOffset(((y<<6) | x) << 1);
    while (*str)
        HwMdSetNTable(((*str++ - 0x20) & 0xFF) | color);
}

void HwMdPutc(char chr, int color, int x, int y)
{
    HwMdSetOffset(((y<<6) | x) << 1);
    HwMdSetNTable(((chr - 0x20) & 0xFF) | color);
}


// Slave SH2 support code ----------------------------------------------

void slave(void)
{
    while (1) ;
}
