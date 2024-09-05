// DS Wifi interface code
// Copyright (C) 2005-2006 Stephen Stair - sgstair@akkit.org - http://www.akkit.org
// wifi_arm7.c - arm7 wifi interface code
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
#include "dsregs.h"
#include "wifi_arm7.h"
#include "wifi_debug.h"
#include "wifi_card.h"

#include "spinlock.h" // .h file with code for spinlocking in it.

//////////////////////////////////////////////////////////////////////////
//
//  Main functionality
//

void Wifi_RFInit() {

}

void Wifi_BBInit() {

}

void Wifi_MacInit() {

}


void Wifi_TxSetup() {

}

void Wifi_RxSetup() {

}


void Wifi_WakeUp() {

}
void Wifi_Shutdown() {

}

//////////////////////////////////////////////////////////////////////////
//
//  MAC Copy functions
//

u16 Wifi_MACRead(u32 MAC_Base, u32 MAC_Offset) {
    return 0;
}

int Wifi_QueueRxMacData(u32 base, u32 len) {

	return 1;
}

int Wifi_CheckTxBuf(s32 offset) {
    return 0;
}

// non-wrapping function.
int Wifi_CopyFirstTxData(s32 macbase) {
    return 0;
}

void Wifi_TxRaw(u16 * data, int datalen) {

}

int Wifi_TxCheck() {
    return 0;
}

void Wifi_LoadBeacon(int from, int to) {

}

void Wifi_SetBeaconChannel(int channel) {

}


//////////////////////////////////////////////////////////////////////////
//
//  Wifi Interrupts
//

void Wifi_Intr_RxEnd() {

}

void Wifi_Intr_CntOverflow() {

}

void Wifi_Intr_TxEnd() { 

}


void Wifi_Intr_DoNothing() {
}



void Wifi_Interrupt() {

}

void Wifi_Update() {

}




//////////////////////////////////////////////////////////////////////////
//
//  Wifi User-called Functions
//

void Wifi_Init(u32 wifidata) {
	
}

void Wifi_Deinit() {
	
}

void Wifi_Start() {
	
}

void Wifi_Stop() {
	
}

void Wifi_SetChannel(int channel) {
	
}
void Wifi_SetWepKey(void * wepkey) {
	
}

void Wifi_SetWepMode(int wepmode) {
	
}

void Wifi_SetBeaconPeriod(int beacon_period) {
	
}

void Wifi_SetMode(int wifimode) {
	
}
void Wifi_SetPreambleType(int preamble_type) {
	
}
void Wifi_DisableTempPowerSave() {
	
}





//////////////////////////////////////////////////////////////////////////
//
//  802.11b system, tied in a bit with the :


int Wifi_TxQueue(u16 * data, int datalen) {

	return 1;
}

int Wifi_GenMgtHeader(u8 * data, u16 headerflags) {

    return 0;
}

int Wifi_SendOpenSystemAuthPacket() {

    return 0;
}

int Wifi_SendSharedKeyAuthPacket() {

    return 0;
}

int Wifi_SendSharedKeyAuthPacket2(int challenge_length, u8 * challenge_Text) {

    return 0;
}


int Wifi_SendAssocPacket() { // uses arm7 data in our struct

    return 0;
}

int Wifi_SendNullFrame() {  // Fix: Either sent ToDS properly or drop ToDS flag. Also fix length (16+4) 

    return 0;
}

int Wifi_SendPSPollFrame() {

    return 0;
}

int Wifi_ProcessReceivedFrame(int macbase, int framelen) {
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// sync functions

void Wifi_Sync() {
	//Wifi_Update();
}

void Wifi_SetSyncHandler(WifiSyncHandler sh) {

}


void installWifiFIFO() {

	wifi_card_init();
}
