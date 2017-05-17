#include "Stream.h"

/*void Stream_Read(Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 read;
	ReturnCode result = 0;

	while (count > 0) {
		ReturnCode result = stream->Read(buffer, count, &read);
		if (read == 0 || !ErrorHandler_Check(result)) {
			error here or something
		}
		count -= read;
	}
}

void Stream_Write(Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 totalWritten = 0;
	while (count > 0) {

	}
}*/

// TODO: This one is called frequently, probably needs inline.
UInt8 Stream_ReadUInt8(Stream* stream) {
	UInt8 result;
	Stream_Read(stream, &result, sizeof(UInt8));
	return result;
}

UInt16 Stream_ReadUInt16_LE(Stream* stream) {
	UInt8 buffer[sizeof(UInt16)];
	Stream_Read(stream, buffer, sizeof(UInt16));
	return (UInt16)(buffer[0] | (buffer[1] << 8));
}

UInt16 Stream_ReadUInt16_BE(Stream* stream) {
	UInt8 buffer[sizeof(UInt16)];
	Stream_Read(stream, buffer, sizeof(UInt16));
	return (UInt16)((buffer[0] << 8) | buffer[1]);
}

UInt32 Stream_ReadUInt32_LE(Stream* stream) {
	UInt8 buffer[sizeof(UInt32)];
	Stream_Read(stream, buffer, sizeof(UInt32));
	return (UInt32)(
		(UInt32)buffer[0]         | ((UInt32)buffer[1] << 8) | 
		((UInt32)buffer[2] << 16) | ((UInt32)buffer[3] << 24));
}

UInt32 Stream_ReadUInt32_BE(Stream* stream) {
	UInt8 buffer[sizeof(UInt32)];
	Stream_Read(stream, buffer, sizeof(UInt32));
	return (UInt32)(
		((UInt32)buffer[0] << 24) | ((UInt32)buffer[1] << 16) |
		((UInt32)buffer[2] << 8)  | (UInt32)buffer[3]);
}

UInt64 Stream_ReadUInt64_BE(Stream* stream) {
	UInt8 buffer[sizeof(UInt64)];
	Stream_Read(stream, buffer, sizeof(UInt64));
	UInt32 hi = (UInt32)(
		((UInt32)buffer[0] << 24) | ((UInt32)buffer[1] << 16) |
		((UInt32)buffer[2] << 8)  | (UInt32)buffer[3]);
	UInt32 lo = (UInt32)(
		((UInt32)buffer[4] << 24) | ((UInt32)buffer[5] << 16) |
		((UInt32)buffer[6] << 8)  | (UInt32)buffer[7]);
	return (UInt64)(((UInt64)hi) << 32) | ((UInt64)lo);
}


void Stream_WriteUInt16_LE(Stream* stream, UInt16 value) {
	UInt8 buffer[sizeof(UInt16)];
	buffer[0] = (UInt8)(value      ); buffer[1] = (UInt8)(value >> 8 );
	Stream_Write(stream, buffer, sizeof(UInt16));
}

void Stream_WriteUInt16_BE(Stream* stream, UInt16 value) {
	UInt8 buffer[sizeof(UInt16)];
	buffer[0] = (UInt8)(value >> 8 ); buffer[1] = (UInt8)(value      );
	Stream_Write(stream, buffer, sizeof(UInt16));
}

void Stream_WriteUInt32_LE(Stream* stream, UInt32 value) {
	UInt8 buffer[sizeof(UInt32)];
	buffer[0] = (UInt8)(value      ); buffer[1] = (UInt8)(value >> 8 );
	buffer[2] = (UInt8)(value >> 16); buffer[3] = (UInt8)(value >> 24);
	Stream_Write(stream, buffer, sizeof(UInt32));
}

void Stream_WriteUInt32_BE(Stream* stream, UInt32 value) {
	UInt8 buffer[sizeof(UInt32)];
	buffer[0] = (UInt8)(value >> 24); buffer[1] = (UInt8)(value >> 16);
	buffer[2] = (UInt8)(value >> 8 ); buffer[3] = (UInt8)(value);
	Stream_Write(stream, buffer, sizeof(UInt32));
}