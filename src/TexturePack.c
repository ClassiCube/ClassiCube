#include "TexturePack.h"
#include "Constants.h"
#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "Bitmap.h"
#include "World.h"
#include "Graphics.h"
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
	uint8_t contents[26];
	uint32_t compressedSize, uncompressedSize;
	int method, pathLen, extraLen;

	String path; char pathBuffer[ZIP_MAXNAMELEN];
	struct Stream portion, compStream;
	struct InflateState inflate;
	ReturnCode res;
	if ((res = Stream_Read(stream, contents, sizeof(contents)))) return res;

	/* contents[0] (2) version needed */
	/* contents[2] (2) flags */
	method = Stream_GetU16_LE(&contents[4]);
	/* contents[6]  (4) last modified */
	/* contents[10] (4) CRC32 */

	compressedSize = Stream_GetU32_LE(&contents[14]);
	if (!compressedSize) compressedSize = entry->CompressedSize;
	uncompressedSize = Stream_GetU32_LE(&contents[18]);
	if (!uncompressedSize) uncompressedSize = entry->UncompressedSize;

	pathLen  = Stream_GetU16_LE(&contents[22]);
	extraLen = Stream_GetU16_LE(&contents[24]);
	if (pathLen > ZIP_MAXNAMELEN) return ZIP_ERR_FILENAME_LEN;

	path = String_Init(pathBuffer, pathLen, pathLen);
	if ((res = Stream_Read(stream, pathBuffer, pathLen))) return res;

	if (!state->SelectEntry(&path)) return 0;
	if ((res = stream->Skip(stream, extraLen))) return res;

	if (method == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		state->ProcessEntry(&path, &portion, entry);
	} else if (method == 8) {
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
	uint8_t contents[42];
	int pathLen, extraLen, commentLen;
	uint32_t extraDataLen;

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

	pathLen    = Stream_GetU16_LE(&contents[24]);
	extraLen   = Stream_GetU16_LE(&contents[26]);
	commentLen = Stream_GetU16_LE(&contents[28]);
	/* contents[30] (2) disk number */
	/* contents[32] (2) internal attributes */
	/* contents[34] (4) external attributes */
	entry->LocalHeaderOffset = Stream_GetU32_LE(&contents[38]);

	extraDataLen = pathLen + extraLen + commentLen;
	return stream->Skip(stream, extraDataLen);
}

static ReturnCode Zip_ReadEndOfCentralDirectory(struct ZipState* state, uint32_t* centralDirectoryOffset) {
	struct Stream* stream = state->Input;
	uint8_t contents[18];

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

enum ZipSig {
	ZIP_SIG_ENDOFCENTRALDIR = 0x06054b50,
	ZIP_SIG_CENTRALDIR      = 0x02014b50,
	ZIP_SIG_LOCALFILEHEADER = 0x04034b50
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
	struct Stream* stream = state->Input;
	uint32_t stream_len, centralDirOffset;
	uint32_t sig = 0;
	int i, count;

	ReturnCode res;
	state->EntriesCount = 0;
	if ((res = stream->Length(stream, &stream_len))) return res;

	/* At -22 for nearly all zips, but try a bit further back in case of comment */
	count = min(257, stream_len);
	for (i = 22; i < count; i++) {
		res = stream->Seek(stream, stream_len - i);
		if (res) return ZIP_ERR_SEEK_END_OF_CENTRAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig == ZIP_SIG_ENDOFCENTRALDIR) break;
	}

	if (sig != ZIP_SIG_ENDOFCENTRALDIR) return ZIP_ERR_NO_END_OF_CENTRAL_DIR;
	res = Zip_ReadEndOfCentralDirectory(state, &centralDirOffset);
	if (res) return res;

	res = stream->Seek(stream, centralDirOffset);
	if (res) return ZIP_ERR_SEEK_CENTRAL_DIR;
	if (state->EntriesCount > ZIP_MAX_ENTRIES) return ZIP_ERR_TOO_MANY_ENTRIES;

	/* Read all the central directory entries */
	for (count = 0; count < state->EntriesCount; count++) {
		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;

		if (sig == ZIP_SIG_CENTRALDIR) {
			res = Zip_ReadCentralDirectory(state, &state->Entries[count]);
			if (res) return res;
		} else if (sig == ZIP_SIG_ENDOFCENTRALDIR) {
			break;
		} else {
			return ZIP_ERR_INVALID_CENTRAL_DIR;
		}
	}

	/* Now read the local file header entries */
	for (i = 0; i < count; i++) {
		struct ZipEntry* entry = &state->Entries[i];
		res = stream->Seek(stream, entry->LocalHeaderOffset);
		if (res) return ZIP_ERR_SEEK_LOCAL_DIR;

		if ((res = Stream_ReadU32_LE(stream, &sig))) return res;
		if (sig != ZIP_SIG_LOCALFILEHEADER) return ZIP_ERR_INVALID_LOCAL_DIR;

		res = Zip_ReadLocalFileHeader(state, entry);
		if (res) return res;
	}
	return 0;
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
#define TEXCACHE_FOLDER "texturecache"
/* Because I didn't store milliseconds in original C# client */
#define TEXCACHE_TICKS_PER_MS 10000
static struct EntryList cache_accepted, cache_denied, cache_eTags, cache_lastModified;

void TextureCache_Init(void) {
	EntryList_Init(&cache_accepted,     TEXCACHE_FOLDER, "acceptedurls.txt", ' ');
	EntryList_Init(&cache_denied,       TEXCACHE_FOLDER, "deniedurls.txt",   ' ');
	EntryList_Init(&cache_eTags,        TEXCACHE_FOLDER, "etags.txt",        ' ');
	EntryList_Init(&cache_lastModified, TEXCACHE_FOLDER, "lastmodified.txt", ' ');
}

bool TextureCache_HasAccepted(const String* url) { return EntryList_Find(&cache_accepted, url) >= 0; }
bool TextureCache_HasDenied(const String* url)   { return EntryList_Find(&cache_denied,   url) >= 0; }

void TextureCache_Accept(const String* url)      { 
	EntryList_Set(&cache_accepted, url, &String_Empty); 
	EntryList_Save(&cache_accepted);
}
void TextureCache_Deny(const String* url)        { 
	EntryList_Set(&cache_denied,   url, &String_Empty); 
	EntryList_Save(&cache_denied);
}

CC_NOINLINE static void TextureCache_MakePath(String* path, const String* url) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));
	String_Format2(path, TEXCACHE_FOLDER "%r%s", &Directory_Separator, &key);
}

bool TextureCache_Has(const String* url) {
	String path; char pathBuffer[FILENAME_SIZE];
	String_InitArray(path, pathBuffer);

	TextureCache_MakePath(&path, url);
	return File_Exists(&path);
}

bool TextureCache_Get(const String* url, struct Stream* stream) {
	String path; char pathBuffer[FILENAME_SIZE];
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, url);
	res = Stream_OpenFile(stream, &path);

	if (res == ReturnCode_FileNotFound) return false;
	if (res) { Chat_LogError2(res, "opening cache for", url); return false; }
	return true;
}

void TexturePack_GetFromTags(const String* url, String* result, struct EntryList* list) {
	String key, value; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));	
	value = EntryList_UNSAFE_Get(list, &key);
	if (value.length) String_AppendString(result, &value);
}

void TextureCache_GetLastModified(const String* url, TimeMS* time) {
	String path;  char pathBuffer[FILENAME_SIZE];
	String entry; char entryBuffer[STRING_SIZE];
	ReturnCode res;

	String_InitArray(entry, entryBuffer);
	TexturePack_GetFromTags(url, &entry, &cache_lastModified);

	if (entry.length && Convert_TryParseUInt64(&entry, time)) {
		*time /= TEXCACHE_TICKS_PER_MS;
	} else {
		String_InitArray(path, pathBuffer);
		TextureCache_MakePath(&path, url);

		res = File_GetModifiedTime_MS(&path, time);
		if (res) { Chat_LogError2(res, "getting last modified time of", url); *time = 0; }
	}
}

void TextureCache_GetETag(const String* url, String* etag) {
	TexturePack_GetFromTags(url, etag, &cache_eTags);
}

void TextureCache_Set(const String* url, uint8_t* data, uint32_t length) {
	String path; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	TextureCache_MakePath(&path, url);
	if (!Utils_EnsureDirectory(TEXCACHE_FOLDER)) return;
	
	res = Stream_CreateFile(&stream, &path);
	if (res) { Chat_LogError2(res, "creating cache for", &path); return; }

	res = Stream_Write(&stream, data, length);
	if (res) { Chat_LogError2(res, "saving data for", url); }

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing cache for", url); }
}

CC_NOINLINE static void TextureCache_SetEntry(const String* url, const String* data, struct EntryList* list) {
	String key; char keyBuffer[STRING_INT_CHARS];
	String_InitArray(key, keyBuffer);

	String_AppendUInt32(&key, Utils_CRC32(url->buffer, url->length));
	EntryList_Set(list, &key, data);
	EntryList_Save(list);
}

void TextureCache_SetETag(const String* url, const String* etag) {
	if (!etag->length) return;
	TextureCache_SetEntry(url, etag, &cache_eTags);
}

void TextureCache_SetLastModified(const String* url, const TimeMS* lastModified) {
	String data; char dataBuffer[STRING_SIZE];
	uint64_t ticks = (*lastModified) * TEXCACHE_TICKS_PER_MS;
	if (!ticks) return;

	String_InitArray(data, dataBuffer);
	String_AppendUInt64(&data, ticks);
	TextureCache_SetEntry(url, &data, &cache_lastModified);
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static void TexturePack_ProcessZipEntry(const String* path, struct Stream* stream, struct ZipEntry* entry) {
	String name = *path; Utils_UNSAFE_GetFilename(&name);
	Event_RaiseEntry(&TextureEvents_FileChanged, stream, &name);
}

static ReturnCode TexturePack_ExtractZip(struct Stream* stream) {
	struct ZipState state;
	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (Gfx_LostContext) return 0;
	
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	return Zip_Extract(&state);
}

void TexturePack_ExtractZip_File(const String* filename) {
	String path; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	String_Format2(&path, "texpacks%r%s", &Directory_Separator, filename);

	res = Stream_OpenFile(&stream, &path);
	if (res) { Chat_LogError2(res, "opening", &path); return; }

	res = TexturePack_ExtractZip(&stream);
	if (res) { Chat_LogError2(res, "extracting", &path); }

	res = stream.Close(&stream);
	if (res) { Chat_LogError2(res, "closing", &path); }
}

ReturnCode TexturePack_ExtractTerrainPng(struct Stream* stream) {
	Bitmap bmp; 
	ReturnCode res = Png_Decode(&bmp, stream);

	if (!res) {
		Event_RaiseVoid(&TextureEvents_PackChanged);
		if (Game_ChangeTerrainAtlas(&bmp)) return 0;
	}

	Mem_Free(bmp.Scan0);
	return res;
}

void TexturePack_ExtractDefault(void) {
	String texPack; char texPackBuffer[STRING_SIZE];

	String_InitArray(texPack, texPackBuffer);
	Game_GetDefaultTexturePack(&texPack);

	TexturePack_ExtractZip_File(&texPack);
	World_TextureUrl.length = 0;
}

void TexturePack_ExtractCurrent(const String* url) {
	static String zipExt = String_FromConst(".zip");
	struct Stream stream;
	bool zip;
	ReturnCode res = 0;

	if (!url->length) { TexturePack_ExtractDefault(); return; }
	
	if (!TextureCache_Get(url, &stream)) {
		/* e.g. 404 errors */
		if (World_TextureUrl.length) TexturePack_ExtractDefault();
	} else {
		if (!String_Equals(url, &World_TextureUrl)) {
			zip = String_ContainsString(url, &zipExt);
			String_Copy(&World_TextureUrl, url);

			res = zip ? TexturePack_ExtractZip(&stream) :
						TexturePack_ExtractTerrainPng(&stream);		
			if (res) Chat_LogError2(res, zip ? "extracting" : "decoding", url);
		}

		res = stream.Close(&stream);
		if (res) { Chat_LogError2(res, "closing cache for", url); }
	}
}

void TexturePack_Extract_Req(struct AsyncRequest* item) {
	String url, etag;
	void* data; uint32_t len;
	struct Stream mem;
	bool png;
	ReturnCode res;

	url  = String_FromRawArray(item->URL);
	String_Copy(&World_TextureUrl, &url);
	data = item->ResultData;
	len  = item->ResultSize;

	etag = String_FromRawArray(item->Etag);
	TextureCache_Set(&url, data, len);
	TextureCache_SetETag(&url, &etag);
	TextureCache_SetLastModified(&url, &item->LastModified);

	Stream_ReadonlyMemory(&mem, data, len);
	png = Png_Detect(data, len);
	res = png ? TexturePack_ExtractTerrainPng(&mem) 
			 : TexturePack_ExtractZip(&mem);

	if (res) Chat_LogError2(res, png ? "decoding" : "extracting", &url);
	ASyncRequest_Free(item);
}
