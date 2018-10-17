#include "Stream.h"
#include "Platform.h"
#include "Funcs.h"
#include "ErrorHandler.h"
#include "Errors.h"

/*########################################################################################################################*
*---------------------------------------------------------Stream----------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_Read(struct Stream* s, uint8_t* buffer, uint32_t count) {
	uint32_t read;
	ReturnCode res;

	while (count) {
		if ((res = s->Read(s, buffer, count, &read))) return res;
		if (!read) return ERR_END_OF_STREAM;

		buffer += read;
		count  -= read;
	}
	return 0;
}

ReturnCode Stream_Write(struct Stream* s, uint8_t* buffer, uint32_t count) {
	uint32_t write;
	ReturnCode res;

	while (count) {
		if ((res = s->Write(s, buffer, count, &write))) return res;
		if (!write) return ERR_END_OF_STREAM;

		buffer += write;
		count  -= write;
	}
	return 0;
}

static ReturnCode Stream_DefaultIO(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	*modified = 0; return ReturnCode_NotSupported;
}
ReturnCode Stream_DefaultReadU8(struct Stream* s, uint8_t* data) {
	uint32_t modified = 0;
	ReturnCode res = s->Read(s, data, 1, &modified);
	return res ? res : (modified ? 0 : ERR_END_OF_STREAM);
}

static ReturnCode Stream_DefaultSkip(struct Stream* s, uint32_t count) {
	uint8_t tmp[3584]; /* not quite 4 KB to avoid chkstk call */
	ReturnCode res;

	while (count) {
		uint32_t toRead = min(count, sizeof(tmp)), read;
		if ((res = s->Read(s, tmp, toRead, &read))) return res;

		if (!read) return ERR_END_OF_STREAM;
		count -= read;
	}
	return 0;
}

static ReturnCode Stream_DefaultSeek(struct Stream* s, uint32_t pos) {
	return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultGet(struct Stream* s, uint32_t* value) { 
	*value = 0; return ReturnCode_NotSupported; 
}
static ReturnCode Stream_DefaultClose(struct Stream* s) { return 0; }

void Stream_Init(struct Stream* s) {
	s->Read   = Stream_DefaultIO;
	s->ReadU8 = Stream_DefaultReadU8;
	s->Write  = Stream_DefaultIO;
	s->Skip   = Stream_DefaultSkip;

	s->Seek   = Stream_DefaultSeek;
	s->Position = Stream_DefaultGet;
	s->Length = Stream_DefaultGet;
	s->Close  = Stream_DefaultClose;
}


/*########################################################################################################################*
*-------------------------------------------------------FileStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_FileRead(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	return File_Read(s->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileWrite(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	return File_Write(s->Meta.File, data, count, modified);
}
static ReturnCode Stream_FileClose(struct Stream* s) {
	ReturnCode res = File_Close(s->Meta.File);
	s->Meta.File = NULL;
	return res;
}
static ReturnCode Stream_FileSkip(struct Stream* s, uint32_t count) {
	return File_Seek(s->Meta.File, count, FILE_SEEKFROM_CURRENT);
}
static ReturnCode Stream_FileSeek(struct Stream* s, uint32_t position) {
	return File_Seek(s->Meta.File, position, FILE_SEEKFROM_BEGIN);
}
static ReturnCode Stream_FilePosition(struct Stream* s, uint32_t* position) {
	return File_Position(s->Meta.File, position);
}
static ReturnCode Stream_FileLength(struct Stream* s, uint32_t* length) {
	return File_Length(s->Meta.File, length);
}

ReturnCode Stream_OpenFile(struct Stream* s, const String* path) {
	void* file;
	ReturnCode res = File_Open(&file, path);
	Stream_FromFile(s, file);
	return res;
}
ReturnCode Stream_CreateFile(struct Stream* s, const String* path) {
	void* file;
	ReturnCode res = File_Create(&file, path);
	Stream_FromFile(s, file);
	return res;
}

void Stream_FromFile(struct Stream* s, void* file) {
	Stream_Init(s);
	s->Meta.File = file;

	s->Read  = Stream_FileRead;
	s->Write = Stream_FileWrite;
	s->Close = Stream_FileClose;
	s->Skip  = Stream_FileSkip;
	s->Seek  = Stream_FileSeek;
	s->Position = Stream_FilePosition;
	s->Length   = Stream_FileLength;
}


/*########################################################################################################################*
*-----------------------------------------------------PortionStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_PortionRead(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, s->Meta.Portion.Left);
	struct Stream* source = s->Meta.Portion.Source;

	ReturnCode res = source->Read(source, data, count, modified);
	s->Meta.Portion.Left -= *modified;
	return res;
}

static ReturnCode Stream_PortionReadU8(struct Stream* s, uint8_t* data) {
	if (!s->Meta.Portion.Left) return ERR_END_OF_STREAM;
	struct Stream* source = s->Meta.Portion.Source;

	s->Meta.Portion.Left--;
	return source->ReadU8(source, data);
}

static ReturnCode Stream_PortionSkip(struct Stream* s, uint32_t count) {
	if (count > s->Meta.Portion.Left) return ReturnCode_InvalidArg;
	struct Stream* source = s->Meta.Portion.Source;

	ReturnCode res = source->Skip(source, count);
	if (!res) s->Meta.Portion.Left -= count;
	return res;
}

static ReturnCode Stream_PortionPosition(struct Stream* s, uint32_t* position) {
	*position = s->Meta.Portion.Length - s->Meta.Portion.Left; return 0;
}
static ReturnCode Stream_PortionLength(struct Stream* s, uint32_t* length) {
	*length = s->Meta.Portion.Length; return 0;
}

void Stream_ReadonlyPortion(struct Stream* s, struct Stream* source, uint32_t len) {
	Stream_Init(s);
	s->Read     = Stream_PortionRead;
	s->ReadU8   = Stream_PortionReadU8;
	s->Skip     = Stream_PortionSkip;
	s->Position = Stream_PortionPosition;
	s->Length   = Stream_PortionLength;

	s->Meta.Portion.Source = source;
	s->Meta.Portion.Left   = len;
	s->Meta.Portion.Length = len;
}


/*########################################################################################################################*
*-----------------------------------------------------MemoryStream--------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_MemoryRead(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, s->Meta.Mem.Left);
	Mem_Copy(data, s->Meta.Mem.Cur, count);
	
	s->Meta.Mem.Cur += count; s->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemoryReadU8(struct Stream* s, uint8_t* data) {
	if (!s->Meta.Mem.Left) return ERR_END_OF_STREAM;

	*data = *s->Meta.Mem.Cur;
	s->Meta.Mem.Cur++; s->Meta.Mem.Left--;
	return 0;
}

static ReturnCode Stream_MemoryWrite(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	count = min(count, s->Meta.Mem.Left);
	Mem_Copy(s->Meta.Mem.Cur, data, count);

	s->Meta.Mem.Cur += count; s->Meta.Mem.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_MemorySkip(struct Stream* s, uint32_t count) {
	if (count > s->Meta.Mem.Left) return ReturnCode_InvalidArg;

	s->Meta.Mem.Cur += count; s->Meta.Mem.Left -= count;
	return 0;
}

static ReturnCode Stream_MemorySeek(struct Stream* s, uint32_t position) {
	if (position >= s->Meta.Mem.Length) return ReturnCode_InvalidArg;

	s->Meta.Mem.Cur  = s->Meta.Mem.Base   + position;
	s->Meta.Mem.Left = s->Meta.Mem.Length - position;
	return 0;
}

static ReturnCode Stream_MemoryPosition(struct Stream* s, uint32_t* position) {
	*position = s->Meta.Mem.Length - s->Meta.Mem.Left; return 0;
}
static ReturnCode Stream_MemoryLength(struct Stream* s, uint32_t* length) {
	*length = s->Meta.Mem.Length; return 0;
}

static void Stream_CommonMemory(struct Stream* s, void* data, uint32_t len) {
	Stream_Init(s);
	s->Skip     = Stream_MemorySkip;
	s->Seek     = Stream_MemorySeek;
	s->Position = Stream_MemoryPosition;
	s->Length   = Stream_MemoryLength;

	s->Meta.Mem.Cur    = data;
	s->Meta.Mem.Left   = len;
	s->Meta.Mem.Length = len;
	s->Meta.Mem.Base   = data;
}

void Stream_ReadonlyMemory(struct Stream* s, void* data, uint32_t len) {
	Stream_CommonMemory(s, data, len);
	s->Read   = Stream_MemoryRead;
	s->ReadU8 = Stream_MemoryReadU8;
}

void Stream_WriteonlyMemory(struct Stream* s, void* data, uint32_t len) {
	Stream_CommonMemory(s, data, len);
	s->Write = Stream_MemoryWrite;
}


/*########################################################################################################################*
*----------------------------------------------------BufferedStream-------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Stream_BufferedRead(struct Stream* s, uint8_t* data, uint32_t count, uint32_t* modified) {
	if (!s->Meta.Buffered.Left) {
		struct Stream* source = s->Meta.Buffered.Source; 
		s->Meta.Buffered.Cur  = s->Meta.Buffered.Base;
		uint32_t read = 0;

		ReturnCode res = source->Read(source, s->Meta.Buffered.Cur, s->Meta.Buffered.Length, &read);
		if (res) return res;
		s->Meta.Buffered.Left  = read;
		s->Meta.Buffered.End  += read;
	}
	
	count = min(count, s->Meta.Buffered.Left);
	Mem_Copy(data, s->Meta.Buffered.Cur, count);

	s->Meta.Buffered.Cur += count; s->Meta.Buffered.Left -= count;
	*modified = count;
	return 0;
}

static ReturnCode Stream_BufferedReadU8(struct Stream* s, uint8_t* data) {
	if (s->Meta.Buffered.Left) {
		*data = *s->Meta.Buffered.Cur;
		s->Meta.Buffered.Cur++; s->Meta.Buffered.Left--;
		return 0;
	}
	return Stream_DefaultReadU8(s, data);
}

static ReturnCode Stream_BufferedSeek(struct Stream* s, uint32_t position) {
	/* Check if seek position is within cached buffer */
	uint32_t length = s->Meta.Buffered.Left + (uint32_t)(s->Meta.Buffered.Cur - s->Meta.Buffered.Base);
	uint32_t start  = s->Meta.Buffered.End  - length;

	if (position >= start && position < start + length) {
		uint32_t offset = position - start;
		s->Meta.Buffered.Cur  = s->Meta.Buffered.Base + offset;
		s->Meta.Buffered.Left = length - offset;
		return 0;
	}

	struct Stream* source = s->Meta.Buffered.Source;
	ReturnCode res        = source->Seek(source, position);

	if (res) return res;
	s->Meta.Buffered.Left = 0;
	s->Meta.Buffered.End  = position;	
	return res;
}

void Stream_ReadonlyBuffered(struct Stream* s, struct Stream* source, void* data, uint32_t size) {
	Stream_Init(s);
	s->Read   = Stream_BufferedRead;
	s->ReadU8 = Stream_BufferedReadU8;
	s->Seek   = Stream_BufferedSeek;

	s->Meta.Buffered.Left   = 0;
	s->Meta.Buffered.End    = 0;	
	s->Meta.Buffered.Cur    = data;
	s->Meta.Buffered.Base   = data;
	s->Meta.Buffered.Length = size;
	s->Meta.Buffered.Source = source;
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

ReturnCode Stream_ReadU32_LE(struct Stream* s, uint32_t* value) {
	uint8_t data[4]; ReturnCode res;
	if ((res = Stream_Read(s, data, 4))) return res;
	*value = Stream_GetU32_LE(data); return 0;
}

ReturnCode Stream_ReadU32_BE(struct Stream* s, uint32_t* value) {
	uint8_t data[4]; ReturnCode res;
	if ((res = Stream_Read(s, data, 4))) return res;
	*value = Stream_GetU32_BE(data); return 0;
}


/*########################################################################################################################*
*--------------------------------------------------Read/Write strings-----------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Stream_ReadUtf8(struct Stream* s, Codepoint* cp) {
	uint8_t data;
	ReturnCode res = s->ReadU8(s, &data);
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
		if ((res = s->ReadU8(s, &data))) return res;

		*cp <<= 6;
		/* Top two bits of each are always 10 */
		*cp |= (Codepoint)(data & 0x3F);
	}
	return 0;
}

ReturnCode Stream_ReadLine(struct Stream* s, String* text) {
	text->length = 0;
	bool readAny = false;
	Codepoint cp;

	for (;;) {	
		ReturnCode res = Stream_ReadUtf8(s, &cp);
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

ReturnCode Stream_WriteLine(struct Stream* s, String* text) {
	uint8_t buffer[2048 + 10]; /* some space for newline */
	uint8_t* cur;
	Codepoint cp;
	ReturnCode res;
	int i, len = 0;

	for (i = 0; i < text->length; i++) {
		/* For extremely large strings */
		if (len >= 2048) {
			if ((res = Stream_Write(s, buffer, len))) return res;
			len = 0;
		}

		cur = buffer + len;
		cp  = Convert_CP437ToUnicode(text->buffer[i]);
		len += Stream_WriteUtf8(cur, cp);
	}
	
	cur = Platform_NewLine;
	while (*cur) { buffer[len++] = *cur++; }
	return Stream_Write(s, buffer, len);
}
