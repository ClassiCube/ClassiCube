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


#ifndef SGIP_H
#define SGIP_H

#include "sgIP_Config.h"
#include "sgIP_memblock.h"
#include "sgIP_sockets.h"
#include "sgIP_Hub.h"
#include "sgIP_IP.h"
#include "sgIP_ARP.h"
#include "sgIP_ICMP.h"
#include "sgIP_TCP.h"
#include "sgIP_UDP.h"
#include "sgIP_DNS.h"
#include "sgIP_DHCP.h"

extern unsigned long volatile sgIP_timems;

#ifdef __cplusplus
extern "C" {
#endif

   void sgIP_Init();
   void sgIP_Timer(int num_ms);


#ifdef __cplusplus
};
#endif


#endif
