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
#ifndef SGIP_HUB_H
#define SGIP_HUB_H

#include "sgIP_Config.h"
#include "sgIP_memblock.h"



#define SGIP_FLAG_PROTOCOL_IN_USE				0x0001
#define SGIP_FLAG_PROTOCOL_ENABLED				0x8000

#define SGIP_FLAG_HWINTERFACE_IN_USE			0x0001
#define SGIP_FLAG_HWINTERFACE_CONNECTED			0x0002
#define SGIP_FLAG_HWINTERFACE_USEDHCP			0x0004
#define SGIP_FLAG_HWINTERFACE_CHANGENETWORK		0x0008
#define SGIP_FLAG_HWINTERFACE_ENABLED			0x8000

#ifdef SGIP_LITTLEENDIAN
#define PROTOCOL_ETHER_ARP                   0x0608
#define PROTOCOL_ETHER_IP                    0x0008
#else
#define PROTOCOL_ETHER_ARP                   0x0806
#define PROTOCOL_ETHER_IP                    0x0800
#endif


// structure sgIP_Hub_Protocol: Used to record the interface between the sgIP Hub and a protocol handler
typedef struct SGIP_HUB_PROTOCOL {
	unsigned short flags;
	unsigned short protocol;
	int (*ReceivePacket)(sgIP_memblock *);

} sgIP_Hub_Protocol;

typedef struct SGIP_HUB_HWINTERFACE {
	unsigned short flags;
	unsigned short hwaddrlen;
	int MTU;
	int (*TransmitFunction)(struct SGIP_HUB_HWINTERFACE *, sgIP_memblock *);
	void * userdata;
	unsigned long ipaddr, gateway, snmask, dns[3];
	unsigned char hwaddr[SGIP_MAXHWADDRLEN];
} sgIP_Hub_HWInterface;

typedef struct SGIP_HEADER_ETHERNET {
	unsigned char dest_mac[6];
	unsigned char src_mac[6];
	unsigned short protocol; 
} sgIP_Header_Ethernet;

#define ntohs(num) htons(num)
#define ntohl(num) htonl(num)


#ifdef __cplusplus
extern "C" {
#endif


extern void sgIP_Hub_Init();


extern sgIP_Hub_Protocol * sgIP_Hub_AddProtocolInterface(int protocolID, int (*ReceivePacket)(sgIP_memblock *), int (*InterfaceInit)(sgIP_Hub_Protocol *));
extern sgIP_Hub_HWInterface * sgIP_Hub_AddHardwareInterface(int (*TransmitFunction)(sgIP_Hub_HWInterface *, sgIP_memblock *), int (*InterfaceInit)(sgIP_Hub_HWInterface *));
extern void sgIP_Hub_RemoveProtocolInterface(sgIP_Hub_Protocol * protocol);
extern void sgIP_Hub_RemoveHardwareInterface(sgIP_Hub_HWInterface * hw);

extern int sgIP_Hub_ReceiveHardwarePacket(sgIP_Hub_HWInterface * hw, sgIP_memblock * packet);
extern int sgIP_Hub_SendProtocolPacket(int protocol, sgIP_memblock * packet, unsigned long dest_address, unsigned long src_address);
extern int sgIP_Hub_SendRawPacket(sgIP_Hub_HWInterface * hw, sgIP_memblock * packet);

extern int sgIP_Hub_IPMaxMessageSize(unsigned long ipaddr);
unsigned long sgIP_Hub_GetCompatibleIP(unsigned long destIP);

extern sgIP_Hub_HWInterface * sgIP_Hub_GetDefaultInterface();

unsigned short htons(unsigned short num);
unsigned long htonl(unsigned long num);

#ifdef __cplusplus
};
#endif


#endif
