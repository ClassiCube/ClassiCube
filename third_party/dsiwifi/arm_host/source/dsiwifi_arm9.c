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
#include "dsiwifi9.h"

#include <nds.h>
#include "dsregs.h"

#include "dswifi9.h"
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <nds/arm9/sassert.h>

#include "wifi_host.h"

#ifdef WIFI_USE_TCP_SGIP

#include "nds/sgIP.h"

//////////////////////////////////////////////////////////////////////////

#endif

WifiLogHandler DSiWifi_pfnLogHandler = NULL;
WifiConnectHandler DSiWifi_pfnConnectHandler = NULL;
WifiReconnectHandler DSiWifi_pfnReconnectHandler = NULL;

u32 DSiWifi_TxBufferWordsAvailable() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}
void DSiWifi_TxBufferWrite(s32 start, s32 len, u16 * data) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

int DSiWifi_RxRawReadPacket(s32 packetID, s32 readlength, u16 * data) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

u16 DSiWifi_RxReadOffset(s32 base, s32 offset) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

int DSiWifi_RawTxFrame(u16 datalen, u16 rate, u16 * data) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return 0;
}


void DSiWifi_RawSetPacketHandler(WifiPacketHandler wphfunc) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}
void DSiWifi_SetSyncHandler(WifiSyncHandler wshfunc) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

void DSiWifi_DisableWifi() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}
void DSiWifi_EnableWifi() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}
void DSiWifi_SetPromiscuousMode(int enable) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

void DSiWifi_ScanMode() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}
void DSiWifi_SetChannel(int channel) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}


int DSiWifi_GetNumAP() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

int DSiWifi_GetAPData(int apnum, Wifi_AccessPoint * apdata) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return 0;
}

int DSiWifi_FindMatchingAP(int numaps, Wifi_AccessPoint * apdata, Wifi_AccessPoint * match_dest) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

int DSiWifi_ConnectAP(Wifi_AccessPoint * apdata, int wepmode, int wepkeyid, u8 * wepkey) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

void DSiWifi_AutoConnect() {
	//sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

int DSiWifi_AssocStatus() {
    
	//sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return wifi_host_get_status();
}


int DSiWifi_DisconnectAP() {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}


#ifdef WIFI_USE_TCP_SGIP



int DSiWifi_TransmitFunction(sgIP_Hub_HWInterface * hw, sgIP_memblock * mb) {
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}

int DSiWifi_Interface_Init(sgIP_Hub_HWInterface * hw) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return 0;
}

void DSiWifi_Timer(int num_ms) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

#endif

unsigned long DSiWifi_Init(int initflags) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return 0;
}

int DSiWifi_CheckInit() {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
	return 0;
}


void DSiWifi_Update() {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

u32 DSiWifi_GetIP() {
	return wifi_host_get_ipaddr();
}

struct in_addr  DSiWifi_GetIPInfo(struct in_addr * pGateway,struct in_addr * pSnmask,struct in_addr * pDns1,struct in_addr * pDns2) {
    //sassert(false, "%s\nis currently unimplemented", __FUNCTION__);

	struct in_addr ip = { wifi_host_get_ipaddr() };
	return ip;
}


//////////////////////////////////////////////////////////////////////////
// Ip addr get/set functions
#ifdef WIFI_USE_TCP_SGIP


void DSiWifi_SetIP(u32 IPaddr, u32 gateway, u32 subnetmask, u32 dns1, u32 dns2) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

void DSiWifi_SetDHCP() {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}

#endif


int DSiWifi_GetData(int datatype, int bufferlen, unsigned char * buffer) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return -1;
}

u32 DSiWifi_GetStats(int statnum) {
    sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
    return 0;
}


//---------------------------------------------------------------------------------
// sync functions

//---------------------------------------------------------------------------------
void DSiWifi_Sync() {
//---------------------------------------------------------------------------------
	sassert(false, "%s\nis currently unimplemented", __FUNCTION__);
}


//---------------------------------------------------------------------------------
bool DSiWifi_InitDefault(bool useFirmwareSettings) {
//---------------------------------------------------------------------------------
    // TODO Wifi_InitDefault
    wifi_host_init();
    return true;
}

void DSiWifi_SetLogHandler(WifiLogHandler handler)
{
    DSiWifi_pfnLogHandler = handler;
}

void DSiWifi_SetConnectHandler(WifiConnectHandler handler)
{
    DSiWifi_pfnConnectHandler = handler;
}

void DSiWifi_SetReconnectHandler(WifiReconnectHandler handler)
{
    DSiWifi_pfnReconnectHandler = handler;
}

void DSiWifi_Shutdown(void)
{
    
}
