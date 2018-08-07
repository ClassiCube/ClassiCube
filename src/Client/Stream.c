#include "Stream.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"

/*########################################################################################################################*
*---------------------------------------------------------Stream----------------------------------------------------------*
*#########################################################################################################################*/
#define Stream_SafeReadBlock(stream, buffer, count, read)\
ReturnCode result = stream->Read(stream, buffer, count, &read);\
if (!read || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "reading from");\
}

#define Stream_SafeWriteBlock(stream, buffer, count, write)\
ReturnCode result = stream->Write(stream, buffer, count, &write);\
if (!write || !ErrorHandler_Check(result)) {\
	Stream_Fail(stream, result, "writing to");\
}

void Stream_Fail(struct Stream* stream, ReturnCode result, const UChar* operation) {
	UChar tmpBuffer[String_BufferSize(400)];
	String tmp = String_InitAndClearArray(tmpBuffer);
	String_Format2(&tmp, "Failed %c %s", operation, &stream->Name);
	ErrorHandler_FailWithCode(result, tmpBuffer);
}

void Stream_Read(struct Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 read;
	while (count > 0) {
		Stream_SafeReadBlock(stream, buffer, count, read);
		buffer += read;
		count  -= read;
	}
}

void Stream_Write(struct Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 write;
	while (count > 0) {
		Stream_SafeWriteBlock(stream, buffer, count, write);
		buffer += write;
		count  -= write;
	}
}

ReturnCode Stream_TryWrite(struct Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 write;
	while (count > 0) {
		ReturnCode result = stream->Write(stream, buffer, count, &write);
		if (result != 0) return result;
		if (!write) return 1;

		buffer += write;
		count  -= write;
	}
	return 0;
}

Int32 Stream_TryReadByte(struct Stream* stream) {
	UInt8 buffer;
	UInt32 read;
	stream->Read(stream, &buffer, sizeof(buffer), &read);
	/* TODO: Check return code here?? Fail if not EndOfStream ?? */
	return read ? buffer : -1;
}

void Stream_Skip(struct Stream* stream, UInt32 count) {
	ReturnCode result = stream->Seek(stream, count, STREAM_SEEKFROM_CURRENT);
	if (result == 0) return;
	UInt8 tmp[3584]; /* not quite 4 KB to avoid chkstk call */

	while (count) {
		UInt32 toRead = min(count, sizeof(tmp)), read;
		result = stream->Read(stream, tmp, toRead, &read);

		if (result != 0) Stream_Fail(stream, result, "Skipping data from");
		if (!read) break; /* end of stream */
		count -= read;
	}

	if (count) Stream_Fail(stream, 0, "Skipping data from");
}

static ReturnCode Stream_DefaultIO(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	*modified = 0; return ReturnCode_NotSupported;
}
static ReturnCode Stream_DefaultClose(struct Stream* stream) { return 0; }
static ReturnCode Stream_DefaultSeek(struct Stream* stream, Int32 offset, Int32 seekType) { 
	return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultGet(struct Stream* stream, UInt32* value) { 
	*value = 0; return ReturnCode_NotSupported; 
}

void Stream_Init(struct Stream* stream, STRING_PURE String* name) {
	stream->Read  = Stream_DefaultIO;
	stream->Write = Stream_DefaultIO;
	stream->Close = Stream_DefaultClose;
	stream->Seek  = Stream_DefaultSeek;
	stream->Position = Stream_DefaultGet;
	stream->Length   = Stream_DefaultGet;
	stream->Name = String_InitAndClearArray(stream->NameBuffer);
	String_AppendString(&stream->Name, name);
}


/*########################################################################################################################*
*-------------------------------------------------------FileStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_FileRead(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	return File_Read(stream->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileWrite(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	return File_Write(stream->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileClose(struct Stream* stream) {
	ReturnCode code = File_Close(stream->Meta.File);
	stream->Meta.File = NULL;
	return code;
}
static ReturnCode Stream_FileSeek(struct Stream* stream, Int32 offset, Int32 seekType) {
	return File_Seek(stream->Meta.File, offset, seekType);
}
static ReturnCode Stream_FilePosition(struct Stream* stream, UInt32* position) {
	return File_Position(stream->Meta.File, position);
}
static ReturnCode Stream_FileLength(struct Stream* stream, UInt32* length) {
	return File_Length(stream->Meta.File, length);
}

void Stream_FromFile(struct Stream* stream, void* file, STRING_PURE String* name) {
	Stream_Init(stream, name);
	stream->Meta.File = file;

	stream->Read  = Stream_FileRead;
	stream->Write = Stream_FileWrite;
	stream->Close = Stream_FileClose;
	stream->Seek  = Stream_FileSeek;
	stream->Position = Stream_FilePosition;
	stream->Length   = Stream_FileLength;
}


/*########################################################################################################################*
*-----------------------------------------------------PortionStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_PortionRead(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	count = min(count, stream->Meta.Mem.Left);
	struct Stream* source = stream->Meta.Portion.Source;
	ReturnCode code = source->Read(source, data, count, modified);
	stream->Meta.Mem.Left -= *modified;
	return code;
}

static ReturnCode Stream_PortionPosition(struct Stream* stream, UInt32* position) {
	*position = stream->Meta.Portion.Length - stream->Meta.Portion.Left; return 0;
}
static ReturnCode Stream_PortionLength(struct Stream* stream, UInt32* length) {
	*length = stream->Meta.Portion.Length; return 0;
}

void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, UInt32 len) {
	Stream_Init(stream, &source->Name);
	stream->Read     = Stream_PortionRead;
	stream->Position = Stream_PortionPosition;
	stream->Length   = Stream_PortionLength;

	stream->Meta.Portion.Source = source;
	stream->Meta.Portion.Left   = len;
	stream->Meta.Portion.Length = len;
}


/*########################################################################################################################*
*-----------------------------------------------------MemoryStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_MemoryRead(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	count = min(count, stream->Meta.Mem.Left);
	if (count) { Mem_Copy(data, stream->Meta.Mem.Cur, count); }
	
	stream->Meta.Mem.Cur  += count;
	stream->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemoryWrite(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	count = min(count, stream->Meta.Mem.Left);
	if (count) { Mem_Copy(stream->Meta.Mem.Cur, data, count); }

	stream->Meta.Mem.Cur   += count;
	stream->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemorySeek(struct Stream* stream, Int32 offset, Int32 seekType) {
	Int32 pos;
	UInt32 curOffset = (UInt32)(stream->Meta.Mem.Cur - stream->Meta.Mem.Base);

	switch (seekType) {
	case STREAM_SEEKFROM_BEGIN:
		pos = offset; break;
	case STREAM_SEEKFROM_CURRENT:
		pos = (Int32)curOffset + offset; break;
	case STREAM_SEEKFROM_END:
		pos = (Int32)stream->Meta.Mem.Length + offset; break;
	default: return 2;
	}

	if (pos < 0 || pos >= stream->Meta.Mem.Length) return ReturnCode_InvalidArg;
	stream->Meta.Mem.Cur  = stream->Meta.Mem.Base   + pos;
	stream->Meta.Mem.Left = stream->Meta.Mem.Length - pos;
	return 0;
}

void Stream_ReadonlyMemory(struct Stream* stream, void* data, UInt32 len, STRING_PURE String* name) {
	Stream_Init(stream, name);
	stream->Read     = Stream_MemoryRead;
	stream->Seek     = Stream_MemorySeek;
	/* TODO: Should we use separate Stream_MemoryPosition functions? */
	stream->Position = Stream_PortionPosition;
	stream->Length   = Stream_PortionLength;

	stream->Meta.Mem.Cur    = data;
	stream->Meta.Mem.Left   = len;
	stream->Meta.Mem.Length = len;
	stream->Meta.Mem.Base   = data;
}

void Stream_WriteonlyMemory(struct Stream* stream, void* data, UInt32 len, STRING_PURE String* name) {
	Stream_ReadonlyMemory(stream, data, len, name);
	stream->Read  = Stream_DefaultIO;
	stream->Write = Stream_MemoryWrite;
}


/*########################################################################################################################*
*----------------------------------------------------BufferedStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_BufferedRead(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	if (stream->Meta.Buffered.Left == 0) {
		struct Stream* source = stream->Meta.Buffered.Source; 
		stream->Meta.Buffered.Cur = stream->Meta.Buffered.Base;
		UInt32 read = 0;

		ReturnCode result = source->Read(source, stream->Meta.Buffered.Cur, stream->Meta.Buffered.Length, &read);
		if (result != 0) return result;
		stream->Meta.Mem.Left = read;
	}
	return Stream_MemoryRead(stream, data, count, modified);
}

void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, UInt32 size) {
	Stream_Init(stream, &source->Name);
	stream->Read = Stream_BufferedRead;

	stream->Meta.Buffered.Cur    = data;
	stream->Meta.Mem.Left        = 0;
	stream->Meta.Mem.Length      = size;
	stream->Meta.Buffered.Base   = data;
	stream->Meta.Buffered.Source = source;
}


/*########################################################################################################################*
*-------------------------------------------------Read/Write primitives---------------------------------------------------*
*#########################################################################################################################*/
UInt16 Stream_GetU16_LE(UInt8* data) {
	return (UInt16)(data[0] | (data[1] << 8));
}

UInt16 Stream_GetU16_BE(UInt8* data) {
	return (UInt16)((data[0] << 8) | data[1]);
}

UInt32 Stream_GetU32_LE(UInt8* data) {
	return (UInt32)(
		 (UInt32)data[0]        | ((UInt32)data[1] << 8) |
		((UInt32)data[2] << 16) | ((UInt32)data[3] << 24));
}

UInt32 Stream_GetU32_BE(UInt8* data) {
	return (UInt32)(
		((UInt32)data[0] << 24) | ((UInt32)data[1] << 16) |
		((UInt32)data[2] << 8)  |  (UInt32)data[3]);
}

UInt8 Stream_ReadU8(struct Stream* stream) {
	UInt8 buffer;
	UInt32 read;
	/* Inline 8 bit reading, because it is very frequently used. */
	Stream_SafeReadBlock(stream, &buffer, sizeof(UInt8), read);
	return buffer;
}

UInt16 Stream_ReadU16_LE(struct Stream* stream) {
	UInt8 buffer[sizeof(UInt16)];
	Stream_Read(stream, buffer, sizeof(UInt16));
	return Stream_GetU16_LE(buffer);
}

UInt16 Stream_ReadU16_BE(struct Stream* stream) {
	UInt8 buffer[sizeof(UInt16)];
	Stream_Read(stream, buffer, sizeof(UInt16));
	return Stream_GetU16_BE(buffer);
}

UInt32 Stream_ReadU32_LE(struct Stream* stream) {
	UInt8 buffer[sizeof(UInt32)];
	Stream_Read(stream, buffer, sizeof(UInt32));
	return Stream_GetU32_LE(buffer);
}

UInt32 Stream_ReadU32_BE(struct Stream* stream) {
	UInt8 buffer[sizeof(UInt32)];
	Stream_Read(stream, buffer, sizeof(UInt32));
	return Stream_GetU32_BE(buffer);
}

void Stream_WriteU8(struct Stream* stream, UInt8 value) {
	UInt32 write;
	/* Inline 8 bit writing, because it is very frequently used. */
	Stream_SafeWriteBlock(stream, &value, sizeof(UInt8), write);
}

void Stream_WriteU16_BE(struct Stream* stream, UInt16 value) {
	UInt8 buffer[sizeof(UInt16)];
	buffer[0] = (UInt8)(value >> 8 ); buffer[1] = (UInt8)(value      );
	Stream_Write(stream, buffer, sizeof(UInt16));
}

void Stream_WriteU32_LE(struct Stream* stream, UInt32 value) {
	UInt8 buffer[sizeof(UInt32)];
	buffer[0] = (UInt8)(value      ); buffer[1] = (UInt8)(value >> 8 );
	buffer[2] = (UInt8)(value >> 16); buffer[3] = (UInt8)(value >> 24);
	Stream_Write(stream, buffer, sizeof(UInt32));
}

void Stream_WriteU32_BE(struct Stream* stream, UInt32 value) {
	UInt8 buffer[sizeof(UInt32)];
	buffer[0] = (UInt8)(value >> 24); buffer[1] = (UInt8)(value >> 16);
	buffer[2] = (UInt8)(value >> 8 ); buffer[3] = (UInt8)(value);
	Stream_Write(stream, buffer, sizeof(UInt32));
}


/*########################################################################################################################*
*--------------------------------------------------Read/Write strings-----------------------------------------------------*
*#########################################################################################################################*/
bool Stream_ReadUtf8Char(struct Stream* stream, UInt16* codepoint) {
	UInt32 read = 0;
	UInt8 data;
	ReturnCode result = stream->Read(stream, &data, 1, &read);

	if (!read) return false; /* end of stream */
	if (!ErrorHandler_Check(result)) { Stream_Fail(stream, result, "reading utf8 from"); }
	/* Header byte is just the raw codepoint (common case) */
	if (data <= 0x7F) { *codepoint = data; return true; }

	/* Header byte encodes variable number of following bytes */
	/* The remaining bits of the header form first part of the character */
	Int32 byteCount = 0, i;
	for (i = 7; i >= 0; i--) {
		if ((data & (1 << i)) != 0) {
			byteCount++;
			data &= (UInt8)~(1 << i);
		} else {
			break;
		}
	}

	*codepoint = data;
	for (i = 0; i < byteCount - 1; i++) {
		result = stream->Read(stream, &data, 1, &read);
		if (!read) return false; /* end of stream */
		if (!ErrorHandler_Check(result)) { Stream_Fail(stream, result, "reading utf8 from"); }

		*codepoint <<= 6;
		/* Top two bits of each are always 10 */
		*codepoint |= (UInt16)(data & 0x3F);
	}
	return true;
}

bool Stream_ReadLine(struct Stream* stream, STRING_TRANSIENT String* text) {
	String_Clear(text);
	bool readAny = false;
	for (;;) {
		UInt16 codepoint;
		if (!Stream_ReadUtf8Char(stream, &codepoint)) break;
		readAny = true;

		/* Handle \r\n or \n line endings */
		if (codepoint == '\r') continue;
		if (codepoint == '\n') return true;
		String_Append(text, Convert_UnicodeToCP437(codepoint));
	}
	return readAny;
}

Int32 Stream_WriteUtf8(UInt8* buffer, UInt16 codepoint) {
	if (codepoint <= 0x7F) {
		buffer[0] = codepoint;
		return 1;
	} else if (codepoint >= 0x80 && codepoint <= 0x7FF) {
		buffer[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
		buffer[1] = 0x80 | ((codepoint)      & 0x3F);
		return 2;
	} else {
		buffer[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
		buffer[1] = 0x80 | ((codepoint >> 6)  & 0x3F);
		buffer[2] = 0x80 | ((codepoint)       & 0x3F);
		return 3;
	}
}

void Stream_WriteLine(struct Stream* stream, STRING_TRANSIENT String* text) {
	UInt8 buffer[2048 + 10];
	UInt32 i, j = 0;

	for (i = 0; i < text->length; i++) {
		/* For extremely large strings */
		if (j >= 2048) {
			Stream_Write(stream, buffer, j); j = 0;
		}

		UInt8* cur = buffer + j;
		UInt16 codepoint = Convert_CP437ToUnicode(text->buffer[i]);
		j += Stream_WriteUtf8(cur, codepoint);
	}
	
	UInt8* ptr = Platform_NewLine;
	while (*ptr) { buffer[j++] = *ptr++; }
	if (j > 0) Stream_Write(stream, buffer, j);
}
