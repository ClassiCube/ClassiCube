#ifndef HW_32X_H
#define HW_32X_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void Hw32xSetFGColor(int s, int r, int g, int b);
extern void Hw32xSetBGColor(int s, int r, int g, int b);
extern void Hw32xInit(int vmode, int lineskip);
extern int Hw32xScreenGetX();
extern int Hw32xScreenGetY();
extern void Hw32xScreenSetXY(int x, int y);
extern void Hw32xScreenClear();
extern void Hw32xDelay(int ticks);
extern void Hw32xScreenFlip(int wait);
extern void Hw32xFlipWait();

extern unsigned short HwMdReadPad(int port);
extern unsigned char HwMdReadSram(unsigned short offset);
extern void HwMdWriteSram(unsigned char byte, unsigned short offset);
extern int HwMdReadMouse(int port);
extern void HwMdClearScreen(void);
extern void HwMdSetOffset(unsigned short offset);
extern void HwMdSetNTable(unsigned short word);
extern void HwMdSetVram(unsigned short word);
extern void HwMdPuts(char *str, int color, int x, int y);
extern void HwMdPutc(char chr, int color, int x, int y);

#endif
