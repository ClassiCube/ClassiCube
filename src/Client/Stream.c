#include "Stream.h"
#include "Platform.h"
#include "Funcs.h"

#define Stream_SafeReadBlock(stream, buffer, count, read)\
ReturnCode result = stream->Read(stream, buffer, count, &read);\
if (read == 0 || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "reading from ");\
}

#define Stream_SafeWriteBlock(stream, buffer, count, write)\
ReturnCode result = stream->Write(stream, buffer, count, &write);\
if (write == 0 || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "writing to ");\
}

void Stream_Fail(Stream* stream, ReturnCode result, const UInt8* operation) {
	UInt8 tmpBuffer[String_BufferSize(400)];
	String tmp = String_InitAndClearArray(tmpBuffer);

	String_AppendConst(&tmp, "Failed ");
	String_AppendConst(&tmp, operation);
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

Int32 Stream_TryReadByte(Stream* stream) {
	UInt8 buffer;
	UInt32 read;
	stream->Read(stream, &buffer, sizeof(buffer), &read);
	/* TODO: Check return code here?? Fail if not EndOfStream ?? */
	return read == 0 ? -1 : buffer;
}


void Stream_SetName(Stream* stream, STRING_PURE String* name) {
	stream->Name = String_InitAndClearArray(stream->NameBuffer);
	String_AppendString(&stream->Name, name);
}

ReturnCode Stream_FileRead(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	return Platform_FileRead(stream->Data, data, count, modified);
}
ReturnCode Stream_FileWrite(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	return Platform_FileWrite(stream->Data, data, count, modified);
}
ReturnCode Stream_FileClose(Stream* stream) {
	ReturnCode code = Platform_FileClose(stream->Data);
	stream->Data = NULL;
	return code;
}
ReturnCode Stream_FileSeek(Stream* stream, Int32 offset, Int32 seekType) {
	return Platform_FileSeek(stream->Data, offset, seekType);
}

void Stream_FromFile(Stream* stream, void* file, STRING_PURE String* name) {
	Stream_SetName(stream, name);
	stream->Data  = file;
	stream->Data2 = 0;

	stream->Read  = Stream_FileRead;
	stream->Write = Stream_FileWrite;
	stream->Close = Stream_FileClose;
	stream->Seek  = Stream_FileSeek;
}

ReturnCode Stream_PortionRead(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	count = min(count, stream->Data2);
	Stream* underlying = (Stream*)stream->Data;
	ReturnCode code = underlying->Read(underlying, data, count, modified);
	stream->Data2 -= *modified;
	return code;
}
ReturnCode Stream_PortionWrite(Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	*modified = 0; return 1;
}
ReturnCode Stream_PortionClose(Stream* stream) { return 0; }
ReturnCode Stream_PortionSeek(Stream* stream, Int32 offset, Int32 seekType) { return 1; }

void Stream_ReadonlyPortion(Stream* stream, Stream* underlying, UInt32 len) {
	Stream_SetName(stream, &underlying->Name);
	stream->Data  = underlying;
	stream->Data2 = len;

	stream->Read  = Stream_PortionRead;
	stream->Write = Stream_PortionWrite;
	stream->Close = Stream_PortionClose;
	stream->Seek  = Stream_PortionSeek;
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


bool Stream_ReadLine(Stream* stream, STRING_TRANSIENT String* text) {
	String_Clear(text);
	for (;;) {
		UInt32 byteCount = 0, read = 0;
		UInt8 header;

		ReturnCode code = stream->Read(stream, &header, 1, &read);
		if (read == 0) return false; /* end of stream */
		if (!ErrorHandler_Check(code)) { Stream_Fail(stream, code, "reading line from "); }

		/* Header byte encodes variable number of following bytes */
		/* The remaining bits of the header form first part of the character */
		Int32 bit;
		for (bit = 7; bit >= 0; bit--) {
			if ((header & (1 << bit)) != 0) {
				byteCount++;
				header &= (UInt8)~(1 << bit);
			} else {
				break;
			}
		}

		UInt16 codeword;
		if (byteCount == 0) {
			codeword = (UInt8)header;
		} else {
			codeword = header;
			Int32 i;
			for (i = 0; i < byteCount - 1; i++) {
				codeword <<= 6;
				/* Top two bits of each are always 10 */
				codeword |= (UInt16)(Stream_ReadUInt8(stream) & 0x3F);
			}
		}

		/* Handle \r\n or \n line endings */
		if (codeword == '\r') continue;
		if (codeword == '\n') return true;
		String_Append(text, Convert_UnicodeToCP437(codeword));
	}
}

void Stream_WriteLine(Stream* stream, STRING_TRANSIENT String* text) {
	UInt8 buffer[3584];
	UInt32 i, j = 0;

	for (i = 0; i < text->length; i++) {
		/* Just in case, handle extremely large strings */
		if (j >= 3032) {
			Stream_Write(stream, buffer, j);
			j = 0;
		}

		UInt16 codepoint = Convert_CP437ToUnicode(text->buffer[i]);
		if (codepoint <= 0x7F) {
			buffer[j++] = (UInt8)codepoint;
		} else if (codepoint >= 0x80 && codepoint <= 0x7FF) {
			buffer[j++] = (UInt8)(0xC0 | (codepoint >> 6) & 0x1F);
			buffer[j++] = (UInt8)(0x80 | (codepoint) & 0x3F);
		} else {
			buffer[j++] = (UInt8)(0xE0 | (codepoint >> 12) & 0x0F);
			buffer[j++] = (UInt8)(0x80 | (codepoint >> 6) & 0x3F);
			buffer[j++] = (UInt8)(0x80 | (codepoint) & 0x3F);
		}
	}
	
	UInt8* ptr = Platform_NewLine;
	while (*ptr != NULL) { buffer[j++] = *ptr++; }
	if (j > 0) Stream_Write(stream, buffer, j);
}