// DSWifi Project - Arm9 Library Header File (dswifi9.h)
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

#ifndef DSWIFI9_H
#define DSWIFI9_H

#include <nds/ndstypes.h>

#include "dswifi_version.h"

// well, some flags and stuff are just stuffed in here and not documented very well yet... Most of the important stuff is documented though.
// Next version should clean up some of this a lot more :)

#define WIFIINIT_OPTION_USELED		   0x0002

// default option is to use 64k heap
#define WIFIINIT_OPTION_USEHEAP_64     0x0000
#define WIFIINIT_OPTION_USEHEAP_128    0x1000
#define WIFIINIT_OPTION_USEHEAP_256    0x2000
#define WIFIINIT_OPTION_USEHEAP_512    0x3000
#define WIFIINIT_OPTION_USECUSTOMALLOC 0x4000
#define WIFIINIT_OPTION_HEAPMASK       0xF000

#define WFLAG_PACKET_DATA		0x0001
#define WFLAG_PACKET_MGT		0x0002
#define WFLAG_PACKET_BEACON		0x0004
#define WFLAG_PACKET_CTRL		0x0008

#define WFLAG_PACKET_ALL		0xFFFF


#define WFLAG_APDATA_ADHOC			0x0001
#define WFLAG_APDATA_WEP			0x0002
#define WFLAG_APDATA_WPA			0x0004
#define WFLAG_APDATA_COMPATIBLE		0x0008
#define WFLAG_APDATA_EXTCOMPATIBLE	0x0010
#define WFLAG_APDATA_SHORTPREAMBLE	0x0020
#define WFLAG_APDATA_ACTIVE			0x8000

enum WIFI_RETURN {
	WIFI_RETURN_OK =				0, // Everything went ok
	WIFI_RETURN_LOCKFAILED  =		1, // the spinlock attempt failed (it wasn't retried cause that could lock both cpus- retry again after a delay.
	WIFI_RETURN_ERROR =				2, // There was an error in attempting to complete the requested task.
	WIFI_RETURN_PARAMERROR =		3, // There was an error in the parameters passed to the function.
};

enum WIFI_STATS {
	// software stats
	WSTAT_RXQUEUEDPACKETS, // number of packets queued into the rx fifo
	WSTAT_TXQUEUEDPACKETS, // number of packets queued into the tx fifo
	WSTAT_RXQUEUEDBYTES, // number of bytes queued into the rx fifo
	WSTAT_TXQUEUEDBYTES, // number of bytes queued into the tx fifo
	WSTAT_RXQUEUEDLOST, // number of packets lost due to space limitations in queuing
	WSTAT_TXQUEUEDREJECTED, // number of packets rejected due to space limitations in queuing
	WSTAT_RXPACKETS,
	WSTAT_RXBYTES,
	WSTAT_RXDATABYTES,
	WSTAT_TXPACKETS,
	WSTAT_TXBYTES,
	WSTAT_TXDATABYTES,
	WSTAT_ARM7_UPDATES,
	WSTAT_DEBUG,
	// harware stats (function mostly unknown.)
	WSTAT_HW_1B0,WSTAT_HW_1B1,WSTAT_HW_1B2,WSTAT_HW_1B3,WSTAT_HW_1B4,WSTAT_HW_1B5,WSTAT_HW_1B6,WSTAT_HW_1B7,
	WSTAT_HW_1B8,WSTAT_HW_1B9,WSTAT_HW_1BA,WSTAT_HW_1BB,WSTAT_HW_1BC,WSTAT_HW_1BD,WSTAT_HW_1BE,WSTAT_HW_1BF,
	WSTAT_HW_1C0,WSTAT_HW_1C1,WSTAT_HW_1C4,WSTAT_HW_1C5,
	WSTAT_HW_1D0,WSTAT_HW_1D1,WSTAT_HW_1D2,WSTAT_HW_1D3,WSTAT_HW_1D4,WSTAT_HW_1D5,WSTAT_HW_1D6,WSTAT_HW_1D7,
	WSTAT_HW_1D8,WSTAT_HW_1D9,WSTAT_HW_1DA,WSTAT_HW_1DB,WSTAT_HW_1DC,WSTAT_HW_1DD,WSTAT_HW_1DE,WSTAT_HW_1DF,

	NUM_WIFI_STATS
};

// user code should NEVER have to use the WIFI_MODE or WIFI_AUTHLEVEL enums... is here in case I want to have some debug code...
enum WIFI_MODE {
	WIFIMODE_DISABLED,
	WIFIMODE_NORMAL,
	WIFIMODE_SCAN,
	WIFIMODE_ASSOCIATE,
	WIFIMODE_ASSOCIATED,
	WIFIMODE_DISASSOCIATE,
	WIFIMODE_CANNOTASSOCIATE,
};
enum WIFI_AUTHLEVEL {
	WIFI_AUTHLEVEL_DISCONNECTED,
	WIFI_AUTHLEVEL_AUTHENTICATED,
	WIFI_AUTHLEVEL_ASSOCIATED,
	WIFI_AUTHLEVEL_DEASSOCIATED,
};

// user code uses members of the WIFIGETDATA structure in calling Wifi_GetData to retreive miscellaneous odd information
enum WIFIGETDATA {
	WIFIGETDATA_MACADDRESS,			// MACADDRESS: returns data in the buffer, requires at least 6 bytes
	WIFIGETDATA_NUMWFCAPS,			// NUM WFC APS: returns number between 0 and 3, doesn't use buffer.

	MAX_WIFIGETDATA
};


enum WEPMODES {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
};
// WIFI_ASSOCSTATUS - returned by Wifi_AssocStatus() after calling Wifi_ConnectAPk
enum WIFI_ASSOCSTATUS {
	ASSOCSTATUS_DISCONNECTED, // not *trying* to connect
	ASSOCSTATUS_SEARCHING, // data given does not completely specify an AP, looking for AP that matches the data.
	ASSOCSTATUS_AUTHENTICATING, // connecting...
	ASSOCSTATUS_ASSOCIATING, // connecting...
	ASSOCSTATUS_ACQUIRINGDHCP, // connected to AP, but getting IP data from DHCP
	ASSOCSTATUS_ASSOCIATED,	// Connected! (COMPLETE if Wifi_ConnectAP was called to start)
	ASSOCSTATUS_CANNOTCONNECT, // error in connecting... (COMPLETE if Wifi_ConnectAP was called to start)
};

extern const char * ASSOCSTATUS_STRINGS[];

// most user code will never need to know about the WIFI_TXHEADER or WIFI_RXHEADER
typedef struct WIFI_TXHEADER {
	u16 enable_flags;
	u16 unknown;
	u16 countup;
	u16 beaconfreq;
	u16 tx_rate;
	u16 tx_length;
} Wifi_TxHeader;

typedef struct WIFI_RXHEADER {
	u16 a;
	u16 b;
	u16 c;
	u16 d;
	u16 byteLength;
	u16 rssi_;
} Wifi_RxHeader;

// WIFI_ACCESSPOINT is an important structure in that it defines how to connect to an access point.
// listed inline are information about the members and their function
// if a field is not necessary for Wifi_ConnectAP it will be marked as such
// *only* 4 fields are absolutely required to be filled in correctly for the connection to work, they are:
// ssid, ssid_len, bssid, and channel - all others can be ignored (though flags should be set to 0)
typedef struct WIFI_ACCESSPOINT {
	char ssid[33]; // the AP's SSID - zero terminated is not necessary.. if ssid[0] is zero, the ssid will be ignored in trying to find an AP to connect to. [REQUIRED]
	char ssid_len; // number of valid bytes in the ssid field (0-32) [REQUIRED]
	u8 bssid[6]; // BSSID is the AP's SSID - setting it to all 00's indicates this is not known and it will be ignored [REQUIRED]
	u8 macaddr[6]; // mac address of the "AP" is only necessary in ad-hoc mode. [generally not required to connect]
	u16 maxrate; // max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B [not required to connect]
	u32 timectr; // internal information about how recently a beacon has been received [not required to connect]
	u16 rssi; // running average of the recent RSSI values for this AP, will be set to 0 after not receiving beacons for a while. [not required to connect]
	u16 flags; // flags indicating various parameters for the AP [not required, but the WFLAG_APDATA_ADHOC flag will be used]
	u32 spinlock; // internal data word used to lock the record to guarantee data coherence [not required to connect]
	u8 channel; // valid channels are 1-13, setting the channel to 0 will indicate the system should search. [REQUIRED]
	u8 rssi_past[8]; // rssi_past indicates the RSSI values for the last 8 beacons received ([7] is the most recent) [not required to connect]
	u8 base_rates[16]; // list of the base rates "required" by the AP (same format as maxrate) - zero-terminated list [not required to connect]
} Wifi_AccessPoint;

// Wifi Packet Handler function: (int packetID, int packetlength) - packetID is only valid while the called function is executing.
// call Wifi_RxRawReadPacket while in the packet handler function, to retreive the data to a local buffer.
typedef void (*WifiPacketHandler)(int, int);

// Wifi Sync Handler function: Callback function that is called when the arm7 needs to be told to synchronize with new fifo data.
// If this callback is used (see Wifi_SetSyncHandler()), it should send a message via the fifo to the arm7, which will call Wifi_Sync() on arm7.
typedef void (*WifiSyncHandler)();


#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// Init/update/state management functions

// Wifi_Init: Initializes the wifi library (arm9 side) and the sgIP library.
//  int initflags: set up some optional things, like controlling the LED blinking.
//  Returns: a 32bit value that *must* be passed to arm7.
extern unsigned long Wifi_Init(int initflags);

// Wifi_CheckInit: Verifies when the ARM7 has been successfully initialized
//  Returns: 1 if the arm7 is ready for wifi, 0 otherwise
extern int Wifi_CheckInit();

// Wifi_DisableWifi: Instructs the ARM7 to disengage wireless and stop receiving or
//   transmitting.
extern void Wifi_DisableWifi();

// Wifi_EnableWifi: Instructs the ARM7 to go into a basic "active" mode, not actually
//   associated to an AP, but actively receiving and potentially transmitting
extern void Wifi_EnableWifi();

// Wifi_SetPromiscuousMode: Allows the DS to enter or leave a "promsicuous" mode, in which
//   all data that can be received is forwarded to the arm9 for user processing.
//   Best used with Wifi_RawSetPacketHandler, to allow user code to use the data
//   (well, the lib won't use 'em, so they're just wasting CPU otherwise.)
//  int enable:  0 to disable promiscuous mode, nonzero to engage
extern void Wifi_SetPromiscuousMode(int enable);

// Wifi_ScanMode: Instructs the ARM7 to periodically rotate through the channels to
//   pick up and record information from beacons given off by APs
extern void Wifi_ScanMode();

// Wifi_SetChannel: If the wifi system is not connected or connecting to an access point, instruct
//   the chipset to change channel
//  int channel: the channel to change to, in the range of 1-13
extern void Wifi_SetChannel(int channel);

// Wifi_GetNumAP:
//  Returns: the current number of APs that are known about and tracked internally
extern int Wifi_GetNumAP();

// Wifi_GetAPData: Grabs data from internal structures for user code (always succeeds)
//  int apnum:					the 0-based index of the access point record to fetch
//  Wifi_AccessPoint * apdata:	Pointer to the location where the retrieved data should be stored
extern int Wifi_GetAPData(int apnum, Wifi_AccessPoint * apdata);

// Wifi_FindMatchingAP: determines whether various APs exist in the local area. You provide a
//   list of APs, and it will return the index of the first one in the list that can be found
//   in the internal list of APs that are being tracked
//  int numaps:					number of records in the list
//  Wifi_AccessPoint * apdata:	pointer to an array of structures with information about the APs to find
//  Wifi_AccessPoint * match_dest:	OPTIONAL pointer to a record to receive the matching AP record.
//  Returns:					-1 for none found, or a positive/zero integer index into the array
extern int Wifi_FindMatchingAP(int numaps, Wifi_AccessPoint * apdata, Wifi_AccessPoint * match_dest);

// Wifi_ConnectAP: Connect to an access point
//  Wifi_AccessPoint * apdata:	basic data on the AP
//  int wepmode:				indicates whether wep is used, and what kind
//  int wepkeyid:				indicates which wep key ID to use for transmitting
//  unsigned char * wepkey:		the wep key, to be used in all 4 key slots (should make this more flexible in the future)
//  Returns:					0 for ok, -1 for error with input data
extern int Wifi_ConnectAP(Wifi_AccessPoint * apdata, int wepmode, int wepkeyid, unsigned char * wepkey);

// Wifi_AutoConnect: Connect to an access point specified by the WFC data in the firmware
extern void Wifi_AutoConnect();

// Wifi_AssocStatus: Returns information about the status of connection to an AP
//  Returns: a value from the WIFI_ASSOCSTATUS enum, continue polling until you
//            receive ASSOCSTATUS_CONNECTED or ASSOCSTATUS_CANNOTCONNECT
extern int Wifi_AssocStatus();

// Wifi_DisconnectAP: Disassociate from the Access Point
extern int Wifi_DisconnectAP();

// Wifi_Timer: This function should be called in a periodic interrupt. It serves as the basis
//   for all updating in the sgIP library, all retransmits, timeouts, and etc are based on this
//   function being called.  It's not timing critical but it is rather essential.
//  int num_ms:		The number of milliseconds since the last time this function was called.
extern void Wifi_Timer(int num_ms);

// Wifi_GetIP:
//  Returns:  The current IP address of the DS (may not be valid before connecting to an AP, or setting the IP manually.)
extern unsigned long Wifi_GetIP(); // get local ip

// Wifi_GetIPInfo: (values may not be valid before connecting to an AP, or setting the IP manually.)
//  struct in_addr * pGateway:	pointer to receive the currently configured gateway IP
//  struct in_addr * pSnmask:	pointer to receive the currently configured subnet mask
//  struct in_addr * pDns1:		pointer to receive the currently configured primary DNS server IP
//  struct in_addr * pDns2:		pointer to receive the currently configured secondary DNS server IP
//  Returns:					The current IP address of the DS
extern struct in_addr Wifi_GetIPInfo(struct in_addr * pGateway,struct in_addr * pSnmask,struct in_addr * pDns1,struct in_addr * pDns2);

// Wifi_SetIP: Set the DS's IP address and other IP configuration information.
//  unsigned long IPaddr:		The new IP address (NOTE! if this value is zero, the IP, the gateway, and the subnet mask will be allocated via DHCP)
//  unsigned long gateway:		The new gateway (example: 192.168.1.1 is 0xC0A80101)
//  unsigned long subnetmask:	The new subnet mask (example: 255.255.255.0 is 0xFFFFFF00)
//  unsigned long dns1:			The new primary dns server (NOTE! if this value is zero AND the IPaddr value is zero, dns1 and dns2 will be allocated via DHCP)
//  unsigned long dns2:			The new secondary dns server
extern void Wifi_SetIP(unsigned long IPaddr, unsigned long gateway, unsigned long subnetmask, unsigned long dns1, unsigned long dns2);

// Wifi_GetData: Retrieve an arbitrary or misc. piece of data from the wifi hardware. see WIFIGETDATA enum.
//  int datatype:				element from the WIFIGETDATA enum specifing what kind of data to get
//  int bufferlen:				length of the buffer to copy data to (not always used)
//  unsigned char * buffer:		buffer to copy element data to (not always used)
//  Returns:					-1 for failure, the number of bytes written to the buffer, or the value requested if the buffer isn't used.
extern int Wifi_GetData(int datatype, int bufferlen, unsigned char * buffer);

// Wifi_GetStats: Retreive an element of the wifi statistics gathered
//  int statnum:		Element from the WIFI_STATS enum, indicating what statistic to return
//  Returns:			the requested stat, or 0 for failure
extern u32 Wifi_GetStats(int statnum);
//////////////////////////////////////////////////////////////////////////
// Raw Send/Receive functions

// Wifi_RawTxFrame: Send a raw 802.11 frame at a specified rate
//  unsigned short datalen:	The length in bytes of the frame to send
//  unsigned short rate:	The rate to transmit at (Specified as mbits/10, 1mbit=0x000A, 2mbit=0x0014)
//  unsigned short * data:	Pointer to the data to send (should be halfword-aligned)
//  Returns:				Nothing of interest.
extern int Wifi_RawTxFrame(unsigned short datalen, unsigned short rate, unsigned short * data);

// Wifi_RawSetPacketHandler: Set a handler to process all raw incoming packets
//  WifiPacketHandler wphfunc:  Pointer to packet handler (see WifiPacketHandler definition for more info)
extern void Wifi_RawSetPacketHandler(WifiPacketHandler wphfunc);

// Wifi_RxRawReadPacket:  Allows user code to read a packet from within the WifiPacketHandler function
//  long packetID:			a non-unique identifier which locates the packet specified in the internal buffer
//  long readlength:		number of bytes to read (actually reads (number+1)&~1 bytes)
//  unsigned short * data:	location for the data to be read into
extern int Wifi_RxRawReadPacket(long packetID, long readlength, unsigned short * data);

//////////////////////////////////////////////////////////////////////////
// Fast transfer support - update functions

// Wifi_Update: Checks for new data from the arm7 and initiates routing if data
//   is available.
extern void Wifi_Update();

// Wifi_Sync: Call this function when requested to sync by the arm7 side of the
//   wifi lib
extern void Wifi_Sync();

// Wifi_SetSyncHandler: Call this function to request notification of when the
//   ARM7-side Wifi_Sync function should be called.
//  WifiSyncHandler sh:    Pointer to the function to be called for notification.
extern void Wifi_SetSyncHandler(WifiSyncHandler sh);

#define WFC_CONNECT	true
#define INIT_ONLY	false


extern bool Wifi_InitDefault(bool useFirmwareSettings);

#ifdef __cplusplus
};
#endif


#endif
