#include "Stream.h"
#include "String_.h"
#include "Platform.h"
#include "Funcs.h"
#include "Errors.h"
#include "Utils.h"

/*########################################################################################################################*
*---------------------------------------------------------Stream----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Stream_Read(struct Stream* s, cc_uint8* buffer, cc_uint32 count) {
	cc_uint32 read;
	cc_result res;

	while (count) {
		if ((res = s->Read(s, buffer, count, &read))) return res;
		if (!read) return ERR_END_OF_STREAM;

		buffer += read;
		count  -= read;
	}
	return 0;
}

cc_result Stream_Write(struct Stream* s, const cc_uint8* buffer, cc_uint32 count) {
	cc_uint32 write;
	cc_result res;

	while (count) {
		if ((res = s->Write(s, buffer, count, &write))) return res;
		if (!write) return ERR_END_OF_STREAM;

		buffer += write;
		count  -= write;
	}
	return 0;
}

static cc_result Stream_DefaultRead(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}
static cc_result Stream_DefaultWrite(struct Stream* s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return ERR_NOT_SUPPORTED;
}
/* Slow way of reading a U8 integer through stream->Read(stream, 1, tmp) */
static cc_result Stream_DefaultReadU8(struct Stream* s, cc_uint8* data) {
	cc_uint32 modified;
	cc_result res = s->Read(s, data, 1, &modified);
	return res ? res : (modified ? 0 : ERR_END_OF_STREAM);
}

static cc_result Stream_DefaultSkip(struct Stream* s, cc_uint32 count) {
	cc_uint8 tmp[3584]; /* not quite 4 KB to avoid chkstk call */
	cc_uint32 toRead, read;
	cc_result res;

	while (count) {
		toRead = min(count, sizeof(tmp));
		if ((res = s->Read(s, tmp, toRead, &read))) return res;

		if (!read) return ERR_END_OF_STREAM;
		count -= read;
	}
	return 0;
}

static cc_result Stream_DefaultSeek(struct Stream* s, cc_uint32 pos) {
	return ERR_NOT_SUPPORTED;
}
static cc_result Stream_DefaultGet(struct Stream* s, cc_uint32* value) { 
	return ERR_NOT_SUPPORTED;
}
static cc_result Stream_DefaultClose(struct Stream* s) { return 0; }

void Stream_Init(struct Stream* s) {
	s->Read   = Stream_DefaultRead;
	s->ReadU8 = Stream_DefaultReadU8;
	s->Write  = Stream_DefaultWrite;
	s->Skip   = Stream_DefaultSkip;

	s->Seek   = Stream_DefaultSeek;
	s->Position = Stream_DefaultGet;
	s->Length = Stream_DefaultGet;
	s->Close  = Stream_DefaultClose;
}


/*########################################################################################################################*
*-------------------------------------------------------FileStream--------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Stream_FileRead(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return File_Read(s->meta.file, data, count, modified);
}
static cc_result Stream_FileWrite(struct Stream* s, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	return File_Write(s->meta.file, data, count, modified);
}
static cc_result Stream_FileClose(struct Stream* s) {
	cc_result res = File_Close(s->meta.file);
	s->meta.file = 0;
	return res;
}
static cc_result Stream_FileSkip(struct Stream* s, cc_uint32 count) {
	return File_Seek(s->meta.file, count, FILE_SEEKFROM_CURRENT);
}
static cc_result Stream_FileSeek(struct Stream* s, cc_uint32 position) {
	return File_Seek(s->meta.file, position, FILE_SEEKFROM_BEGIN);
}
static cc_result Stream_FilePosition(struct Stream* s, cc_uint32* position) {
	return File_Position(s->meta.file, position);
}
static cc_result Stream_FileLength(struct Stream* s, cc_uint32* length) {
	return File_Length(s->meta.file, length);
}

cc_result Stream_OpenFile(struct Stream* s, const cc_string* path) {
	cc_filepath str;
	Platform_EncodePath(&str, path);

	return Stream_OpenPath(s, &str);
}

cc_result Stream_CreateFile(struct Stream* s, const cc_string* path) {
	cc_filepath str;
	Platform_EncodePath(&str, path);

	return Stream_CreatePath(s, &str);
}

cc_result Stream_OpenPath(struct Stream* s, const cc_filepath* path) {
	cc_file file;
	cc_result res = File_Open(&file, path);

	Stream_FromFile(s, file);
	return res;
}

cc_result Stream_CreatePath(struct Stream* s, const cc_filepath* path) {
	cc_file file;
	cc_result res = File_Create(&file, path);

	Stream_FromFile(s, file);
	return res;
}

cc_result Stream_AppendPath(struct Stream* s, const cc_filepath* path) {
	cc_file file;
	cc_result res;

	if ((res = File_OpenOrCreate(&file, path)))        return res;
	if ((res = File_Seek(file, 0, FILE_SEEKFROM_END))) return res;
	Stream_FromFile(s, file);
	return res;
}

cc_result Stream_WriteAllTo(const cc_string* path, const cc_uint8* data, cc_uint32 length) {
	struct Stream stream;
	cc_result res, closeRes;
	cc_filepath raw_path;

	Platform_EncodePath(&raw_path, path);
	res = Stream_CreatePath(&stream, &raw_path);
	if (res) return res;

	res      = Stream_Write(&stream, data, length);
	closeRes = stream.Close(&stream);
	return res ? res : closeRes;
}

void Stream_FromFile(struct Stream* s, cc_file file) {
	Stream_Init(s);
	s->meta.file = file;

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
static cc_result Stream_PortionRead(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct Stream* source;
	cc_result res;

	count  = min(count, s->meta.portion.left);
	source = s->meta.portion.source;

	res = source->Read(source, data, count, modified);
	s->meta.portion.left -= *modified;
	return res;
}

static cc_result Stream_PortionReadU8(struct Stream* s, cc_uint8* data) {
	struct Stream* source;
	if (!s->meta.portion.left) return ERR_END_OF_STREAM;
	source = s->meta.portion.source;

	s->meta.portion.left--;
	return source->ReadU8(source, data);
}

static cc_result Stream_PortionSkip(struct Stream* s, cc_uint32 count) {
	struct Stream* source;
	cc_result res;

	if (count > s->meta.portion.left) return ERR_INVALID_ARGUMENT;
	source = s->meta.portion.source;

	res = source->Skip(source, count);
	if (!res) s->meta.portion.left -= count;
	return res;
}

static cc_result Stream_PortionPosition(struct Stream* s, cc_uint32* position) {
	*position = s->meta.portion.length - s->meta.portion.left; 
	return 0;
}

static cc_result Stream_PortionLength(struct Stream* s, cc_uint32* length) {
	*length = s->meta.portion.length; 
	return 0;
}

void Stream_ReadonlyPortion(struct Stream* s, struct Stream* source, cc_uint32 len) {
	Stream_Init(s);
	s->Read     = Stream_PortionRead;
	s->ReadU8   = Stream_PortionReadU8;
	s->Skip     = Stream_PortionSkip;
	s->Position = Stream_PortionPosition;
	s->Length   = Stream_PortionLength;

	s->meta.portion.source = source;
	s->meta.portion.left   = len;
	s->meta.portion.length = len;
}


/*########################################################################################################################*
*-----------------------------------------------------MemoryStream--------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Stream_MemoryRead(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	count = min(count, s->meta.mem.left);
	Mem_Copy(data, s->meta.mem.cur, count);
	
	s->meta.mem.cur  += count; 
	s->meta.mem.left -= count;
	*modified = count;
	return 0;
}

static cc_result Stream_MemoryReadU8(struct Stream* s, cc_uint8* data) {
	if (!s->meta.mem.left) return ERR_END_OF_STREAM;

	*data = *s->meta.mem.cur;
	s->meta.mem.cur++; 
	s->meta.mem.left--;
	return 0;
}

static cc_result Stream_MemorySkip(struct Stream* s, cc_uint32 count) {
	if (count > s->meta.mem.left) return ERR_INVALID_ARGUMENT;

	s->meta.mem.cur  += count; 
	s->meta.mem.left -= count;
	return 0;
}

static cc_result Stream_MemorySeek(struct Stream* s, cc_uint32 position) {
	if (position >= s->meta.mem.length) return ERR_INVALID_ARGUMENT;

	s->meta.mem.cur  = s->meta.mem.base   + position;
	s->meta.mem.left = s->meta.mem.length - position;
	return 0;
}

static cc_result Stream_MemoryPosition(struct Stream* s, cc_uint32* position) {
	*position = s->meta.mem.length - s->meta.mem.left; return 0;
}
static cc_result Stream_MemoryLength(struct Stream* s, cc_uint32* length) {
	*length = s->meta.mem.length; return 0;
}

void Stream_ReadonlyMemory(struct Stream* s, void* data, cc_uint32 len) {
	Stream_Init(s);
	s->Read     = Stream_MemoryRead;
	s->ReadU8   = Stream_MemoryReadU8;
	s->Skip     = Stream_MemorySkip;
	s->Seek     = Stream_MemorySeek;
	s->Position = Stream_MemoryPosition;
	s->Length   = Stream_MemoryLength;

	s->meta.mem.cur    = (cc_uint8*)data;
	s->meta.mem.left   = len;
	s->meta.mem.length = len;
	s->meta.mem.base   = (cc_uint8*)data;
}


/*########################################################################################################################*
*----------------------------------------------------BufferedStream-------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Stream_BufferedRead(struct Stream* s, cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct Stream* source;
	cc_uint32 read;
	cc_result res;

	/* Refill buffer */
	if (!s->meta.buffered.left) {
		source               = s->meta.buffered.source; 
		s->meta.buffered.cur = s->meta.buffered.base;

		res = source->Read(source, s->meta.buffered.cur, s->meta.buffered.length, &read);
		if (res) return res;
		s->meta.buffered.left  = read;
		s->meta.buffered.end  += read;
	}
	
	count = min(count, s->meta.buffered.left);
	Mem_Copy(data, s->meta.buffered.cur, count);

	s->meta.buffered.cur  += count; 
	s->meta.buffered.left -= count;
	*modified = count;
	return 0;
}

static cc_result Stream_BufferedReadU8(struct Stream* s, cc_uint8* data) {
	if (!s->meta.buffered.left) return Stream_DefaultReadU8(s, data);

	*data = *s->meta.buffered.cur;
	s->meta.buffered.cur++; 
	s->meta.buffered.left--;
	return 0;
}

static cc_result Stream_BufferedSeek(struct Stream* s, cc_uint32 position) {
	struct Stream* source;
	cc_uint32 beg, len, offset;
	cc_result res;

	/* Check if seek position is within cached buffer */
	len = s->meta.buffered.left + (cc_uint32)(s->meta.buffered.cur - s->meta.buffered.base);
	beg = s->meta.buffered.end  - len;

	if (position >= beg && position < beg + len) {
		offset = position - beg;
		s->meta.buffered.cur  = s->meta.buffered.base + offset;
		s->meta.buffered.left = len - offset;
		return 0;
	}

	source = s->meta.buffered.source;
	res    = source->Seek(source, position);
	if (res) return res;

	s->meta.buffered.cur  = s->meta.buffered.base;
	s->meta.buffered.left = 0;
	s->meta.buffered.end  = position;
	return res;
}

void Stream_ReadonlyBuffered(struct Stream* s, struct Stream* source, void* data, cc_uint32 size) {
	Stream_Init(s);
	s->Read   = Stream_BufferedRead;
	s->ReadU8 = Stream_BufferedReadU8;
	s->Seek   = Stream_BufferedSeek;

	s->meta.buffered.left   = 0;
	s->meta.buffered.end    = 0;
	s->meta.buffered.cur    = (cc_uint8*)data;
	s->meta.buffered.base   = (cc_uint8*)data;
	s->meta.buffered.length = size;
	s->meta.buffered.source = source;
}


/*########################################################################################################################*
*-----------------------------------------------------CRC32Stream---------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Stream_Crc32Write(struct Stream* stream, const cc_uint8* data, cc_uint32 count, cc_uint32* modified) {
	struct Stream* source;
	cc_uint32 i, crc32 = stream->meta.crc32.crc32;

	/* TODO: Optimise this calculation */
	for (i = 0; i < count; i++) {
		crc32 = Utils_Crc32Table[(crc32 ^ data[i]) & 0xFF] ^ (crc32 >> 8);
	}
	stream->meta.crc32.crc32 = crc32;

	source = stream->meta.crc32.source;
	return source->Write(source, data, count, modified);
}

void Stream_WriteonlyCrc32(struct Stream* s, struct Stream* source) {
	Stream_Init(s);
	s->Write = Stream_Crc32Write;

	s->meta.crc32.source = source;
	s->meta.crc32.crc32  = 0xFFFFFFFFUL;
}


/*########################################################################################################################*
*-------------------------------------------------Read/Write primitives---------------------------------------------------*
*#########################################################################################################################*/
cc_uint16 Stream_GetU16_LE(const cc_uint8* data) {
	return (cc_uint16)(data[0] | (data[1] << 8));
}

cc_uint16 Stream_GetU16_BE(const cc_uint8* data) {
	return (cc_uint16)((data[0] << 8) | data[1]);
}

cc_uint32 Stream_GetU32_LE(const cc_uint8* data) {
	return (cc_uint32)(
		 (cc_uint32)data[0]        | ((cc_uint32)data[1] << 8) |
		((cc_uint32)data[2] << 16) | ((cc_uint32)data[3] << 24));
}

cc_uint32 Stream_GetU32_BE(const cc_uint8* data) {
	return (cc_uint32)(
		((cc_uint32)data[0] << 24) | ((cc_uint32)data[1] << 16) |
		((cc_uint32)data[2] << 8)  |  (cc_uint32)data[3]);
}

void Stream_SetU16_LE(cc_uint8* data, cc_uint16 value) {
	data[0] = (cc_uint8)(value      ); data[1] = (cc_uint8)(value >> 8 );
}

void Stream_SetU16_BE(cc_uint8* data, cc_uint16 value) {
	data[0] = (cc_uint8)(value >> 8 ); data[1] = (cc_uint8)(value      );
}

void Stream_SetU32_LE(cc_uint8* data, cc_uint32 value) {
	data[0] = (cc_uint8)(value      ); data[1] = (cc_uint8)(value >> 8 );
	data[2] = (cc_uint8)(value >> 16); data[3] = (cc_uint8)(value >> 24);
}

void Stream_SetU32_BE(cc_uint8* data, cc_uint32 value) {
	data[0] = (cc_uint8)(value >> 24); data[1] = (cc_uint8)(value >> 16);
	data[2] = (cc_uint8)(value >> 8 ); data[3] = (cc_uint8)(value);
}

cc_result Stream_ReadU32_LE(struct Stream* s, cc_uint32* value) {
	cc_uint8 data[4]; cc_result res;
	if ((res = Stream_Read(s, data, 4))) return res;
	*value = Stream_GetU32_LE(data); return 0;
}

cc_result Stream_ReadU32_BE(struct Stream* s, cc_uint32* value) {
	cc_uint8 data[4]; cc_result res;
	if ((res = Stream_Read(s, data, 4))) return res;
	*value = Stream_GetU32_BE(data); return 0;
}


/*########################################################################################################################*
*--------------------------------------------------Read/Write strings-----------------------------------------------------*
*#########################################################################################################################*/
cc_result Stream_ReadLine(struct Stream* s, cc_string* text) {
	cc_bool readAny = false;
	cc_codepoint cp;
	cc_result res;

	cc_uint8 tmp[8];
	cc_uint32 len;

	text->length = 0;
	for (;;) {
		len = 0;

		/* Read a UTF8 character from the stream */
		/* (in most cases it's just one byte) */
		do {
			if ((res = s->ReadU8(s, &tmp[len]))) break;
			len++;
		} while (!Convert_Utf8ToCodepoint(&cp, tmp, len));

		if (res == ERR_END_OF_STREAM) break;
		if (res) return res;

		readAny = true;
		/* Handle \r\n or \n line endings */
		if (cp == '\r') continue;
		if (cp == '\n') return 0;

		/* ignore byte order mark */
		if (cp == 0xFEFF) continue;
		String_Append(text, Convert_CodepointToCP437(cp));
	}
	return readAny ? 0 : ERR_END_OF_STREAM;
}

cc_result Stream_WriteLine(struct Stream* s, cc_string* text) {
	cc_uint8 buffer[2048 + 10]; /* some space for newline */
	const char* nl;
	cc_uint8* cur;
	cc_result res;
	int i, len = 0;

	for (i = 0; i < text->length; i++) {
		/* For extremely large strings */
		if (len >= 2048) {
			if ((res = Stream_Write(s, buffer, len))) return res;
			len = 0;
		}

		cur = buffer + len;
		len += Convert_CP437ToUtf8(text->buffer[i], cur);
	}
	
	nl = _NL;
	while (*nl) { buffer[len++] = *nl++; }
	return Stream_Write(s, buffer, len);
}
