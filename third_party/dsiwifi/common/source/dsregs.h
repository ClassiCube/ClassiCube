//////////////////////////////////////////////////////////////////////////
// DSRegs.h - (c) 2005-2006 Stephen Stair
// General reference mumbo jumbo to give me easy access to the regs I like.
//////////////////////////////////////////////////////////////////////////
/****************************************************************************** 
DSWifi Lib and test materials are licenced under the MIT open source licence:
Copyright (c) 2005-2006 Stephen Stair

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef DSREGS_H
#define DSREGS_H

#include <nds.h>


//////////////////////////////////////////////////////////////////////////
// General registers


// general memory range defines
#define		PAL			((u16 *) 0x05000000)
#define		VRAM1		((u16 *) 0x06000000)
#define		VRAM2		((u16 *) 0x06200000)

//#define		OAM			((u16 *) 0x07000000)
#define		CART		((u16 *) 0x08000000)


// video registers
#define		DISPCNT		(*((u32 volatile *) 0x04000000))
#define		DISPSTAT	(*((u16 volatile *) 0x04000004))
#define		VCOUNT		(*((u16 volatile *) 0x04000006))
#define		BG0CNT		(*((u16 volatile *) 0x04000008))
#define		BG1CNT		(*((u16 volatile *) 0x0400000A))
#define		BG2CNT		(*((u16 volatile *) 0x0400000C))
#define		BG3CNT		(*((u16 volatile *) 0x0400000E))
#define		BG0HOFS		(*((u16 volatile *) 0x04000010))
#define		BG0VOFS		(*((u16 volatile *) 0x04000012))
#define		BG1HOFS		(*((u16 volatile *) 0x04000014))
#define		BG1VOFS		(*((u16 volatile *) 0x04000016))
#define		BG2HOFS		(*((u16 volatile *) 0x04000018))
#define		BG2VOFS		(*((u16 volatile *) 0x0400001A))
#define		BG3HOFS		(*((u16 volatile *) 0x0400001C))
#define		BG3VOFS		(*((u16 volatile *) 0x0400001E))
#define		BG2PA		(*((u16 volatile *) 0x04000020))
#define		BG2PB		(*((u16 volatile *) 0x04000022))
#define		BG2PC		(*((u16 volatile *) 0x04000024))
#define		BG2PD		(*((u16 volatile *) 0x04000026))
#define		BG2X		(*((u32 volatile *) 0x04000028))
#define		BG2Y		(*((u32 volatile *) 0x0400002C))
#define		BG3PA		(*((u16 volatile *) 0x04000030))
#define		BG3PB		(*((u16 volatile *) 0x04000032))
#define		BG3PC		(*((u16 volatile *) 0x04000034))
#define		BG3PD		(*((u16 volatile *) 0x04000036))
#define		BG3X		(*((u32 volatile *) 0x04000038))
#define		BG3Y		(*((u32 volatile *) 0x0400003C))
#define		WIN0H		(*((u16 volatile *) 0x04000040))
#define		WIN1H		(*((u16 volatile *) 0x04000042))
#define		WIN0V		(*((u16 volatile *) 0x04000044))
#define		WIN1V		(*((u16 volatile *) 0x04000046))
#define		WININ		(*((u16 volatile *) 0x04000048))
#define		WINOUT		(*((u16 volatile *) 0x0400004A))
#define		MOSAIC		(*((u16 volatile *) 0x0400004C))
#define		BLDCNT		(*((u16 volatile *) 0x04000050))
#define		BLDALPHA	(*((u16 volatile *) 0x04000052))
#define		BLDY		(*((u16 volatile *) 0x04000054))

#define		DISPCNT2	(*((u32 volatile *) 0x04001000))
#define		DISPSTAT2	(*((u16 volatile *) 0x04001004))
#define		VCOUNT2		(*((u16 volatile *) 0x04001006))
#define		BG0CNT2		(*((u16 volatile *) 0x04001008))
#define		BG1CNT2		(*((u16 volatile *) 0x0400100A))
#define		BG2CNT2		(*((u16 volatile *) 0x0400100C))
#define		BG3CNT2		(*((u16 volatile *) 0x0400100E))
#define		BG0HOFS2	(*((u16 volatile *) 0x04001010))
#define		BG0VOFS2	(*((u16 volatile *) 0x04001012))
#define		BG1HOFS2	(*((u16 volatile *) 0x04001014))
#define		BG1VOFS2	(*((u16 volatile *) 0x04001016))
#define		BG2HOFS2	(*((u16 volatile *) 0x04001018))
#define		BG2VOFS2	(*((u16 volatile *) 0x0400101A))
#define		BG3HOFS2	(*((u16 volatile *) 0x0400101C))
#define		BG3VOFS2	(*((u16 volatile *) 0x0400101E))
#define		BG2PA2		(*((u16 volatile *) 0x04001020))
#define		BG2PB2		(*((u16 volatile *) 0x04001022))
#define		BG2PC2		(*((u16 volatile *) 0x04001024))
#define		BG2PD2		(*((u16 volatile *) 0x04001026))
#define		BG2X2		(*((u32 volatile *) 0x04001028))
#define		BG2Y2		(*((u32 volatile *) 0x0400102C))
#define		BG3PA2		(*((u16 volatile *) 0x04001030))
#define		BG3PB2		(*((u16 volatile *) 0x04001032))
#define		BG3PC2		(*((u16 volatile *) 0x04001034))
#define		BG3PD2		(*((u16 volatile *) 0x04001036))
#define		BG3X2		(*((u32 volatile *) 0x04001038))
#define		BG3Y2		(*((u32 volatile *) 0x0400103C))
#define		WIN0H2		(*((u16 volatile *) 0x04001040))
#define		WIN1H2		(*((u16 volatile *) 0x04001042))
#define		WIN0V2		(*((u16 volatile *) 0x04001044))
#define		WIN1V2		(*((u16 volatile *) 0x04001046))
#define		WININ2		(*((u16 volatile *) 0x04001048))
#define		WINOUT2		(*((u16 volatile *) 0x0400104A))
#define		MOSAIC2		(*((u16 volatile *) 0x0400104C))
#define		BLDCNT2		(*((u16 volatile *) 0x04001050))
#define		BLDALPHA2	(*((u16 volatile *) 0x04001052))
#define		BLDY2		(*((u16 volatile *) 0x04001054))

// video memory defines
#define		PAL_BG1		((u16 *) 0x05000000)
#define		PAL_FG1		((u16 *) 0x05000200)
#define		PAL_BG2		((u16 *) 0x05000400)
#define		PAL_FG2		((u16 *) 0x05000600)

// other video defines
#define		VRAMBANKCNT	(((u16 volatile *) 0x04000240))

#define		RGB(r,g,b)	( ((r)&31) | (((g)&31)<<5) | (((b)&31)<<10) )
#define		VRAM_SETBANK(bank, set)	\
	if((bank)&1) { VRAMBANKCNT[(bank)>>1] = (VRAMBANKCNT[(bank)>>1]&0x00ff) | (((set)&0xff)<<8); } else \
		{ VRAMBANKCNT[(bank)>>1] = (VRAMBANKCNT[(bank)>>1]&0xff00) | ((set)&0xff); }

// joypad input 
#define		KEYINPUT	(*((u16 volatile *) 0x04000130))
#define		KEYCNT		(*((u16 volatile *) 0x04000132))


// System registers
#define		WAITCNT		(*((u16 volatile *) 0x04000204))
//#define		IME			(*((u16 volatile *) 0x04000208))
//#define		IE			(*((u32 volatile *) 0x04000210))
//#define		IF			(*((u32 volatile *) 0x04000214))
#define		HALTCNT		(*((u16 volatile *) 0x04000300))






//////////////////////////////////////////////////////////////////////////
// ARM7 specific registers
#ifdef ARM7
#define		POWERCNT7	(*((u16 volatile *) 0x04000304))

#define		SPI_CR		(*((u16 volatile *) 0x040001C0))
#define		SPI_DATA	(*((u16 volatile *) 0x040001C2))



#endif

//////////////////////////////////////////////////////////////////////////
// ARM9 specific registers
#ifdef ARM9
#define		POWERCNT	(*((u16 volatile *) 0x04000308))



#endif
// End of file!
#endif



