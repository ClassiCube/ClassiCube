#include "Stream.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Errors.h"

/*########################################################################################################################*
*---------------------------------------------------------Stream----------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_Read(struct Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 read;
	while (count) {
		ReturnCode res = stream->Read(stream, buffer, count, &read);
		if (res) return res;
		if (!read) return ERR_END_OF_STREAM;

		buffer += read;
		count  -= read;
	}
	return 0;
}

ReturnCode Stream_Write(struct Stream* stream, UInt8* buffer, UInt32 count) {
	UInt32 write;
	while (count) {
		ReturnCode res = stream->Write(stream, buffer, count, &write);
		if (res) return res;
		if (!write) return ERR_END_OF_STREAM;

		buffer += write;
		count  -= write;
	}
	return 0;
}

ReturnCode Stream_Skip(struct Stream* stream, UInt32 count) {
	ReturnCode res = stream->Seek(stream, count, STREAM_SEEKFROM_CURRENT);
	if (!res) return 0;
	UInt8 tmp[3584]; /* not quite 4 KB to avoid chkstk call */

	while (count) {
		UInt32 toRead = min(count, sizeof(tmp)), read;
		res = stream->Read(stream, tmp, toRead, &read);
		if (res) return res;
		if (!read) return ERR_END_OF_STREAM;

		count -= read;
	}
	return 0;
}

static ReturnCode Stream_DefaultIO(struct Stream* stream, UInt8* data, UInt32 count, UInt32* modified) {
	*modified = 0; return ReturnCode_NotSupported;
}
ReturnCode Stream_DefaultReadU8(struct Stream* stream, UInt8* data) {
	UInt32 modified = 0;
	ReturnCode res = stream->Read(stream, data, 1, &modified);
	return res ? res : (modified ? 0 : ERR_END_OF_STREAM);
}

static ReturnCode Stream_DefaultClose(struct Stream* stream) { return 0; }
static ReturnCode Stream_DefaultSeek(struct Stream* stream, Int32 offset, Int32 seekType) { 
	return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultGet(struct Stream* stream, UInt32* value) { 
	*value = 0; return ReturnCode_NotSupported; 
}

void Stream_Init(struct Stream* stream) {
	stream->Read   = Stream_DefaultIO;
	stream->ReadU8 = Stream_DefaultReadU8;
	stream->Write  = Stream_DefaultIO;
	stream->Close  = Stream_DefaultClose;
	stream->Seek   = Stream_DefaultSeek;
	stream->Position = Stream_DefaultGet;
	stream->Length   = Stream_DefaultGet;
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
	ReturnCode res = File_Close(stream->Meta.File);
	stream->Meta.File = NULL;
	return res;
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

void Stream_FromFile(struct Stream* stream, void* file) {
	Stream_Init(stream);
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

static ReturnCode Stream_PortionReadU8(struct Stream* stream, UInt8* data) {
	if (!stream->Meta.Mem.Left) return ERR_END_OF_STREAM;
	stream->Meta.Mem.Left--;
	struct Stream* source = stream->Meta.Portion.Source;
	return source->ReadU8(source, data);
}

static ReturnCode Stream_PortionPosition(struct Stream* stream, UInt32* position) {
	*position = stream->Meta.Portion.Length - stream->Meta.Portion.Left; return 0;
}
static ReturnCode Stream_PortionLength(struct Stream* stream, UInt32* length) {
	*length = stream->Meta.Portion.Length; return 0;
}

void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, UInt32 len) {
	Stream_Init(stream);
	stream->Read     = Stream_PortionRead;
	stream->ReadU8   = Stream_PortionReadU8;
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
	
	stream->Meta.Mem.Cur  += count; stream->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemoryReadU8(struct Stream* stream, UInt8* data) {
	if (!stream->Meta.Mem.Left) return ERR_END_OF_STREAM;

	*data = *stream->Meta.Mem.Cur;
	stream->Meta.Mem.Cur++; stream->Meta.Mem.Left--;
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
	default: return ReturnCode_InvalidArg;
	}

	if (pos < 0 || pos >= stream->Meta.Mem.Length) return ReturnCode_InvalidArg;
	stream->Meta.Mem.Cur  = stream->Meta.Mem.Base   + pos;
	stream->Meta.Mem.Left = stream->Meta.Mem.Length - pos;
	return 0;
}

static void Stream_CommonMemory(struct Stream* stream, void* data, UInt32 len) {
	Stream_Init(stream);
	stream->Seek = Stream_MemorySeek;
	/* TODO: Should we use separate Stream_MemoryPosition functions? */
	stream->Position = Stream_PortionPosition;
	stream->Length   = Stream_PortionLength;

	stream->Meta.Mem.Cur = data;
	stream->Meta.Mem.Left = len;
	stream->Meta.Mem.Length = len;
	stream->Meta.Mem.Base = data;
}

void Stream_ReadonlyMemory(struct Stream* stream, void* data, UInt32 len) {
	Stream_CommonMemory(stream, data, len);
	stream->Read   = Stream_MemoryRead;
	stream->ReadU8 = Stream_MemoryReadU8;
}

void Stream_WriteonlyMemory(struct Stream* stream, void* data, UInt32 len) {
	Stream_CommonMemory(stream, data, len);
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

		ReturnCode res = source->Read(source, stream->Meta.Buffered.Cur, stream->Meta.Buffered.Length, &read);
		if (res) return res;
		stream->Meta.Mem.Left = read;
	}
	return Stream_MemoryRead(stream, data, count, modified);
}

static ReturnCode Stream_BufferedReadU8(struct Stream* stream, UInt8* data) {
	if (stream->Meta.Buffered.Left) return Stream_MemoryReadU8(stream, data);
	return Stream_DefaultReadU8(stream, data);
}

void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, UInt32 size) {
	Stream_Init(stream);
	stream->Read   = Stream_BufferedRead;
	stream->ReadU8 = Stream_BufferedReadU8;

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

void Stream_SetU16_BE(UInt8* data, UInt16 value) {
	data[0] = (UInt8)(value >> 8 ); data[1] = (UInt8)(value      );
}

void Stream_SetU32_LE(UInt8* data, UInt32 value) {
	data[0] = (UInt8)(value      ); data[1] = (UInt8)(value >> 8 );
	data[2] = (UInt8)(value >> 16); data[3] = (UInt8)(value >> 24);
}

void Stream_SetU32_BE(UInt8* data, UInt32 value) {
	data[0] = (UInt8)(value >> 24); data[1] = (UInt8)(value >> 16);
	data[2] = (UInt8)(value >> 8 ); data[3] = (UInt8)(value);
}

ReturnCode Stream_ReadU32_LE(struct Stream* stream, UInt32* value) {
	UInt8 data[4]; ReturnCode res;
	if ((res = Stream_Read(stream, data, 4))) return res;
	*value = Stream_GetU32_LE(data); return 0;
}

ReturnCode Stream_ReadU32_BE(struct Stream* stream, UInt32* value) {
	UInt8 data[4]; ReturnCode res;
	if ((res = Stream_Read(stream, data, 4))) return res;
	*value = Stream_GetU32_BE(data); return 0;
}


/*########################################################################################################################*
*--------------------------------------------------Read/Write strings-----------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_ReadUtf8(struct Stream* stream, UInt16* codepoint) {
	UInt8 data;
	ReturnCode res = stream->ReadU8(stream, &data);
	if (res) return res;

	/* Header byte is just the raw codepoint (common case) */
	if (data <= 0x7F) { *codepoint = data; return 0; }

	/* Header byte encodes variable number of following bytes */
	/* The remaining bits of the header form first part of the character */
	Int32 byteCount = 0, i;
	for (i = 7; i >= 0; i--) {
		if (data & (1 << i)) {
			byteCount++;
			data &= (UInt8)~(1 << i);
		} else {
			break;
		}
	}

	*codepoint = data;
	for (i = 0; i < byteCount - 1; i++) {
		if ((res = stream->ReadU8(stream, &data))) return res;

		*codepoint <<= 6;
		/* Top two bits of each are always 10 */
		*codepoint |= (UInt16)(data & 0x3F);
	}
	return 0;
}

ReturnCode Stream_ReadLine(struct Stream* stream, STRING_TRANSIENT String* text) {
	text->length = 0;
	bool readAny = false;
	UInt16 codepoint;

	for (;;) {	
		ReturnCode res = Stream_ReadUtf8(stream, &codepoint);
		if (res == ERR_END_OF_STREAM) break;
		if (res) return res;

		readAny = true;
		/* Handle \r\n or \n line endings */
		if (codepoint == '\r') continue;
		if (codepoint == '\n') return 0;
		String_Append(text, Convert_UnicodeToCP437(codepoint));
	}
	return readAny ? 0 : ERR_END_OF_STREAM;
}

Int32 Stream_WriteUtf8(UInt8* buffer, UInt16 codepoint) {
	if (codepoint <= 0x7F) {
		buffer[0] = codepoint;
		return 1;
	} else if (codepoint <= 0x7FF) {
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

ReturnCode Stream_WriteLine(struct Stream* stream, STRING_TRANSIENT String* text) {
	UInt8 buffer[2048 + 10];
	UInt32 i, j = 0;
	ReturnCode res;

	for (i = 0; i < text->length; i++) {
		/* For extremely large strings */
		if (j >= 2048) {
			if ((res = Stream_Write(stream, buffer, j))) return res;
			j = 0;
		}

		UInt8* cur = buffer + j;
		UInt16 codepoint = Convert_CP437ToUnicode(text->buffer[i]);
		j += Stream_WriteUtf8(cur, codepoint);
	}
	
	UInt8* ptr = Platform_NewLine;
	while (*ptr) { buffer[j++] = *ptr++; }
	return Stream_Write(stream, buffer, j);
}
