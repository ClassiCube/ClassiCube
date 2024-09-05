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


#ifndef SGIP_CONFIG_H
#define SGIP_CONFIG_H

#define __LINUX_ERRNO_EXTENSIONS__
#include <errno.h>
#include <sys/errno.h>

//////////////////////////////////////////////////////////////////////////
// General options - these control the core functionality of the stack.

// SGIP_USEDYNAMICMEMORY: Allows the stack to use memory as it needs it, via malloc()/free()
//  This option is extremely useful in environments where it can be used, as it prevents the 
//  overhead of allocating per-connection memory in advance, and allows an unlimited number
//  of connections, provided the memory space.  This option requires the implementation of
//  two C functions, "void * sgIP_malloc(int)" and "void sgIP_free(void *)", which behave 
//  similarly to the malloc and free functions commonly used in C.
#define SGIP_USEDYNAMICMEMORY

// SGIP_INTERRUPT_THREADING_MODEL: Provides memory protection in a system that can allow 
//  multiple processing "threads" by way of interrupts.  This is not required on single
//  threaded systems, and not adequate on multithreaded systems, but provides a way to
//  allow protection against contention on interrupt-driven systems.  This option requires
//  the system to implement two C functions "int sgIP_DisableInterrupts()" and additionally
//  "void sgIP_RestoreInterrupts(int)" that takes as a parameter the value returned by
//  sgIP_DisableInterrupts().  Interrupts are disabled upon beginning work with sensitive
//  memory areas or allocation/deallocation of memory, and are restored afterwards.
#define SGIP_INTERRUPT_THREADING_MODEL

// SGIP_MULTITHREADED_THREADING_MODEL: Standard memory protection for large multithreaded
//  systems, such as operating systems and the like.  This kind of memory protection is
//  useful for true multithreaded systems but useless in a single-threaded system and 
//  harmful in an interrupt-based multiprocess system.
//#define SGIP_MULTITHREADED_THREADING_MODEL

#define SGIP_LITTLEENDIAN

//////////////////////////////////////////////////////////////////////////
// Temporary memory system settings

// SGIP_MEMBLOCK_DATASIZE: This is the maximum data size contained in a single sgIP_memblock.
//  for best performance ensure this value is larger than any packet that is expected to be
//  received, however, in a memory-tight situation, much smaller values can be used.
#define SGIP_MEMBLOCK_DATASIZE		1600

// SGIP_MEMBLOCK_BASENUM: The starting number of memblocks that will be allocated. This is 
//  also the total number of memblocks that will be allocated if sgIP is not configured to use
//  dynamic memory allocation.
#define SGIP_MEMBLOCK_BASENUM		12

// SGIP_MEMBLOCK_STEPNUM: In the case that all memblocks are full, and dynamic memory is
//  enabled, this many additional memblocks will be allocated in an attempt to satasfy the
//  memory usage demands of the stack.
#define SGIP_MEMBLOCK_STEPNUM		6

// SGIP_MEMBLOCK_DYNAMIC_MALLOC_ALL: Who cares what the other memblock defines say, let's
// Generate all memblocks by mallocing 'em.
#define SGIP_MEMBLOCK_DYNAMIC_MALLOC_ALL

//////////////////////////////////////////////////////////////////////////
// Hardware layer settings

// SGIP_MAXHWADDRLEN: The maximum usable hardware address length.  Ethernet is 6 bytes.
#define SGIP_MAXHWADDRLEN	8

// SGIP_MAXHWHEADER: The maximum allocated size for hardware headers.
#define SGIP_MAXHWHEADER	16

// SGIP_MTU_OVERRIDE: This is the maximum MTU that will be accepted.  By default it is being
//  set to 1460 bytes in order to be courteous to Ethernet and it's ridiculously low MTU.
//  This value will allow you to prevent fragmentation of IP packets by not using the full
//  MTU available to your network interface when the IP packet will just be sliced and diced
//  at the next smaller MTU. (the stack will still use HW mtu if it's lower.)
#define SGIP_MTU_OVERRIDE	1460


//////////////////////////////////////////////////////////////////////////
// Connection settings - can be tuned to change memory usage and performance

// SGIP_TCP_STATELESS_LISTEN: Uses a technique to prevent syn-flooding from blocking listen
//  ports by using all the connection blocks/memory.
#define SGIP_TCP_STATELESS_LISTEN

// SGIP_TCP_STEALTH: Only sends packets in response to connections to active ports. Doing so
//  causes ports to appear not as closed, but as if the deviced does not exist when probing
//  ports that are not in use.
//#define SGIP_TCP_STEALTH

// SGIP_TCP_TTL: Time-to-live value given to outgoing packets, in the absence of a reason to
//  manually override this value.
#define SGIP_IP_TTL								128

// SGIP_TCPRECEIVEBUFFERLENGTH: The size (in bytes) of the receive FIFO in a TCP connection
#define SGIP_TCP_RECEIVEBUFFERLENGTH			8192

// SGIP_TCPTRANSMITBUFFERLENGTH: The size (in bytes) of the transmit FIFO in a TCP connection
#define SGIP_TCP_TRANSMITBUFFERLENGTH			8192

// SGIP_TCPOOBBUFFERLENGTH: The size (in bytes) of the receive OOB data FIFO in a TCP connection
#define SGIP_TCP_OOBBUFFERLENGTH				256

// SGIP_ARP_MAXENTRIES: The maximum number of cached ARP entries - this is defined staticly
//  because it's somewhat impractical to dynamicly allocate memory for such a small structure
//  (at least on most smaller systems)
#define SGIP_ARP_MAXENTRIES						32

// SGIP_HUB_MAXHWINTERFACES: The maximum number of hardware interfaces the sgIP hub will 
//  connect to. A hardware interface being some port (ethernet, wifi, etc) that will relay
//  packets to the outside world.
#define SGIP_HUB_MAXHWINTERFACES				1

// SGIP_HUB_MAXPROTOCOLINTERFACES: The maximum number of protocol interfaces the sgIP hub will
//  connect to. A protocol interface being a software handler for a certain protocol type
//  (such as IP)
#define SGIP_HUB_MAXPROTOCOLINTERFACES			1

#define SGIP_TCP_FIRSTOUTGOINGPORT				40000
#define SGIP_TCP_LASTOUTGOINGPORT				65000
#define SGIP_UDP_FIRSTOUTGOINGPORT				40000
#define SGIP_UDP_LASTOUTGOINGPORT				65000

#define SGIP_TCP_GENTIMEOUTMS                6000
#define SGIP_TCP_TRANSMIT_DELAY              25
#define SGIP_TCP_TRANSMIT_IMMTHRESH          40
#define SGIP_TCP_TIMEMS_2MSL                 1000*60*2
#define SGIP_TCP_MAXRETRY                    7
#define SGIP_TCP_MAXSYNS                     64
#define SGIP_TCP_REACK_THRESH                   1000

#define SGIP_TCP_SYNRETRYMS						250
#define SGIP_TCP_GENRETRYMS						500
#define SGIP_TCP_BACKOFFMAX						6000

#define SGIP_SOCKET_MAXSOCKETS					32

//#define SGIP_SOCKET_DEFAULT_NONBLOCK			1


//////////////////////////////////////////////////////////////////////////
//  DNS settings

#define SGIP_DNS_MAXRECORDSCACHE             16
#define SGIP_DNS_MAXRECORDADDRS              4
#define SGIP_DNS_MAXALIASES                  4
#define SGIP_DNS_TIMEOUTMS                   5000
#define SGIP_DNS_MAXRETRY                    3
#define SGIP_DNS_MAXSERVERRETRY              4

//////////////////////////////////////////////////////////////////////////

#define SGIP_DHCP_ERRORTIMEOUT               45000
#define SGIP_DHCP_RESENDTIMEOUT				 3000
#define SGIP_DHCP_DEFAULTHOSTNAME            "NintendoDS"
#define SGIP_DHCP_CLASSNAME                  "sgIP 0.3"

//////////////////////////////////////////////////////////////////////////
//  Static memory settings - only used if SGIP_USEDYNAMICMEMORY is NOT defined.

// SGIP_TCP_MAXCONNECTIONS: In the case dynamic memory is not used, this value gives the max
//  number of TCP blocks available for inbound/outbound connections via TCP.
#define SGIP_TCP_MAXCONNECTIONS					10




//////////////////////////////////////////////////////////////////////////
// Debugging options


// SGIP_DEBUG: Enable debug logging.
//  requires external function "void sgIP_dbgprint(char *, ...);"
//#define SGIP_DEBUG

#ifdef SGIP_DEBUG
#define SGIP_DEBUG_MESSAGE(param) sgIP_dbgprint param
#define SGIP_DEBUG_ERROR(param) sgIP_dbgprint param; while(1);
#else
#define SGIP_DEBUG_MESSAGE(param) 
#define SGIP_DEBUG_ERROR(param)
#endif


//////////////////////////////////////////////////////////////////////////
//  Error handling
extern int sgIP_errno;
#define SGIP_ERROR(a) ((errno=(a)), -1)
#define SGIP_ERROR0(a) ((errno=(a)), 0)

//////////////////////////////////////////////////////////////////////////
//  Error checking


#ifdef SGIP_MULTITHREADED_THREADING_MODEL
#ifdef SGIP_INTERRUPT_THREADING_MODEL
#error SGIP_INTERRUPT_THREADING_MODEL and SGIP_MULTITHREADED_THREADING_MODEL cannot be used together!
#endif
#endif


//////////////////////////////////////////////////////////////////////////
// External option-based dependencies
#include <nds/interrupts.h>

#ifdef SGIP_INTERRUPT_THREADING_MODEL
#ifdef __cplusplus
extern "C" {
#endif
   void sgIP_IntrWaitEvent();
#ifdef __cplusplus
};
#endif
#define SGIP_INTR_PROTECT() \
	int tIME; \
	tIME=enterCriticalSection()
#define SGIP_INTR_REPROTECT() \
	tIME=enterCriticalSection()
#define SGIP_INTR_UNPROTECT() \
	leaveCriticalSection(tIME)
#define SGIP_WAITEVENT() \
   sgIP_IntrWaitEvent()
#else // !SGIP_INTERRUPT_THREADING_MODEL
#define SGIP_INTR_PROTECT()
#define SGIP_INTR_REPROTECT()
#define SGIP_INTR_UNPROTECT()
#define SGIP_WAITEVENT();
#endif // SGIP_INTERRUPT_THREADING_MODEL

#ifdef SGIP_DEBUG
#ifdef __cplusplus
extern "C" {
#endif
	extern void sgIP_dbgprint(char *, ...);
#ifdef __cplusplus
};
#endif
#endif // SGIP_DEBUG

#ifdef SGIP_USEDYNAMICMEMORY
#ifdef __cplusplus
extern "C" {
#endif
	extern void * sgIP_malloc(int);
	extern void sgIP_free(void *);
#ifdef __cplusplus
};
#endif
#endif // SGIP_USEDYNAMICMEMORY

#endif
