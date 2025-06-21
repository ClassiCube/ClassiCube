// DSWifi Project - sgIP Internet Protocol Stack Implementation
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
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


#ifndef SGIP_MEMBLOCK_H
#define SGIP_MEMBLOCK_H

#include "sgIP_Config.h"


typedef struct SGIP_MEMBLOCK {
	int totallength;
	int thislength;
	struct SGIP_MEMBLOCK * next;
	char * datastart;
	char reserved[SGIP_MEMBLOCK_DATASIZE-16]; // assume the other 4 values are 16 bytes total in length.
} sgIP_memblock;

#define SGIP_MEMBLOCK_HEADERSIZE 16
#define SGIP_MEMBLOCK_INTERNALSIZE (SGIP_MEMBLOCK_DATASIZE-16)
#define SGIP_MEMBLOCK_FIRSTINTERNALSIZE (SGIP_MEMBLOCK_DATASIZE-16-SGIP_MAXHWHEADER)

#ifdef __cplusplus
extern "C" {
#endif

	extern void sgIP_memblock_Init();
	extern sgIP_memblock * sgIP_memblock_alloc(int packetsize);
	extern sgIP_memblock * sgIP_memblock_allocHW(int headersize, int packetsize);
	extern void sgIP_memblock_free(sgIP_memblock * mb);
	extern void sgIP_memblock_exposeheader(sgIP_memblock * mb, int change);
	extern void sgIP_memblock_trimsize(sgIP_memblock * mb, int newsize);

	extern int sgIP_memblock_IPChecksum(sgIP_memblock * mb, int startbyte, int chksum_length);
	extern int sgIP_memblock_CopyToLinear(sgIP_memblock * mb, void * dest_buf, int startbyte, int copy_length);
	extern int sgIP_memblock_CopyFromLinear(sgIP_memblock * mb, void * src_buf, int startbyte, int copy_length);
	extern int sgIP_memblock_CopyBlock(sgIP_memblock * mb_src, sgIP_memblock * mb_dest, int start_src, int start_dest, int copy_length);
#ifdef __cplusplus
};
#endif


#endif
