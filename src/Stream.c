#include "Stream.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Errors.h"

/*########################################################################################################################*
*---------------------------------------------------------Stream----------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_Read(struct Stream* stream, uint8_t* buffer, uint32_t count) {
	uint32_t read;
	while (count) {
		ReturnCode res = stream->Read(stream, buffer, count, &read);
		if (res) return res;
		if (!read) return ERR_END_OF_STREAM;

		buffer += read;
		count  -= read;
	}
	return 0;
}

ReturnCode Stream_Write(struct Stream* stream, uint8_t* buffer, uint32_t count) {
	uint32_t write;
	while (count) {
		ReturnCode res = stream->Write(stream, buffer, count, &write);
		if (res) return res;
		if (!write) return ERR_END_OF_STREAM;

		buffer += write;
		count  -= write;
	}
	return 0;
}

static ReturnCode Stream_DefaultIO(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	*modified = 0; return ReturnCode_NotSupported;
}
ReturnCode Stream_DefaultReadU8(struct Stream* stream, uint8_t* data) {
	uint32_t modified = 0;
	ReturnCode res = stream->Read(stream, data, 1, &modified);
	return res ? res : (modified ? 0 : ERR_END_OF_STREAM);
}

static ReturnCode Stream_DefaultSkip(struct Stream* stream, uint32_t count) {
	uint8_t tmp[3584]; /* not quite 4 KB to avoid chkstk call */
	ReturnCode res;

	while (count) {
		uint32_t toRead = min(count, sizeof(tmp)), read;
		if ((res = stream->Read(stream, tmp, toRead, &read))) return res;

		if (!read) return ERR_END_OF_STREAM;
		count -= read;
	}
	return 0;
}

static ReturnCode Stream_DefaultSeek(struct Stream* stream, uint32_t pos) {
	return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultGet(struct Stream* stream, uint32_t* value) { 
	*value = 0; return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultClose(struct Stream* stream) { return 0; }

void Stream_Init(struct Stream* stream) {
	stream->Read   = Stream_DefaultIO;
	stream->ReadU8 = Stream_DefaultReadU8;
	stream->Write  = Stream_DefaultIO;
	stream->Skip   = Stream_DefaultSkip;

	stream->Seek   = Stream_DefaultSeek;
	stream->Position = Stream_DefaultGet;
	stream->Length = Stream_DefaultGet;
	stream->Close  = Stream_DefaultClose;
}


/*########################################################################################################################*
*-------------------------------------------------------FileStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_FileRead(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	return File_Read(stream->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	return File_Write(stream->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileClose(struct Stream* stream) {
	ReturnCode res = File_Close(stream->Meta.File);
	stream->Meta.File = NULL;
	return res;
}
static ReturnCode Stream_FileSkip(struct Stream* stream, uint32_t count) {
	return File_Seek(stream->Meta.File, count, FILE_SEEKFROM_CURRENT);
}
static ReturnCode Stream_FileSeek(struct Stream* stream, uint32_t pos) {
	return File_Seek(stream->Meta.File, pos, FILE_SEEKFROM_BEGIN);
}
static ReturnCode Stream_FilePosition(struct Stream* stream, uint32_t* position) {
	return File_Position(stream->Meta.File, position);
}
static ReturnCode Stream_FileLength(struct Stream* stream, uint32_t* length) {
	return File_Length(stream->Meta.File, length);
}

void Stream_FromFile(struct Stream* stream, void* file) {
	Stream_Init(stream);
	stream->Meta.File = file;

	stream->Read  = Stream_FileRead;
	stream->Write = Stream_FileWrite;
	stream->Close = Stream_FileClose;
	stream->Skip  = Stream_FileSkip;
	stream->Seek  = Stream_FileSeek;
	stream->Position = Stream_FilePosition;
	stream->Length   = Stream_FileLength;
}


/*########################################################################################################################*
*-----------------------------------------------------PortionStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_PortionRead(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, stream->Meta.Mem.Left);
	struct Stream* source = stream->Meta.Portion.Source;
	ReturnCode res = source->Read(source, data, count, modified);
	stream->Meta.Mem.Left -= *modified;
	return res;
}

static ReturnCode Stream_PortionReadU8(struct Stream* stream, uint8_t* data) {
	if (!stream->Meta.Mem.Left) return ERR_END_OF_STREAM;
	stream->Meta.Mem.Left--;
	struct Stream* source = stream->Meta.Portion.Source;
	return source->ReadU8(source, data);
}

static ReturnCode Stream_PortionSkip(struct Stream* stream, uint32_t count) {
	if (count > stream->Meta.Mem.Left) return ReturnCode_InvalidArg;
	struct Stream* source = stream->Meta.Portion.Source;

	ReturnCode res = source->Skip(source, count);
	if (!res) stream->Meta.Mem.Left -= count;
	return res;
}

static ReturnCode Stream_PortionPosition(struct Stream* stream, uint32_t* position) {
	*position = stream->Meta.Portion.Length - stream->Meta.Portion.Left; return 0;
}
static ReturnCode Stream_PortionLength(struct Stream* stream, uint32_t* length) {
	*length = stream->Meta.Portion.Length; return 0;
}

void Stream_ReadonlyPortion(struct Stream* stream, struct Stream* source, uint32_t len) {
	Stream_Init(stream);
	stream->Read     = Stream_PortionRead;
	stream->ReadU8   = Stream_PortionReadU8;
	stream->Skip     = Stream_PortionSkip;
	stream->Position = Stream_PortionPosition;
	stream->Length   = Stream_PortionLength;

	stream->Meta.Portion.Source = source;
	stream->Meta.Portion.Left   = len;
	stream->Meta.Portion.Length = len;
}


/*########################################################################################################################*
*-----------------------------------------------------MemoryStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_MemoryRead(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, stream->Meta.Mem.Left);
	if (count) { Mem_Copy(data, stream->Meta.Mem.Cur, count); }
	
	stream->Meta.Mem.Cur  += count; stream->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemoryReadU8(struct Stream* stream, uint8_t* data) {
	if (!stream->Meta.Mem.Left) return ERR_END_OF_STREAM;

	*data = *stream->Meta.Mem.Cur;
	stream->Meta.Mem.Cur++; stream->Meta.Mem.Left--;
	return 0;
}

static ReturnCode Stream_MemoryWrite(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, stream->Meta.Mem.Left);
	if (count) { Mem_Copy(stream->Meta.Mem.Cur, data, count); }

	stream->Meta.Mem.Cur   += count;
	stream->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemorySkip(struct Stream* stream, uint32_t count) {
	if (count > stream->Meta.Mem.Left) return ReturnCode_InvalidArg;

	stream->Meta.Mem.Cur  += count;
	stream->Meta.Mem.Left -= count;
	return 0;
}

static ReturnCode Stream_MemorySeek(struct Stream* stream, uint32_t pos) {
	if (pos >= stream->Meta.Mem.Length) return ReturnCode_InvalidArg;

	stream->Meta.Mem.Cur  = stream->Meta.Mem.Base   + pos;
	stream->Meta.Mem.Left = stream->Meta.Mem.Length - pos;
	return 0;
}

static void Stream_CommonMemory(struct Stream* stream, void* data, uint32_t len) {
	Stream_Init(stream);
	stream->Skip = Stream_MemorySkip;
	stream->Seek = Stream_MemorySeek;
	/* TODO: Should we use separate Stream_MemoryPosition functions? */
	stream->Position = Stream_PortionPosition;
	stream->Length   = Stream_PortionLength;

	stream->Meta.Mem.Cur = data;
	stream->Meta.Mem.Left = len;
	stream->Meta.Mem.Length = len;
	stream->Meta.Mem.Base = data;
}

void Stream_ReadonlyMemory(struct Stream* stream, void* data, uint32_t len) {
	Stream_CommonMemory(stream, data, len);
	stream->Read   = Stream_MemoryRead;
	stream->ReadU8 = Stream_MemoryReadU8;
}

void Stream_WriteonlyMemory(struct Stream* stream, void* data, uint32_t len) {
	Stream_CommonMemory(stream, data, len);
	stream->Write = Stream_MemoryWrite;
}


/*########################################################################################################################*
*----------------------------------------------------BufferedStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_BufferedRead(struct Stream* stream, uint8_t* data, uint32_t count, uint32_t* modified) {
	if (stream->Meta.Buffered.Left == 0) {
		struct Stream* source = stream->Meta.Buffered.Source; 
		stream->Meta.Buffered.Cur = stream->Meta.Buffered.Base;
		uint32_t read = 0;

		ReturnCode res = source->Read(source, stream->Meta.Buffered.Cur, stream->Meta.Buffered.Length, &read);
		if (res) return res;
		stream->Meta.Mem.Left = read;
	}
	return Stream_MemoryRead(stream, data, count, modified);
}

static ReturnCode Stream_BufferedReadU8(struct Stream* stream, uint8_t* data) {
	if (stream->Meta.Buffered.Left) return Stream_MemoryReadU8(stream, data);
	return Stream_DefaultReadU8(stream, data);
}

void Stream_ReadonlyBuffered(struct Stream* stream, struct Stream* source, void* data, uint32_t size) {
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
uint16_t Stream_GetU16_LE(uint8_t* data) {
	return (uint16_t)(data[0] | (data[1] << 8));
}

uint16_t Stream_GetU16_BE(uint8_t* data) {
	return (uint16_t)((data[0] << 8) | data[1]);
}

uint32_t Stream_GetU32_LE(uint8_t* data) {
	return (uint32_t)(
		 (uint32_t)data[0]        | ((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24));
}

uint32_t Stream_GetU32_BE(uint8_t* data) {
	return (uint32_t)(
		((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
		((uint32_t)data[2] << 8)  |  (uint32_t)data[3]);
}

void Stream_SetU16_BE(uint8_t* data, uint16_t value) {
	data[0] = (uint8_t)(value >> 8 ); data[1] = (uint8_t)(value      );
}

void Stream_SetU32_LE(uint8_t* data, uint32_t value) {
	data[0] = (uint8_t)(value      ); data[1] = (uint8_t)(value >> 8 );
	data[2] = (uint8_t)(value >> 16); data[3] = (uint8_t)(value >> 24);
}

void Stream_SetU32_BE(uint8_t* data, uint32_t value) {
	data[0] = (uint8_t)(value >> 24); data[1] = (uint8_t)(value >> 16);
	data[2] = (uint8_t)(value >> 8 ); data[3] = (uint8_t)(value);
}

ReturnCode Stream_ReadU32_LE(struct Stream* stream, uint32_t* value) {
	uint8_t data[4]; ReturnCode res;
	if ((res = Stream_Read(stream, data, 4))) return res;
	*value = Stream_GetU32_LE(data); return 0;
}

ReturnCode Stream_ReadU32_BE(struct Stream* stream, uint32_t* value) {
	uint8_t data[4]; ReturnCode res;
	if ((res = Stream_Read(stream, data, 4))) return res;
	*value = Stream_GetU32_BE(data); return 0;
}


/*########################################################################################################################*
*--------------------------------------------------Read/Write strings-----------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_ReadUtf8(struct Stream* stream, Codepoint* cp) {
	uint8_t data;
	ReturnCode res = stream->ReadU8(stream, &data);
	if (res) return res;

	/* Header byte is just the raw codepoint (common case) */
	if (data <= 0x7F) { *cp = data; return 0; }

	/* Header byte encodes variable number of following bytes */
	/* The remaining bits of the header form first part of the character */
	int byteCount = 0, i;
	for (i = 7; i >= 0; i--) {
		if (data & (1 << i)) {
			byteCount++;
			data &= (uint8_t)~(1 << i);
		} else {
			break;
		}
	}

	*cp = data;
	for (i = 0; i < byteCount - 1; i++) {
		if ((res = stream->ReadU8(stream, &data))) return res;

		*cp <<= 6;
		/* Top two bits of each are always 10 */
		*cp |= (Codepoint)(data & 0x3F);
	}
	return 0;
}

ReturnCode Stream_ReadLine(struct Stream* stream, String* text) {
	text->length = 0;
	bool readAny = false;
	Codepoint cp;

	for (;;) {	
		ReturnCode res = Stream_ReadUtf8(stream, &cp);
		if (res == ERR_END_OF_STREAM) break;
		if (res) return res;

		readAny = true;
		/* Handle \r\n or \n line endings */
		if (cp == '\r') continue;
		if (cp == '\n') return 0;
		String_Append(text, Convert_UnicodeToCP437(cp));
	}
	return readAny ? 0 : ERR_END_OF_STREAM;
}

int Stream_WriteUtf8(uint8_t* buffer, Codepoint cp) {
	if (cp <= 0x7F) {
		buffer[0] = (uint8_t)cp;
		return 1;
	} else if (cp <= 0x7FF) {
		buffer[0] = 0xC0 | ((cp >> 6) & 0x1F);
		buffer[1] = 0x80 | ((cp)      & 0x3F);
		return 2;
	} else {
		buffer[0] = 0xE0 | ((cp >> 12) & 0x0F);
		buffer[1] = 0x80 | ((cp >> 6)  & 0x3F);
		buffer[2] = 0x80 | ((cp)       & 0x3F);
		return 3;
	}
}

ReturnCode Stream_WriteLine(struct Stream* stream, String* text) {
	uint8_t buffer[2048 + 10];
	int i, j = 0;
	ReturnCode res;

	for (i = 0; i < text->length; i++) {
		/* For extremely large strings */
		if (j >= 2048) {
			if ((res = Stream_Write(stream, buffer, j))) return res;
			j = 0;
		}

		uint8_t* cur   = buffer + j;
		Codepoint cp = Convert_CP437ToUnicode(text->buffer[i]);
		j += Stream_WriteUtf8(cur, cp);
	}
	
	uint8_t* ptr = Platform_NewLine;
	while (*ptr) { buffer[j++] = *ptr++; }
	return Stream_Write(stream, buffer, j);
}
