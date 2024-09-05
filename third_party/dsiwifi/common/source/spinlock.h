// DS Wifi interface code
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
// spinlock.h - code for spinlocking for basic wifi structure memory protection
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



/*

__asm (
".GLOBL SLasm_Acquire, SLasm_Release   \n"
".ARM \n"
"SLasm_Acquire:					\n"
"   ldr r2,[r0]					\n"
"   cmp r2,#0					\n"
"   movne r0,#1	                \n"
"   bxne lr		                \n"
"   mov	r2,r1					\n"
"   swp r2,r2,[r0]				\n"
"   cmp r2,#0					\n"
"   cmpne r2,r1	                \n"
"   moveq r0,#0	                \n"
"   bxeq lr		                \n"
"   swp r2,r2,[r0]				\n"
"   mov r0,#1                   \n"
"   bx lr		                \n"
"\n\n"
"SLasm_Release:					\n"
"   ldr r2,[r0]					\n"
"   cmp r2,r1	                \n"
"   movne r0,#2                 \n"
"   bxne lr		                \n"
"   mov r2,#0					\n"
"   swp r2,r2,[r0]				\n"
"   cmp r2,r1					\n"
"   moveq r0,#0	                \n"
"   movne r0,#2                 \n"
"   bx lr		                \n"	  
);

*/

