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
#ifndef SGIP_DNS_H
#define SGIP_DNS_H

#include "sgIP_Config.h"

#define SGIP_DNS_FLAG_ACTIVE     1
#define SGIP_DNS_FLAG_RESOLVED   2
#define SGIP_DNS_FLAG_BUSY       4

typedef struct SGIP_DNS_RECORD {
   char              name [256];
   char              aliases[SGIP_DNS_MAXALIASES][256];
   unsigned char     addrdata[SGIP_DNS_MAXRECORDADDRS*4];
   short             addrlen;
   short             addrclass;
   int               numaddr,numalias;
   int               TTL;
   int               flags;
} sgIP_DNS_Record;

typedef struct SGIP_DNS_HOSTENT {
   char *           h_name;
   char **          h_aliases;
   int              h_addrtype; // class - 1=IN (internet)
   int              h_length;
   char **          h_addr_list;
} sgIP_DNS_Hostent;

#ifdef __cplusplus
extern "C" {
#endif

extern void sgIP_DNS_Init();
extern void sgIP_DNS_Timer1000ms();

extern sgIP_DNS_Hostent * sgIP_DNS_gethostbyname(const char * name);
extern sgIP_DNS_Record  * sgIP_DNS_GetUnusedRecord();
extern sgIP_DNS_Record  * sgIP_DNS_FindDNSRecord(const char * name);

#ifdef __cplusplus
};
#endif


#endif
