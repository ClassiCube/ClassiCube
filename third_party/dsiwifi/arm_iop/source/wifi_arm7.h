// DS Wifi interface code
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
// wifi_arm7.h - arm7 wifi interface header
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

#ifndef WIFI_ARM7_H
#define WIFI_ARM7_H

#ifndef ARM7
#error Wifi is only accessible from the ARM7
#endif


// keepalive updated in the update handler, which should be called in vblank
// keepalive set for 2 minutes.
#define WIFI_KEEPALIVE_COUNT		(60*60*2)


#define WIFI_REG(ofs) (*((volatile u16 *)(0x04800000+(ofs))))
// Wifi regs
#define W_WEPKEY0		(((volatile u16 *)(0x04805F80)))
#define W_WEPKEY1		(((volatile u16 *)(0x04805FA0)))
#define W_WEPKEY2		(((volatile u16 *)(0x04805FC0)))
#define W_WEPKEY3		(((volatile u16 *)(0x04805FE0)))

#define W_MODE_RST		(*((volatile u16 *)(0x04800004)))
#define W_MODE_WEP		(*((volatile u16 *)(0x04800006)))
#define W_IF			(*((volatile u16 *)(0x04800010)))
#define W_IE			(*((volatile u16 *)(0x04800012)))
#define W_MACADDR		(((volatile u16 *)(0x04800018)))
#define W_BSSID			(((volatile u16 *)(0x04800020)))
#define W_AIDS			(*((volatile u16 *)(0x04800028)))
#define W_RETRLIMIT		(*((volatile u16 *)(0x0480002C)))
#define W_POWERSTATE	(*((volatile u16 *)(0x0480003C)))
#define W_RANDOM		(*((volatile u16 *)(0x04800044)))

#define W_BBSIOCNT		(*((volatile u16 *)(0x04800158)))
#define W_BBSIOWRITE	(*((volatile u16 *)(0x0480015A)))
#define W_BBSIOREAD		(*((volatile u16 *)(0x0480015C)))
#define W_BBSIOBUSY		(*((volatile u16 *)(0x0480015E)))
#define W_RFSIODATA2	(*((volatile u16 *)(0x0480017C)))
#define W_RFSIODATA1	(*((volatile u16 *)(0x0480017E)))
#define W_RFSIOBUSY		(*((volatile u16 *)(0x04800180)))






#include "wifi_shared.h"

extern volatile Wifi_MainStruct * WifiData;

// Wifi Sync Handler function: Callback function that is called when the arm9 needs to be told to synchronize with new fifo data.
// If this callback is used (see Wifi_SetSyncHandler()), it should send a message via the fifo to the arm9, which will call Wifi_Sync() on arm9.
typedef void (*WifiSyncHandler)();


#ifdef __cplusplus
extern "C" {
#endif



extern void Read_Flash(int address, char * destination, int length);
extern void InitFlashData();
extern int ReadFlashByte(int address);
extern int ReadFlashHWord(int address);
extern int ReadFlashBytes(int address, int numbytes);

extern int Wifi_BBRead(int a);
extern int Wifi_BBWrite(int a, int b);
extern void Wifi_RFWrite(int writedata);

extern void Wifi_RFInit();
extern void Wifi_BBInit();
extern void Wifi_WakeUp();
extern void Wifi_Shutdown();
extern void Wifi_MacInit();
extern void Wifi_Interrupt();
extern void Wifi_Update();

extern void Wifi_CopyMacAddr(volatile void * dest, volatile void * src);
extern int Wifi_CmpMacAddr(volatile void * mac1, volatile void * mac2);

extern void Wifi_Init(u32 WifiData);
extern void Wifi_Deinit();
extern void Wifi_Start();
extern void Wifi_Stop();
extern void Wifi_SetChannel(int channel);
extern void Wifi_SetWepKey(void * wepkey);
extern void Wifi_SetWepMode(int wepmode);
extern void Wifi_SetBeaconPeriod(int beacon_period);
extern void Wifi_SetMode(int wifimode);
extern void Wifi_SetPreambleType(int preamble_type);
extern void Wifi_TxSetup();
extern void Wifi_RxSetup();
extern void Wifi_DisableTempPowerSave();

extern int Wifi_SendOpenSystemAuthPacket();
extern int Wifi_SendAssocPacket();
extern int Wifi_SendNullFrame();
extern int Wifi_SendPSPollFrame();
extern int Wifi_ProcessReceivedFrame(int macbase, int framelen);

extern void Wifi_Sync();
extern void Wifi_SetSyncHandler(WifiSyncHandler sh);


#ifdef __cplusplus
};
#endif


#endif
