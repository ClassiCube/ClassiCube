#include "TexturePack.h"
#include "Constants.h"
#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "Bitmap.h"
#include "World.h"
#include "GraphicsAPI.h"
#include "Event.h"
#include "Game.h"
#include "AsyncDownloader.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Deflate.h"
#include "Stream.h"
#include "Funcs.h"
#include "Errors.h"
#include "Chat.h"

/*########################################################################################################################*
*--------------------------------------------------------ZipEntry---------------------------------------------------------*
*#########################################################################################################################*/
#define ZIP_MAXNAMELEN 512
static ReturnCode Zip_ReadLocalFileHeader(struct ZipState* state, struct ZipEntry* entry) {
	struct Stream* stream = state->Input;
	uint8_t contents[(3 * 2) + (4 * 4) + (2 * 2)];
	ReturnCode res;
	if ((res = Stream_Read(stream, contents, sizeof(contents)))) return res;

	/* contents[0] (2) version needed */
	/* contents[2] (2) flags */
	int method = Stream_GetU16_LE(&contents[4]);
	/* contents[6]  (4) last modified */
	/* contents[10] (4) CRC32 */

	UInt32 compressedSize = Stream_GetU32_LE(&contents[14]);
	if (!compressedSize) compressedSize = entry->CompressedSize;
	UInt32 uncompressedSize = Stream_GetU32_LE(&contents[18]);
	if (!uncompressedSize) uncompressedSize = entry->UncompressedSize;

	int pathLen  = Stream_GetU16_LE(&contents[22]);
	int extraLen = Stream_GetU16_LE(&contents[24]);
	char pathBuffer[ZIP_MAXNAMELEN];

	if (pathLen > ZIP_MAXNAMELEN) return ZIP_ERR_FILENAME_LEN;
	String path = String_Init(pathBuffer, pathLen, pathLen);
	if ((res = Stream_Read(stream, pathBuffer, pathLen))) return res;
	pathBuffer[pathLen] = '\0';

	if (!state->SelectEntry(&path)) return 0;
	if ((res = Stream_Skip(stream, extraLen))) return res;
	struct Stream portion, compStream;

	if (method == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		state->ProcessEntry(&path, &portion, entry);
	} else if (method == 8) {
		struct InflateState inflate;
		Stream_ReadonlyPortion(&portion, stream, compressedSize);
		Inflate_MakeStream(&compStream, &inflate, &portion);
		state->ProcessEntry(&path, &compStream, entry);
	} else {
		Platform_Log1("Unsupported.zip entry compression method: %i", &method);
	}
	return 0;
}

static ReturnCode Zip_ReadCentralDirectory(struct ZipState* state, struct ZipEntry* entry) {
	struct Stream* stream = state->Input;
	uint8_t contents[(4 * 2) + (4 * 4) + (3 * 2) + (2 * 2) + (2 * 4)];
	ReturnCode res;
	if ((res = Stream_Read(stream, contents, sizeof(contents)))) return res;

	/* contents[0] (2) OS */
	/* contents[2] (2) version needed*/
	/* contents[4] (2) flags */
	/* contents[6] (2) compresssion method*/
	/* contents[8] (4) last modified */
	entry->Crc32            = Stream_GetU32_LE(&contents[12]);
	entry->CompressedSize   = Stream_GetU32_LE(&contents[16]);
	entry->UncompressedSize = Stream_GetU32_LE(&contents[20]);

	int pathLen    = Stream_GetU16_LE(&contents[24]);
	int extraLen   = Stream_GetU16_LE(&contents[26]);
	int commentLen = Stream_GetU16_LE(&contents[28]);
	/* contents[30] (2) disk number */
	/* contents[32] (2) internal attributes */
	/* contents[34] (4) external attributes */
	entry->LocalHeaderOffset = Stream_GetU32_LE(&contents[38]);

	UInt32 extraDataLen = pathLen + extraLen + commentLen;
	return Stream_Skip(stream, extraDataLen);
}

static ReturnCode Zip_ReadEndOfCentralDirectory(struct ZipState* state, UInt32* centralDirectoryOffset) {
	struct Stream* stream = state->Input;
	uint8_t contents[(3 * 2) + 2 + (2 * 4) + 2];
	ReturnCode res;
	if ((res = Stream_Read(stream, contents, sizeof(contents)))) return res;

	/* contents[0] (2) disk number */
	/* contents[2] (2) disk number start */
	/* contents[4] (2) disk entries */
	state->EntriesCount = Stream_GetU16_LE(&contents[6]);
	/* contents[8] (4) central directory size */
	*centralDirectoryOffset = Stream_GetU32_LE(&contents[12]);
	/* contents[16] (2) comment length */
	return 0;
}

enum ZIP_SIG {
	ZIP_SIG_ENDOFCENTRALDIR = 0x06054b50,
	ZIP_SIG_CENTRALDIR = 0x02014b50,
	ZIP_SIG_LOCALFILEHEADER = 0x04034b50,
};

static void Zip_DefaultProcessor(const String* path, struct Stream* data, struct ZipEntry* entry) { }
static bool Zip_DefaultSelector(const String* path) { return true; }
void Zip_Init(struct ZipState* state, struct Stream* input) {
	state->Input = input;
	state->EntriesCount = 0;
	state->ProcessEntry = Zip_DefaultProcessor;
	state->SelectEntry  = Zip_DefaultSelector;
}

ReturnCode Zip_Extract(struct ZipState* state) {
	state->EntriesCount = 0;
	struct Stream* stream = state->Input;
	UInt32 sig = 0, stream_len;

	ReturnCode res;
	if ((res = stream->Length(stream, &stream_len))) return res;

	/* At -22 for nearly all zips, but try a bit further back in case of comment */
	int i, len = min(257, stream_len);
	for (i = 22; i < len; i++) {
		res = stream->Seek(stream, -i, STREAM_SEEKFROM_END);
		if (res) return ZIP_ERR_SEEK_END_OF_CENTRAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig == ZIP_SIG_ENDOFCENTRALDIR) break;
	}
	if (sig != ZIP_SIG_ENDOFCENTRALDIR) return ZIP_ERR_NO_END_OF_CENTRAL_DIR;

	UInt32 centralDirOffset;
	res = Zip_ReadEndOfCentralDirectory(state, &centralDirOffset);
	if (res) return res;

	res = stream->Seek(stream, centralDirOffset, STREAM_SEEKFROM_BEGIN);
	if (res) return ZIP_ERR_SEEK_CENTRAL_DIR;
	if (state->EntriesCount > ZIP_MAX_ENTRIES) return ZIP_ERR_TOO_MANY_ENTRIES;

	/* Read all the central directory entries */
	int count = 0;
	while (count < state->EntriesCount) {
		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;

		if (sig == ZIP_SIG_CENTRALDIR) {
			res = Zip_ReadCentralDirectory(state, &state->Entries[count]);
			if (res) return res;
			count++;
		} else if (sig == ZIP_SIG_ENDOFCENTRALDIR) {
			break;
		} else {
			return ZIP_ERR_INVALID_CENTRAL_DIR;
		}
	}

	/* Now read the local file header entries */
	for (i = 0; i < count; i++) {
		struct ZipEntry* entry = &state->Entries[i];
		res = stream->Seek(stream, entry->LocalHeaderOffset, STREAM_SEEKFROM_BEGIN);
		if (res) return ZIP_ERR_SEEK_LOCAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig != ZIP_SIG_LOCALFILEHEADER) return ZIP_ERR_INVALID_LOCAL_DIR;

		res = Zip_ReadLocalFileHeader(state, entry);
		if (res) return res;
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------EntryList--------------------------------------------------------*
*#########################################################################################################################*/
struct EntryList {
	const char* Folder;
	const char* Filename;
	StringsBuffer Entries;
};

static void EntryList_Load(struct EntryList* list) {
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);
	String_Format3(&path, "%c%r%c", list->Folder, &Directory_Separator, list->Filename);

	char lineBuffer[FILENAME_SIZE];
	String line = String_FromArray(lineBuffer);
	ReturnCode res;

	void* file; res = File_Open(&file, &path);
	if (res == ReturnCode_FileNotFound) return;
	if (res) { Chat_LogError2(res, "opening", &path); return; }
	struct Stream stream; Stream_FromFile(&stream, file);
	{
		/* ReadLine reads single byte at a time */
		uint8_t buffer[2048]; struct Stream buffered;
		Stream_ReadonlyBuffered(&buffered, &stream, buffer, sizeof(buffer));

		for (;;) {
			res = Stream_ReadLine(&buffered, &line);
			if (res == ERR_END_OF_STREAM) break;
			if (res) { Chat_LogError2(res, "reading from", &path); break; }

			String_TrimStart(&line);
			String_TrimEnd(&line);

			if (!line.length) continue;
			StringsBuffer_Add(&list->Entries, &line);
		}
	}
	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); }
}

static void EntryList_Save(struct EntryList* list) {
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);
	String_Format3(&path, "%c%r%c", list->Folder, &Directory_Separator, list->Filename);

	ReturnCode res;
	if (!Utils_EnsureDirectory(list->Folder)) return;

	void* file; res = File_Create(&file, &path);
	if (res) { Chat_LogError2(res, "creating", &path); return; }
	struct Stream stream; Stream_FromFile(&stream, file);
	{
		int i;
		for (i = 0; i < list->Entries.Count; i++) {
			String entry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
			res = Stream_WriteLine(&stream, &entry);
			if (res) { Chat_LogError2(res, "writing to", &path); break; }
		}
	}
	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); }
}

static void EntryList_Add(struct EntryList* list, const String* entry) {
	StringsBuffer_Add(&list->Entries, entry);
	EntryList_Save(list);
}

static bool EntryList_Has(struct EntryList* list, const String* entry) {
	int i;
	for (i = 0; i < list->Entries.Count; i++) {
		String curEntry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (String_Equals(&curEntry, entry)) return true;
	}
	return false;
}

static void EntryList_UNSAFE_Make(struct EntryList* list, STRING_REF const char* folder, STRING_REF const char* file) {
	list->Folder = folder;
	list->Filename = file;
	EntryList_Load(list);
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
#define TEXCACHE_FOLDER "texturecache"
/* Because I didn't store milliseconds in original C# client */
#define TEXCACHE_TICKS_PER_MS 10000
struct EntryList cache_accepted, cache_denied, cache_eTags, cache_lastModified;

#define TexCache_InitAndMakePath(url) char pathBuffer[FILENAME_SIZE]; \
String path = String_FromArray(pathBuffer); \
TextureCache_MakePath(&path, url);

#define TexCache_Crc32(url) char crc32Buffer[STRING_INT_CHARS]; \
String crc32 = String_FromArray(crc32Buffer);\
String_AppendUInt32(&crc32, Utils_CRC32(url->buffer, url->length));

void TextureCache_Init(void) {
	EntryList_UNSAFE_Make(&cache_accepted,     TEXCACHE_FOLDER, "acceptedurls.txt");
	EntryList_UNSAFE_Make(&cache_denied,       TEXCACHE_FOLDER, "deniedurls.txt");
	EntryList_UNSAFE_Make(&cache_eTags,        TEXCACHE_FOLDER, "etags.txt");
	EntryList_UNSAFE_Make(&cache_lastModified, TEXCACHE_FOLDER, "lastmodified.txt");
}

bool TextureCache_HasAccepted(const String* url) { return EntryList_Has(&cache_accepted, url); }
bool TextureCache_HasDenied(const String* url)   { return EntryList_Has(&cache_denied,   url); }
void TextureCache_Accept(const String* url) { EntryList_Add(&cache_accepted, url); }
void TextureCache_Deny(const String* url)   { EntryList_Add(&cache_denied,   url); }

static void TextureCache_MakePath(String* path, const String* url) {
	TexCache_Crc32(url);
	String_Format2(path, TEXCACHE_FOLDER "%r%s", &Directory_Separator, &crc32);
}

bool TextureCache_HasUrl(const String* url) {
	TexCache_InitAndMakePath(url);
	return File_Exists(&path);
}

bool TextureCache_GetStream(const String* url, struct Stream* stream) {
	TexCache_InitAndMakePath(url);
	ReturnCode res;

	void* file; res = File_Open(&file, &path);
	if (res == ReturnCode_FileNotFound) return false;

	if (res) { Chat_LogError2(res, "opening cache for", url); return false; }
	Stream_FromFile(stream, file);
	return true;
}

void TexturePack_GetFromTags(const String* url, String* result, struct EntryList* list) {
	TexCache_Crc32(url);
	int i;
	String line, key, value;

	for (i = 0; i < list->Entries.Count; i++) {
		line = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (!String_UNSAFE_Separate(&line, ' ', &key, &value)) continue;

		if (!String_CaselessEquals(&key, &crc32)) continue;
		String_AppendString(result, &value);
	}
}

void TextureCache_GetLastModified(const String* url, TimeMS* time) {
	char entryBuffer[STRING_SIZE];
	String entry = String_FromArray(entryBuffer);
	TexturePack_GetFromTags(url, &entry, &cache_lastModified);
	TimeMS temp;

	if (entry.length && Convert_TryParseUInt64(&entry, &temp)) {
		*time = temp / TEXCACHE_TICKS_PER_MS;
	} else {
		TexCache_InitAndMakePath(url);
		ReturnCode res = File_GetModifiedTime_MS(&path, &temp);

		if (res) { Chat_LogError2(res, "getting last modified time of", url); return; }
		*time = temp;
	}
}

void TextureCache_GetETag(const String* url, String* etag) {
	TexturePack_GetFromTags(url, etag, &cache_eTags);
}

void TextureCache_AddData(const String* url, uint8_t* data, UInt32 length) {
	TexCache_InitAndMakePath(url);
	ReturnCode res;
	if (!Utils_EnsureDirectory(TEXCACHE_FOLDER)) return;

	void* file; res = File_Create(&file, &path);
	if (res) { Chat_LogError2(res, "creating cache for", url); return; }
	struct Stream stream; Stream_FromFile(&stream, file);
	{
		res = Stream_Write(&stream, data, length);
		if (res) { Chat_LogError2(res, "saving data for", url); }
	}
	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing cache for", url); }
}

void TextureCache_AddToTags(const String* url, const String* data, struct EntryList* list) {
	TexCache_Crc32(url);
	char entryBuffer[2048];
	String entry = String_FromArray(entryBuffer);
	String_Format2(&entry, "%s %s", &crc32, data);

	int i;
	for (i = 0; i < list->Entries.Count; i++) {
		String curEntry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (!String_CaselessStarts(&curEntry, &crc32)) continue;

		StringsBuffer_Remove(&list->Entries, i);
		break;
	}
	EntryList_Add(list, &entry);
}

void TextureCache_AddETag(const String* url, const String* etag) {
	if (!etag->length) return;
	TextureCache_AddToTags(url, etag, &cache_eTags);
}

void TextureCache_AddLastModified(const String* url, TimeMS* lastModified) {
	if (!lastModified) return;
	uint64_t ticks = (*lastModified) * TEXCACHE_TICKS_PER_MS;

	char dataBuffer[STRING_SIZE];
	String data = String_FromArray(dataBuffer);
	String_AppendUInt64(&data, ticks);
	TextureCache_AddToTags(url, &data, &cache_lastModified);
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static void TexturePack_ProcessZipEntry(const String* path, struct Stream* stream, struct ZipEntry* entry) {
	String name = *path; Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents_FileChanged, stream, &name);
}

static ReturnCode TexturePack_ExtractZip(struct Stream* stream) {
	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (Gfx_LostContext) return 0;

	struct ZipState state;
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	return Zip_Extract(&state);
}

void TexturePack_ExtractZip_File(const String* filename) {
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);
	String_Format2(&path, "texpacks%r%s", &Directory_Separator, filename);
	ReturnCode res;

	void* file; res = File_Open(&file, &path);
	if (res) { Chat_LogError2(res, "opening", &path); return; }
	struct Stream stream; Stream_FromFile(&stream, file);
	{
		res = TexturePack_ExtractZip(&stream);
		if (res) { Chat_LogError2(res, "extracting", &path); }
	}
	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); }
}

ReturnCode TexturePack_ExtractTerrainPng(struct Stream* stream) {
	Bitmap bmp; 
	ReturnCode res = Bitmap_DecodePng(&bmp, stream);

	if (!res) {
		Event_RaiseVoid(&TextureEvents_PackChanged);
		if (Game_ChangeTerrainAtlas(&bmp)) return 0;
	}

	Mem_Free(bmp.Scan0);
	return res;
}

void TexturePack_ExtractDefault(void) {
	char texPackBuffer[STRING_SIZE];
	String texPack = String_FromArray(texPackBuffer);
	Game_GetDefaultTexturePack(&texPack);

	TexturePack_ExtractZip_File(&texPack);
	World_TextureUrl.length = 0;
}

void TexturePack_ExtractCurrent(const String* url) {
	if (!url->length) { TexturePack_ExtractDefault(); return; }

	struct Stream stream;
	if (!TextureCache_GetStream(url, &stream)) {
		/* e.g. 404 errors */
		if (World_TextureUrl.length) TexturePack_ExtractDefault();
	} else {
		String zipExt = String_FromConst(".zip");
		ReturnCode res = 0;

		if (String_Equals(url, &World_TextureUrl)) {
		} else {
			bool zip = String_ContainsString(url, &zipExt);
			String_Copy(&World_TextureUrl, url);
			const char* operation = zip ? "extracting" : "decoding";
			
			res = zip ? TexturePack_ExtractZip(&stream) :
						TexturePack_ExtractTerrainPng(&stream);		
			if (res) { Chat_LogError2(res, operation, url); }
		}

		res = stream.Close(&stream);
		if (res) { Chat_LogError2(res, "closing cache for", url); }
	}
}

void TexturePack_Extract_Req(struct AsyncRequest* item) {
	String url = String_FromRawArray(item->URL);
	String_Copy(&World_TextureUrl, &url);
	void* data = item->ResultData;
	UInt32 len = item->ResultSize;

	String etag = String_FromRawArray(item->Etag);
	TextureCache_AddData(&url, data, len);
	TextureCache_AddETag(&url, &etag);
	TextureCache_AddLastModified(&url, &item->LastModified);

	struct Stream mem; Stream_ReadonlyMemory(&mem, data, len);
	bool png = Bitmap_DetectPng(data, len);
	ReturnCode res = png ? TexturePack_ExtractTerrainPng(&mem) 
						: TexturePack_ExtractZip(&mem);
	const char* operation = png ? "decoding" : "extracting";

	if (res) { Chat_LogError2(res, operation, &url); }
	ASyncRequest_Free(item);
}
