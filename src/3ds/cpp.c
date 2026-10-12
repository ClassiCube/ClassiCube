//code from https://github.com/skyfloogle/red-viper 

// This file handles communication with the Circle Pad Pro.
// For interactions with the calibration applet, see extrapad.c.

#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/ipc.h>
#include <3ds/synchronization.h>
#include <malloc.h>
#include <stdio.h>

#include "cpp.h"
#include "3ds/services/apt.h"
#include "3ds/services/hid.h"
#include "3ds/thread.h"

#define MILLIS 1000000

typedef struct {
    u32 latest_receive_error_result;
    u32 latest_send_error_result;
    u8 connection_status;
    u8 trying_to_connect_status;
    u8 connection_role;
    u8 machine_id;
    u8 connected;
    u8 network_id;
    u8 initialized;
} SharedMemoryHeader;

typedef struct {
    u32 begin_index;
    u32 end_index;
    u32 packet_count;
    u32 unknown;
} BufferInfo;

typedef struct {
    u32 offset;
    u32 size;
} PacketInfo;

typedef struct {
    SharedMemoryHeader header;
    BufferInfo recvBufferInfo;
    PacketInfo recvPackets[];
} SharedMemory;

typedef struct {
    u8 head;
    u8 response_time;
    u8 unknown;
} InputRequest;

typedef struct __attribute__((packed)) {
    u8 head;
	u16 pad_x: 12;
	u16 pad_y: 12;
    u8 battery_level: 5;
    bool zl_up: 1;
    bool zr_up: 1;
    bool r_up: 1;
    u8 unknown;
} InputResponse;

typedef struct {
	u8 head;
	u8 response_time;
	u16 offset;
	u16 size;
} CalibrationRequest;

typedef struct {
	u16 x_offset;
	u16 y_offset;
	float x_scale;
	float y_scale;
} CalibrationData;

typedef struct __attribute__((packed)) {
	u8 _padding1;
	u16 x_offset: 12;
	u16 y_offset: 12;
	float x_scale;
	float y_scale;
	u8 _padding2[3];
	u8 crc8;
} CalibrationResponseData;

typedef struct __attribute__((packed)) {
	u8 head;
	u16 offset;
	u16 size;
	CalibrationResponseData candidates[4];
} CalibrationResponse;

static Handle iruserHandle;

static SharedMemory *iruserSharedMemAddr;
static u32 iruserSharedMemSize;
static Handle iruserSharedMemHandle;

static Thread iruserThread;
static Handle iruserExitEvent;

static volatile u32 kHeld = 0;
static volatile circlePosition cPos = {0, 0};
static volatile u8 batteryLevel = 0;

static int iruserRefCount = 0;

static int iruserPacketId;
static int iruserRecvPacketCapacity;
static int iruserRecvBuffSize;
static int iruserSendPacketCapacity;
static int iruserSendBuffSize;
static u8 iruserBaudRate;
static volatile bool iruserConnected = false;

static Result __IRUSER_FinalizeIrnop(void);
static Result __IRUSER_ClearReceiveBuffer(void);
static Result __IRUSER_ClearSendBuffer(void);
static Result __IRUSER_RequireConnection(u8 deviceId);
static Result __IRUSER_Disconnect(void);
static Result __IRUSER_GetReceiveEvent(Handle *out);
static Result __IRUSER_GetConnectionStatusEvent(Handle *out);
static Result __IRUSER_SendIrnop(const void *buffer, u32 size);
static Result __IRUSER_InitializeIrnopShared
    (Handle shared_memory,size_t shared_buff_size,size_t recv_buff_size,
    size_t recv_buff_packet_count,size_t send_buff_size,size_t send_buff_packet_count,
    uint8_t baud_rate);
static Result __IRUSER_ReleaseSharedData(u32 count);

static void iruserThreadFunc(void*);
static u8 crc8(const void *buf, size_t len);
static u8 crc8_loop(const void *container_start, size_t container_size, const void *segment, size_t segment_size);
static u32 iruserReadPacket(void *out, u32 out_len);
static Result iruserClearPacket(int count);

Result cppInit(void)
{
    bool new_3ds;
    APT_CheckNew3DS(&new_3ds);
    if (new_3ds) return 0;

    Result ret = 0;

    if (AtomicPostIncrement(&iruserRefCount)) return -1;

    iruserSharedMemSize = 0x1000;

    size_t recv_data_size = 2000;
    size_t recv_packet_count = 0xa0;
    size_t send_data_size = 0x200;
    size_t send_packet_count = 0x20;
    u8 baud_rate = 4;
    size_t recv_buff_size = recv_data_size + recv_packet_count * 8;
    size_t send_buff_size = send_data_size + send_packet_count * 8;

    iruserRecvPacketCapacity = recv_packet_count;
    iruserRecvBuffSize = recv_buff_size;
    iruserSendPacketCapacity = send_packet_count;
    iruserSendBuffSize = send_buff_size;
    iruserBaudRate = baud_rate;
    iruserPacketId = 0;

    iruserSharedMemAddr = memalign(0x1000, iruserSharedMemSize);
    if (iruserSharedMemAddr == NULL)
    {
        ret = -1;
        goto cleanup0;
    }

    ret = srvGetServiceHandle(&iruserHandle, "ir:USER");
    if(R_FAILED(ret))goto cleanup1;

    ret = svcCreateMemoryBlock(&iruserSharedMemHandle, (u32)iruserSharedMemAddr, iruserSharedMemSize, MEMPERM_READ, MEMPERM_READWRITE);
    if (R_FAILED(ret))goto cleanup2;

    ret = svcCreateEvent(&iruserExitEvent, RESET_ONESHOT);
    if(R_FAILED(ret)) goto cleanup3;

    iruserThread = threadCreate(iruserThreadFunc, NULL, 0x200, 0x28, 0, true);
    if (iruserThread == NULL) {
        ret = -1;
        goto cleanup4;
    }

    return 0;

    cleanup4:
    svcCloseHandle(iruserExitEvent);
    cleanup3:
    svcCloseHandle(iruserSharedMemHandle);
    cleanup2:
    svcCloseHandle(iruserHandle);
    cleanup1:
    free(iruserSharedMemAddr);
    iruserSharedMemAddr = NULL;
    cleanup0:
    AtomicDecrement(&iruserRefCount);
    return ret;
}

void cppExit()
{
    bool new_3ds;
    APT_CheckNew3DS(&new_3ds);
    if (new_3ds) return;

    if (AtomicDecrement(&iruserRefCount)) return;

    svcSignalEvent(iruserExitEvent);
    threadJoin(iruserThread, 20 * MILLIS);
}

bool cppGetConnected(void) {
    return iruserConnected;
}

void cppCircleRead(circlePosition *pos) {
    if (pos) *pos = cPos;
}

u32 cppKeysHeld(void) {
    return kHeld;
}

u8 cppBatteryLevel(void) {
    return batteryLevel;
}

enum {
    SAR_EXIT = -1,
    SAR_READERROR = -2,
    SAR_TIMEOUT = -3,
};

static Result sendAndReceive(const void *request, int request_size, void *response, int response_size, Handle *two_events, int64_t timeout, u8 expected_head) {
    Result subres, res;
    for (int i = 0; i < 4; i++) {
        __IRUSER_SendIrnop(request, request_size);
        s32 synced_handle;
        subres = svcWaitSynchronizationN(&synced_handle, two_events, 2, false, timeout);
        if (R_DESCRIPTION(subres) == RD_TIMEOUT)  {
            // Timeout.
            res = SAR_TIMEOUT;
            continue;
        }
        if (synced_handle == 1) {
            // Exit.
            res = SAR_EXIT;
            break;
        }
        subres = iruserReadPacket(response, response_size);
        if (subres != response_size || *(u8*)response != expected_head) {
            // Read error.
            res = SAR_READERROR;
            continue;
        }
        return 0;
    }
    // Timeout.
    return res;
}

static bool checkCalibrationData(CalibrationData *out, CalibrationResponseData *candidate) {
    if (crc8(candidate, 15) == candidate->crc8) {
        out->x_offset = candidate->x_offset;
        out->y_offset = candidate->y_offset;
        out->x_scale = candidate->x_scale;
        out->y_scale = candidate->y_scale;
        return true;
    }
    return false;
}

static void iruserDisconnect(void) {
    iruserConnected = false;
    __IRUSER_Disconnect();
    __IRUSER_ClearReceiveBuffer();
    __IRUSER_ClearSendBuffer();
    __IRUSER_FinalizeIrnop();
    batteryLevel = 0;
    kHeld = 0;
    cPos.dx = 0;
    cPos.dy = 0;
}

static void iruserThreadFunc(void* param) {
    Result ret;

    Handle two_events[2];
    two_events[1] = iruserExitEvent;
    s32 synced_handle;
    Handle connstatus_event = 0, recv_event = 0;

    while (true) {
        CalibrationData calibration_data;
        // Wait for CPP connection.
        while (true) {
            __IRUSER_InitializeIrnopShared(iruserSharedMemHandle, iruserSharedMemSize, iruserRecvBuffSize, iruserRecvPacketCapacity, iruserSendBuffSize, iruserSendPacketCapacity, iruserBaudRate);
            __IRUSER_GetConnectionStatusEvent(&connstatus_event);
            __IRUSER_GetReceiveEvent(&recv_event);
            two_events[0] = connstatus_event;
            // Try four times in quick succession, then wait a second.
            for (int i = 0; i < 4; i++) {
                __IRUSER_RequireConnection(1);
                ret = svcWaitSynchronizationN(&synced_handle, two_events, 2, false, 14 * MILLIS);
                if (R_DESCRIPTION(ret) != RD_TIMEOUT) {
                    if (synced_handle == 0) goto connected;
                    else goto cleanup;
                }
                __IRUSER_Disconnect();
            }
            svcCloseHandle(connstatus_event);
            svcCloseHandle(recv_event);
            iruserDisconnect();
            svcWaitSynchronization(iruserExitEvent, 1000 * MILLIS);
        }
        connected:
        // Retrieve calibration data.
        CalibrationRequest calibration_request = {2, 100, 0, 0x40};
        CalibrationResponse calibration_response;
        two_events[0] = recv_event;
        ret = sendAndReceive(&calibration_request, sizeof(calibration_request), &calibration_response, sizeof(calibration_response), two_events, 20 * MILLIS, 0x11);
        if (ret == SAR_EXIT) goto cleanup;
        if (ret == SAR_READERROR) goto disconnect;
        if (ret == SAR_TIMEOUT) goto disconnect;

        bool calibration_data_found = false;
        for (int i = 0; i < 4; i++) {
            if (checkCalibrationData(&calibration_data, &calibration_response.candidates[i])) {
                calibration_data_found = true;
                break;
            }
        }

        if (!calibration_data_found) {
            for (int i = 0; i < 0x40; i++) {
                calibration_request.offset = 0x400 + i;
                ret = sendAndReceive(&calibration_request, sizeof(calibration_request), &calibration_response, sizeof(calibration_response), two_events, 20 * MILLIS, 0x11);
                if (ret == SAR_EXIT) goto cleanup;
                if (ret == SAR_READERROR) goto disconnect;
                if (ret == SAR_TIMEOUT) goto disconnect;
                for (int j = 0; j < 4; j++) {
                    if (checkCalibrationData(&calibration_data, &calibration_response.candidates[j])) {
                        calibration_data_found = true;
                        break;
                    }
                }
            }
        }

        if (!calibration_data_found) goto disconnect;

        iruserConnected = true;

        while (iruserSharedMemAddr->header.connection_status == 2) {
            InputRequest input_request = {0x01, 0x20, 0x87};
            InputResponse input_response;
            ret = sendAndReceive(&input_request, sizeof(input_request), &input_response, sizeof(input_response), two_events, 20 * MILLIS, 0x10);
            if (ret == SAR_EXIT) goto cleanup;
            if (ret == SAR_TIMEOUT) goto disconnect;
            if (ret == SAR_READERROR) continue; // Ignore read errors.
            if (input_response.head != 0x10) continue;

            cPos.dx = (s16)((float)(s16)(input_response.pad_x - calibration_data.x_offset) * calibration_data.x_scale) / 8;
            cPos.dy = (s16)((float)(s16)(input_response.pad_y - calibration_data.y_offset) * calibration_data.y_scale) / 8;
            
            u32 keys = 0;
            if (!input_response.r_up) keys |= KEY_R;
            if (!input_response.zl_up) keys |= KEY_ZL;
            if (!input_response.zr_up) keys |= KEY_ZR;
            kHeld = keys;

            batteryLevel = input_response.battery_level;
        }
        disconnect:
        svcCloseHandle(connstatus_event);
        svcCloseHandle(recv_event);
        iruserDisconnect();
    }
    cleanup:
    svcCloseHandle(connstatus_event);
    svcCloseHandle(recv_event);
    iruserDisconnect();
    svcCloseHandle(iruserSharedMemHandle);
    svcCloseHandle(iruserHandle);

    free(iruserSharedMemAddr);
    iruserSharedMemAddr = NULL;
    iruserSharedMemSize = 0;
}

static u32 iruserReadPacket(void *out, u32 out_len)
{
    if (iruserSharedMemAddr->recvBufferInfo.packet_count <= 0) {
        return 0;
    }
    PacketInfo *packetInfo = &iruserSharedMemAddr->recvPackets[iruserPacketId];
    u8 *start = ((u8*)&iruserSharedMemAddr->recvPackets[iruserRecvPacketCapacity]);
    u8 *end = start + iruserRecvBuffSize;

    u8 *cursor = start + packetInfo->offset;

    // The last byte is the CRC8 of what came before, so the CRC8 of the whole thing should be 0.
    if (crc8_loop(start, iruserRecvBuffSize, cursor, packetInfo->size)) {
        // Invalid packet, ignore.
        iruserClearPacket(1);
        return 0;
    }

    u8 header[4];
    for (int i = 0; i < 4; i++) {
        header[i] = *cursor++;
        if (cursor == end) cursor = start;
    }

    int content_length = header[2] & 0x3f;
    if (header[2] & 0x40) {
        content_length = (content_length << 8) | header[3];
    } else {
        // Backtrack one byte.
        if (cursor == start) cursor = end;
        cursor--;
    }
    
    int len_to_read = content_length <= out_len ? content_length : out_len;

    for (int i = 0; i < len_to_read; i++) {
        *(u8*)out++ = *cursor++;
        if (cursor == end) cursor = start;
    }

    if (content_length <= out_len) iruserClearPacket(1);

    return content_length;
}

static Result iruserClearPacket(int count) {
    Result ret = __IRUSER_ReleaseSharedData(count);
    if (R_FAILED(ret)) return ret;
    iruserPacketId += count;
    while (iruserPacketId >= iruserRecvPacketCapacity) {
        iruserPacketId -= iruserRecvPacketCapacity;
    }
    return 0;
}

static Result __IRUSER_FinalizeIrnop(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x2,0,0); // 0x20000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_ClearReceiveBuffer(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x3,0,0); // 0x30000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_ClearSendBuffer(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x4,0,0); // 0x40000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_RequireConnection(u8 deviceId)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x6,1,0); // 0x60040
    cmdbuf[1] = deviceId;
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_Disconnect(void)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x9,0,0); // 0x90000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_GetReceiveEvent(Handle *out)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0xA,0,0); // 0xA0000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    *out = (Handle)cmdbuf[3];

    return ret;
}

static Result __IRUSER_GetConnectionStatusEvent(Handle *out)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0xC,0,0); // 0xC0000
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];
    
    *out = (Handle)cmdbuf[3];

    return ret;
}

static Result __IRUSER_SendIrnop(const void *buffer, u32 size)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0xD,1,2); // 0xD0042
    cmdbuf[1] = size;
    cmdbuf[2] = (size << 14) | 2;
    cmdbuf[3] = (u32)buffer;
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_InitializeIrnopShared
    (Handle shared_memory,size_t shared_buff_size,size_t recv_buff_size,
    size_t recv_buff_packet_count,size_t send_buff_size,size_t send_buff_packet_count,
    uint8_t baud_rate)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();
    
    cmdbuf[0] = IPC_MakeHeader(0x18,6,2); // 0x180182
    cmdbuf[1] = shared_buff_size;
    cmdbuf[2] = recv_buff_size;
    cmdbuf[3] = recv_buff_packet_count;
    cmdbuf[4] = send_buff_size;
    cmdbuf[5] = send_buff_packet_count;
    cmdbuf[6] = baud_rate;
    cmdbuf[7] = 0;
    cmdbuf[8] = shared_memory;

    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

static Result __IRUSER_ReleaseSharedData(u32 count)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x19,1,0); // 0x190040
    cmdbuf[1] = count;
    
    if(R_FAILED(ret = svcSendSyncRequest(iruserHandle)))return ret;
    ret = (Result)cmdbuf[1];

    return ret;
}

const static u8 crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3,
};

static u8 crc8(const void *buf, size_t len) {
	u8 res = 0;
	for (int i = 0; i < len; i++) {
		res = crc8_table[res ^ ((u8*)buf)[i]];
	}
	return res;
}

static u8 crc8_loop(const void *container_start, size_t container_size, const void *segment, size_t segment_size) {
	u8 res = 0;
	for (int i = 0; i < segment_size; i++) {
		res = crc8_table[res ^ *(u8*)segment++];
        if (segment == container_start + container_size)
            segment = container_start;
	}
	return res;
}
