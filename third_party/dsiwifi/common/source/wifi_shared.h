// DS Wifi interface code
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
// wifi_shared.h - Shared structures to be used by arm9 and arm7
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

#ifndef WIFI_SHARED_H
#define WIFI_SHARED_H


#include <nds.h>


#define WIFIINIT_OPTION_USELED		   0x0002

// on spinlock contention, the side unsuccessfully attempting the lock reverts the lock.
// if the unlocking side sees the lock incorrectly set, the unlocking side will delay until it has reverted to the correct value, then continue unlocking.
// there should be a delay of at least about ~10-20 cycles between a lock and unlock, to prevent contention.
#define SPINLOCK_NOBODY			0x0000	
#define SPINLOCK_ARM7			0x0001
#define SPINLOCK_ARM9			0x0002

#define SPINLOCK_OK		0x0000
#define SPINLOCK_INUSE	0x0001
#define SPINLOCK_ERROR	0x0002

#ifdef ARM7
#define SPINLOCK_VALUE SPINLOCK_ARM7
#endif
#ifdef ARM9
#define SPINLOCK_VALUE SPINLOCK_ARM9
#endif


#define Spinlock_Acquire(structtolock) SLasm_Acquire(&((structtolock).spinlock),SPINLOCK_VALUE)
#define Spinlock_Release(structtolock) SLasm_Release(&((structtolock).spinlock),SPINLOCK_VALUE)
#define Spinlock_Check(structtolock) (((structtolock).spinlock)!=SPINLOCK_NOBODY)

#ifdef __cplusplus
extern "C" {
#endif

extern u32 SLasm_Acquire(volatile u32 * lockaddr, u32 lockvalue);
extern u32 SLasm_Release(volatile u32 * lockaddr, u32 lockvalue);

#ifdef __cplusplus
};
#endif

// If for whatever reason you want to ditch SGIP and use your own stack, comment out the following line.
#define WIFI_USE_TCP_SGIP	0

#define WIFI_RXBUFFER_SIZE	(1024*12)
#define WIFI_TXBUFFER_SIZE	(1024*24)
#define WIFI_MAX_AP			32
#define WIFI_MAX_ASSOC_RETRY	30
#define WIFI_PS_POLL_CONST  2

#define WIFI_MAX_PROBE		4

#define WIFI_AP_TIMEOUT    40

#define WFLAG_PACKET_DATA		0x0001
#define WFLAG_PACKET_MGT		0x0002
#define WFLAG_PACKET_BEACON		0x0004
#define WFLAG_PACKET_CTRL		0x0008


#define WFLAG_PACKET_ALL		0xFFFF

#define WFLAG_ARM7_ACTIVE		0x0001
#define WFLAG_ARM7_RUNNING		0x0002

#define WFLAG_ARM9_ACTIVE		0x0001
#define WFLAG_ARM9_USELED		0x0002
#define WFLAG_ARM9_ARM7READY	0x0004
#define WFLAG_ARM9_NETUP		0x0008
#define WFLAG_ARM9_NETREADY		0x0010

#define WFLAG_ARM9_INITFLAGMASK	0x0002

#define WFLAG_IP_GOTDHCP		0x0001

// request - request flags
#define WFLAG_REQ_APCONNECT			0x0001
#define WFLAG_REQ_APCOPYVALUES		0x0002
#define WFLAG_REQ_APADHOC			0x0008
#define WFLAG_REQ_PROMISC			0x0010
#define WFLAG_REQ_USEWEP			0x0020

// request - informational flags
#define WFLAG_REQ_APCONNECTED		0x8000

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

enum WEPMODES {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
};

enum WIFI_ASSOCSTATUS {
    ASSOCSTATUS_DISCONNECTED, // not *trying* to connect
    ASSOCSTATUS_SEARCHING, // data given does not completely specify an AP, looking for AP that matches the data.
    ASSOCSTATUS_AUTHENTICATING, // connecting...
    ASSOCSTATUS_ASSOCIATING, // connecting...
    ASSOCSTATUS_ACQUIRINGDHCP, // connected to AP, but getting IP data from DHCP
    ASSOCSTATUS_ASSOCIATED,	// Connected! (COMPLETE if Wifi_ConnectAP was called to start)
    ASSOCSTATUS_CANNOTCONNECT, // error in connecting... (COMPLETE if Wifi_ConnectAP was called to start)
};

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

typedef struct WIFI_ACCESSPOINT {
	char ssid[33]; // 0-32byte data, zero
	char ssid_len;
	u8 bssid[6];
	u8 macaddr[6];
	u16 maxrate; // max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B
	u32 timectr;
	u16 rssi;
	u16 flags;
	u32 spinlock;
	u8 channel;
	u8 rssi_past[8];
	u8 base_rates[16]; // terminated by a 0 entry
} Wifi_AccessPoint;

typedef struct WIFI_MAINSTRUCT {
	unsigned long dummy1[8];
	// wifi status
	u16 curChannel, reqChannel;
	u16 curMode, reqMode;
	u16 authlevel,authctr;
	vu32 flags9, flags7;
	u32 reqPacketFlags;
	u16 curReqFlags, reqReqFlags;
	u32 counter7,bootcounter7;
	u16 MacAddr[3];
	u16 authtype;
	u16 iptype,ipflags;
	u32 ip,snmask,gateway;

	// current AP data
	char ssid7[34],ssid9[34];
	u16 bssid7[3], bssid9[3];
	u8 apmac7[6], apmac9[6];
	char wepmode7, wepmode9;
	char wepkeyid7, wepkeyid9;
	u8 wepkey7[20],wepkey9[20];
	u8 baserates7[16], baserates9[16];
	u8 apchannel7, apchannel9;
	u8 maxrate7;
	u16 ap_rssi;
	u16 pspoll_period;

	// AP data
	Wifi_AccessPoint aplist[WIFI_MAX_AP];

	// probe stuff
	u8 probe9_numprobe;
	u8 probe9_ssidlen[WIFI_MAX_PROBE];
	char probe9_ssid[WIFI_MAX_PROBE][32];

	// WFC data
	u8 wfc_enable[4]; // wep mode, or 0x80 for "enabled"
	Wifi_AccessPoint wfc_ap[3];
	unsigned long wfc_config[3][5]; // ip, gateway, snmask, primarydns, 2nddns
	u8 wfc_wepkey[3][16];
	

	// wifi data
	u32 rxbufIn, rxbufOut; // bufIn/bufOut have 2-byte granularity.
	u16 rxbufData[WIFI_RXBUFFER_SIZE/2]; // send raw 802.11 data through! rxbuffer is for rx'd data, arm7->arm9 transfer

	u32 txbufIn, txbufOut;
	u16 txbufData[WIFI_TXBUFFER_SIZE/2]; // tx buffer is for data to tx, arm9->arm7 transfer

	// stats data
	u32 stats[NUM_WIFI_STATS];
   
	u16 debug[30];

   u32 random; // semirandom number updated at the convenience of the arm7. use for initial seeds & such.

	unsigned long dummy2[8];

} Wifi_MainStruct;



#endif

