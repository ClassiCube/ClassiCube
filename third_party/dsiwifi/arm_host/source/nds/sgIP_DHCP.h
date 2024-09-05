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
#ifndef SGIP_DHCP_H
#define SGIP_DHCP_H

#include "sgIP_Config.h"
#include "sgIP_Hub.h"

// "DHCP Server" port is 67, "DHCP Client" port is 68
// DHCP messages broadcast by a client prior to that client obtaining its IP address must have the source address field in the IP header set to 0.



typedef struct SGIP_DHCP_PACKET { // yes, freaking big endian prevails here too.
   unsigned char op;       // opcode/message type (1=BOOTREQUEST, 2=BOOTREPLY)
   unsigned char htype;    // hardware address type
   unsigned char hlen;     // Hardware address length (should be 6, for ethernet/wifi)
   unsigned char hops;     // set to 0
   unsigned long xid;      // 4-byte client specified transaction ID
   unsigned short secs;    // seconds elapsed since client started trying to boot
   unsigned short flags;   // flags
   unsigned long ciaddr;   // client IP address, filled in by client if verifying previous params
   unsigned long yiaddr;   // "your" (client) IP address
   unsigned long siaddr;   // IP addr of next server to use in bootstrap.
   unsigned long giaddr;   // Relay agent IP address
   unsigned char chaddr[16];  // client hardware address
   char sname[64];         // optional server hostname (null terminated string)
   char file[128];         // boot file name, null terminated string
   char options[312];      // optional parameters
} sgIP_DHCP_Packet;

enum SGIP_DHCP_STATUS {
   SGIP_DHCP_STATUS_IDLE,
   SGIP_DHCP_STATUS_WORKING,
   SGIP_DHCP_STATUS_FAILED,
   SGIP_DHCP_STATUS_SUCCESS
};

#define DHCP_TYPE_DISCOVER	1
#define DHCP_TYPE_OFFER		2
#define DHCP_TYPE_REQUEST	3
#define DHCP_TYPE_ACK		5
#define DHCP_TYPE_RELEASE	7


#ifdef __cplusplus
extern "C" {
#endif

   void sgIP_DHCP_Init();

   void sgIP_DHCP_SetHostName(char * s); // just for the fun of it.
   int sgIP_DHCP_IsDhcpIp(unsigned long ip); // check if the IP address was assigned via dhcp.
   void sgIP_DHCP_Start(sgIP_Hub_HWInterface * interface, int getDNS); // begin dhcp transaction to get IP and maybe DNS data.
   void sgIP_DHCP_Release(); // call to dump our DHCP address and leave.
   int  sgIP_DHCP_Update(); // MUST be called periodicly after _Start; returns status - call until it returns something other than SGIP_DHCP_STATUS_WORKING
   void sgIP_DHCP_Terminate(); // kill the process where it stands; deallocate all DHCP resources.

#ifdef __cplusplus
};
#endif


#endif
