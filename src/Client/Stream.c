#include "Stream.h"

#define Stream_SafeReadBlock(stream, buffer, count, read)\
ReturnCode result = stream->Read(buffer, count, &read);\
if (read == 0 || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "reading from ");\
}

#define Stream_SafeWriteBlock(stream, buffer, count, write)\
ReturnCode result = stream->Write(buffer, count, &write);\
if (write == 0 || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "writing to ");\
}

void Stream_Fail(Stream* stream, ReturnCode result, const UInt8* operation) {
	UInt8 tmpBuffer[String_BufferSize(400)];
	String tmp = String_FromRawBuffer(tmpBuffer, 400);

	String_AppendConstant(&tmp, "Failed ");
	String_AppendConstant(&tmp, operation);
	String_AppendString(&tmp, &stream->Name);
	ErrorHandler_FailWithCode(result, tmpBuffer);
}

void Stream_Read(Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 read;
	while (count > 0) {
		Stream_SafeReadBlock(stream, buffer, count, read);
		count -= read;
	}
}

void Stream_Write(Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 write;
	while (count > 0) {
		Stream_SafeWriteBlock(stream, buffer, count, write);
		count -= write;
	}
}


UInt8 Stream_ReadUInt8(Stream* stream) {
	UInt8 buffer;
	UInt32 read;
	/* Inline 8 bit reading, because it is very frequently used. */
	Stream_SafeReadBlock(stream, &buffer, sizeof(UInt8), read);
	return buffer;
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
	/* infrequently called, so not bothering to optimise this. */
	UInt32 hi = Stream_ReadUInt32_BE(stream);
	UInt32 lo = Stream_ReadUInt32_LE(stream);
	return (UInt64)(((UInt64)hi) << 32) | ((UInt64)lo);
}


void Stream_WriteUInt8(Stream* stream, UInt8 value) {
	UInt32 write;
	/* Inline 8 bit writing, because it is very frequently used. */
	Stream_SafeWriteBlock(stream, &value, sizeof(UInt8), write);
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