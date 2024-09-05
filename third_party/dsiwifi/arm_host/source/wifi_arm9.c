// DS Wifi interface code
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
// wifi_arm9.c - arm9 wifi support code
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


#include <nds.h>
#include <nds/arm9/console.h>
#include "dsregs.h"

#include "dswifi9.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <nds/arm9/sassert.h>

#ifdef WIFI_USE_TCP_SGIP

#include "nds/sgIP.h"

//////////////////////////////////////////////////////////////////////////

#endif

u32 __attribute__((weak)) Wifi_TxBufferWordsAvailable() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}
void __attribute__((weak)) Wifi_TxBufferWrite(s32 start, s32 len, u16 * data) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

int __attribute__((weak)) Wifi_RxRawReadPacket(s32 packetID, s32 readlength, u16 * data) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

u16 __attribute__((weak)) Wifi_RxReadOffset(s32 base, s32 offset) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

int __attribute__((weak)) Wifi_RawTxFrame(u16 datalen, u16 rate, u16 * data) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return 0;
}


void Wifi_RawSetPacketHandler(WifiPacketHandler wphfunc) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}
void Wifi_SetSyncHandler(WifiSyncHandler wshfunc) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

void __attribute__((weak)) Wifi_DisableWifi() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}
void __attribute__((weak)) Wifi_EnableWifi() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}
void __attribute__((weak)) Wifi_SetPromiscuousMode(int enable) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

void __attribute__((weak)) Wifi_ScanMode() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}
void __attribute__((weak)) Wifi_SetChannel(int channel) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}


int __attribute__((weak)) Wifi_GetNumAP() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

int __attribute__((weak)) Wifi_GetAPData(int apnum, Wifi_AccessPoint * apdata) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return 0;
}

int __attribute__((weak)) Wifi_FindMatchingAP(int numaps, Wifi_AccessPoint * apdata, Wifi_AccessPoint * match_dest) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

int __attribute__((weak)) Wifi_ConnectAP(Wifi_AccessPoint * apdata, int wepmode, int wepkeyid, u8 * wepkey) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}
void __attribute__((weak)) Wifi_AutoConnect() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

int __attribute__((weak)) Wifi_AssocStatus() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}


int __attribute__((weak)) Wifi_DisconnectAP() {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}


#ifdef WIFI_USE_TCP_SGIP



int __attribute__((weak)) Wifi_TransmitFunction(sgIP_Hub_HWInterface * hw, sgIP_memblock * mb) {
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

int __attribute__((weak)) Wifi_Interface_Init(sgIP_Hub_HWInterface * hw) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return 0;
}

void __attribute__((weak)) Wifi_Timer(int num_ms) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

#endif

unsigned long __attribute__((weak)) Wifi_Init(int initflags) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return 0;
}

int __attribute__((weak)) Wifi_CheckInit() {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}


void __attribute__((weak)) Wifi_Update() {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}


//////////////////////////////////////////////////////////////////////////
// Ip addr get/set functions
#ifdef WIFI_USE_TCP_SGIP

u32 __attribute__((weak)) Wifi_GetIP() {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
	return 0;
}

struct in_addr __attribute__((weak))  Wifi_GetIPInfo(struct in_addr * pGateway,struct in_addr * pSnmask,struct in_addr * pDns1,struct in_addr * pDns2) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);

	struct in_addr ip = { INADDR_NONE };
	return ip;
}


void __attribute__((weak)) Wifi_SetIP(u32 IPaddr, u32 gateway, u32 subnetmask, u32 dns1, u32 dns2) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

void __attribute__((weak)) Wifi_SetDHCP() {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}

#endif


int __attribute__((weak)) Wifi_GetData(int datatype, int bufferlen, unsigned char * buffer) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return -1;
}

u32 __attribute__((weak)) Wifi_GetStats(int statnum) {
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return 0;
}


//---------------------------------------------------------------------------------
// sync functions

//---------------------------------------------------------------------------------
void __attribute__((weak)) Wifi_Sync() {
//---------------------------------------------------------------------------------
	sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
}


//---------------------------------------------------------------------------------
bool Wifi_InitDefault(bool useFirmwareSettings) {
//---------------------------------------------------------------------------------
    sassert(false, "Legacy WiFi requires\n`-ldswifi`!\nOtherwise, use\n`DSi%s`\nfor DSi-only or hybrid", __FUNCTION__);
    return false;
}

