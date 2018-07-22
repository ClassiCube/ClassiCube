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

/*########################################################################################################################*
*--------------------------------------------------------ZipEntry---------------------------------------------------------*
*#########################################################################################################################*/
#define ZIP_MAXNAMELEN 512
static String Zip_ReadFixedString(struct Stream* stream, UChar* buffer, UInt16 length) {
	if (length > ZIP_MAXNAMELEN) ErrorHandler_Fail("Zip string too long");
	String fileName = String_Init(buffer, length, length);
	Stream_Read(stream, buffer, length);
	buffer[length] = NULL; /* Ensure null terminated */
	return fileName;
}

static void Zip_ReadLocalFileHeader(struct ZipState* state, struct ZipEntry* entry) {
	struct Stream* stream = state->Input;
	UInt16 versionNeeded = Stream_ReadU16_LE(stream);
	UInt16 flags = Stream_ReadU16_LE(stream);
	UInt16 compressionMethod = Stream_ReadU16_LE(stream);
	Stream_ReadU32_LE(stream); /* last modified */
	Stream_ReadU32_LE(stream); /* CRC32 */

	Int32 compressedSize = Stream_ReadI32_LE(stream);
	if (compressedSize == 0) compressedSize = entry->CompressedDataSize;
	Int32 uncompressedSize = Stream_ReadI32_LE(stream);
	if (uncompressedSize == 0) uncompressedSize = entry->UncompressedDataSize;

	UInt16 fileNameLen = Stream_ReadU16_LE(stream);
	UInt16 extraFieldLen = Stream_ReadU16_LE(stream);
	UChar filenameBuffer[String_BufferSize(ZIP_MAXNAMELEN)];
	String filename = Zip_ReadFixedString(stream, filenameBuffer, fileNameLen);
	if (!state->SelectEntry(&filename)) return;

	ReturnCode code = Stream_Skip(stream, extraFieldLen);
	ErrorHandler_CheckOrFail(code, "Zip - skipping local header extra");
	if (versionNeeded > 20) {
		Int32 version = versionNeeded;
		Platform_Log1("May not be able to properly extract a .zip enty with version %i", &version);
	}

	struct Stream portion, compStream;
	if (compressionMethod == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		state->ProcessEntry(&filename, &portion, entry);
	} else if (compressionMethod == 8) {
		struct InflateState inflate;
		Stream_ReadonlyPortion(&portion, stream, compressedSize);
		Inflate_MakeStream(&compStream, &inflate, &portion);
		state->ProcessEntry(&filename, &compStream, entry);
	} else {
		Int32 method = compressionMethod;
		Platform_Log1("Unsupported.zip entry compression method: %i", &method);
	}
}

static void Zip_ReadCentralDirectory(struct ZipState* state, struct ZipEntry* entry) {
	struct Stream* stream = state->Input;
	Stream_ReadU16_LE(stream); /* OS */
	Stream_ReadU16_LE(stream); /* version needed*/
	Stream_ReadU16_LE(stream); /* flags */
	Stream_ReadU16_LE(stream); /* compresssion method*/
	Stream_ReadU32_LE(stream); /* last modified */
	entry->Crc32 = Stream_ReadU32_LE(stream);
	entry->CompressedDataSize = Stream_ReadI32_LE(stream);
	entry->UncompressedDataSize = Stream_ReadI32_LE(stream);

	UInt16 fileNameLen = Stream_ReadU16_LE(stream);
	UInt16 extraFieldLen = Stream_ReadU16_LE(stream);
	UInt16 fileCommentLen = Stream_ReadU16_LE(stream);
	Stream_ReadU16_LE(stream); /* disk number */
	Stream_ReadU16_LE(stream); /* internal attributes */
	Stream_ReadU32_LE(stream); /* external attributes */
	entry->LocalHeaderOffset = Stream_ReadI32_LE(stream);

	UInt32 extraDataLen = fileNameLen + extraFieldLen + fileCommentLen;
	ReturnCode code = Stream_Skip(stream, extraDataLen);
	ErrorHandler_CheckOrFail(code, "Zip - skipping central header extra");
}

static void Zip_ReadEndOfCentralDirectory(struct ZipState* state, Int32* centralDirectoryOffset) {
	struct Stream* stream = state->Input;
	Stream_ReadU16_LE(stream); /* disk number */
	Stream_ReadU16_LE(stream); /* disk number start */
	Stream_ReadU16_LE(stream); /* disk entries */
	state->EntriesCount = Stream_ReadU16_LE(stream);
	Stream_ReadU32_LE(stream); /* central directory size */
	*centralDirectoryOffset = Stream_ReadI32_LE(stream);
	Stream_ReadU16_LE(stream); /* comment length */
}

enum ZIP_SIG {
	ZIP_SIG_ENDOFCENTRALDIR = 0x06054b50,
	ZIP_SIG_CENTRALDIR = 0x02014b50,
	ZIP_SIG_LOCALFILEHEADER = 0x04034b50,
};

static void Zip_DefaultProcessor(STRING_TRANSIENT String* path, struct Stream* data, struct ZipEntry* entry) { }
static bool Zip_DefaultSelector(STRING_TRANSIENT String* path) { return true; }
void Zip_Init(struct ZipState* state, struct Stream* input) {
	state->Input = input;
	state->EntriesCount = 0;
	state->ProcessEntry = Zip_DefaultProcessor;
	state->SelectEntry  = Zip_DefaultSelector;
}

ReturnCode Zip_Extract(struct ZipState* state) {
	state->EntriesCount = 0;
	struct Stream* stream = state->Input;
	UInt32 sig = 0, stream_len = 0;
	ReturnCode result = stream->Length(stream, &stream_len);

	/* At -22 for nearly all zips, but try a bit further back in case of comment */
	Int32 i, len = min(257, stream_len);
	for (i = 22; i < len; i++) {
		result = stream->Seek(stream, -i, STREAM_SEEKFROM_END);
		if (result != 0) return ZIP_ERR_SEEK_END_OF_CENTRAL_DIR;

		sig = Stream_ReadU32_LE(stream);
		if (sig == ZIP_SIG_ENDOFCENTRALDIR) break;
	}
	if (sig != ZIP_SIG_ENDOFCENTRALDIR) return ZIP_ERR_NO_END_OF_CENTRAL_DIR;

	Int32 centralDirectoryOffset;
	Zip_ReadEndOfCentralDirectory(state, &centralDirectoryOffset);
	result = stream->Seek(stream, centralDirectoryOffset, STREAM_SEEKFROM_BEGIN);
	if (result != 0) return ZIP_ERR_SEEK_CENTRAL_DIR;

	if (state->EntriesCount > ZIP_MAX_ENTRIES) return ZIP_ERR_TOO_MANY_ENTRIES;

	/* Read all the central directory entries */
	Int32 count = 0;
	while (count < state->EntriesCount) {
		sig = Stream_ReadU32_LE(stream);
		if (sig == ZIP_SIG_CENTRALDIR) {
			Zip_ReadCentralDirectory(state, &state->Entries[count]);
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
		result = stream->Seek(stream, entry->LocalHeaderOffset, STREAM_SEEKFROM_BEGIN);
		if (result != 0) return ZIP_ERR_SEEK_LOCAL_DIR;

		sig = Stream_ReadU32_LE(stream);
		if (sig != ZIP_SIG_LOCALFILEHEADER) return ZIP_ERR_INVALID_LOCAL_DIR;
		Zip_ReadLocalFileHeader(state, entry);
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------EntryList--------------------------------------------------------*
*#########################################################################################################################*/
struct EntryList {
	UChar FolderBuffer[String_BufferSize(STRING_SIZE)];
	UChar FileBuffer[String_BufferSize(STRING_SIZE)];
	StringsBuffer Entries;
};

static void EntryList_Load(struct EntryList* list) {
	String folder = String_FromRawArray(list->FolderBuffer);
	String filename = String_FromRawArray(list->FileBuffer);
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format3(&path, "%s%r%s", &folder, &Platform_DirectorySeparator, &filename);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) return;
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "EntryList_Load - open file");
	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		/* ReadLine reads single byte at a time */
		UInt8 buffer[2048]; struct Stream buffered;
		Stream_ReadonlyBuffered(&buffered, &stream, buffer, sizeof(buffer));

		while (Stream_ReadLine(&buffered, &path)) {
			String_UNSAFE_TrimStart(&path);
			String_UNSAFE_TrimEnd(&path);

			if (path.length == 0) continue;
			StringsBuffer_Add(&list->Entries, &path);
		}
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "EntryList_Load - close file");
}

static void EntryList_Save(struct EntryList* list) {
	String folder = String_FromRawArray(list->FolderBuffer);
	String filename = String_FromRawArray(list->FileBuffer);
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format3(&path, "%s%r%s", &folder, &Platform_DirectorySeparator, &filename);

	if (!Platform_DirectoryExists(&folder)) {
		ReturnCode dirResult = Platform_DirectoryCreate(&folder);
		ErrorHandler_CheckOrFail(dirResult, "EntryList_Save - create directory");
	}

	void* file;
	ReturnCode result = Platform_FileCreate(&file, &path);
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "EntryList_Save - open file");
	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		Int32 i;
		for (i = 0; i < list->Entries.Count; i++) {
			String entry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
			Stream_WriteLine(&stream, &entry);
		}
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "EntryList_Save - close file");
}

static void EntryList_Add(struct EntryList* list, STRING_PURE String* entry) {
	StringsBuffer_Add(&list->Entries, entry);
	EntryList_Save(list);
}

static bool EntryList_Has(struct EntryList* list, STRING_PURE String* entry) {
	Int32 i;
	for (i = 0; i < list->Entries.Count; i++) {
		String curEntry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (String_Equals(&curEntry, entry)) return true;
	}
	return false;
}

static void EntryList_Make(struct EntryList* list, STRING_PURE const UChar* folder, STRING_PURE const UChar* file) {
	String dstFolder = String_InitAndClearArray(list->FolderBuffer);
	String_AppendConst(&dstFolder, folder);
	String dstFile = String_InitAndClearArray(list->FileBuffer);
	String_AppendConst(&dstFile, file);

	StringsBuffer_Init(&list->Entries);
	EntryList_Load(list);
}


/*########################################################################################################################*
*------------------------------------------------------TextureCache-------------------------------------------------------*
*#########################################################################################################################*/
#define TEXCACHE_FOLDER "texturecache"
/* Because I didn't store milliseconds in original C# client */
#define TEXCACHE_TICKS_PER_MS 10000LL
struct EntryList cache_accepted, cache_denied, cache_eTags, cache_lastModified;

#define TexCache_InitAndMakePath(url) \
UChar pathBuffer[String_BufferSize(FILENAME_SIZE)]; \
path = String_InitAndClearArray(pathBuffer); \
TextureCache_MakePath(&path, url);

#define TexCache_Crc32(url) \
UChar crc32Buffer[STRING_INT_CHARS];\
crc32 = String_InitAndClearArray(crc32Buffer);\
String_AppendUInt32(&crc32, Utils_CRC32(url->buffer, url->length));

void TextureCache_Init(void) {
	EntryList_Make(&cache_accepted,     TEXCACHE_FOLDER, "acceptedurls.txt");
	EntryList_Make(&cache_denied,       TEXCACHE_FOLDER, "deniedurls.txt");
	EntryList_Make(&cache_eTags,        TEXCACHE_FOLDER, "etags.txt");
	EntryList_Make(&cache_lastModified, TEXCACHE_FOLDER, "lastmodified.txt");
}

bool TextureCache_HasAccepted(STRING_PURE String* url) { return EntryList_Has(&cache_accepted, url); }
bool TextureCache_HasDenied(STRING_PURE String* url)   { return EntryList_Has(&cache_denied,   url); }
void TextureCache_Accept(STRING_PURE String* url) { EntryList_Add(&cache_accepted, url); }
void TextureCache_Deny(STRING_PURE String* url)   { EntryList_Add(&cache_denied,   url); }

static void TextureCache_MakePath(STRING_TRANSIENT String* path, STRING_PURE String* url) {
	String crc32; TexCache_Crc32(url);
	String_Format2(path, TEXCACHE_FOLDER "%r%s", &Platform_DirectorySeparator, &crc32);
}

bool TextureCache_HasUrl(STRING_PURE String* url) {
	String path; TexCache_InitAndMakePath(url);
	return Platform_FileExists(&path);
}

bool TextureCache_GetStream(STRING_PURE String* url, struct Stream* stream) {
	String path; TexCache_InitAndMakePath(url);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) return false;

	ErrorHandler_CheckOrFail(result, "TextureCache - GetStream");
	Stream_FromFile(stream, file, &path);
	return true;
}

void TexturePack_GetFromTags(STRING_PURE String* url, STRING_TRANSIENT String* result, struct EntryList* list) {
	String crc32; TexCache_Crc32(url);
	Int32 i;
	for (i = 0; i < list->Entries.Count; i++) {
		String entry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (!String_CaselessStarts(&entry, &crc32)) continue;

		Int32 sepIndex = String_IndexOf(&entry, ' ', 0);
		if (sepIndex == -1) continue;
		
		String value = String_UNSAFE_SubstringAt(&entry, sepIndex + 1);
		String_AppendString(result, &value);
	}
}

void TextureCache_GetLastModified(STRING_PURE String* url, DateTime* time) {
	UChar entryBuffer[String_BufferSize(STRING_SIZE)];
	String entry = String_InitAndClearArray(entryBuffer);
	TexturePack_GetFromTags(url, &entry, &cache_lastModified);

	Int64 ticks;
	if (entry.length > 0 && Convert_TryParseInt64(&entry, &ticks)) {
		DateTime_FromTotalMs(time, ticks / TEXCACHE_TICKS_PER_MS);
	} else {
		String path; TexCache_InitAndMakePath(url);
		ReturnCode result = Platform_FileGetModifiedTime(&path, time);
		ErrorHandler_CheckOrFail(result, "TextureCache - get file last modified time")
	}
}

void TextureCache_GetETag(STRING_PURE String* url, STRING_PURE String* etag) {
	TexturePack_GetFromTags(url, etag, &cache_eTags);
}

void TextureCache_AddData(STRING_PURE String* url, UInt8* data, UInt32 length) {
	String path; TexCache_InitAndMakePath(url);
	ReturnCode result;

	String folder = String_FromConst(TEXCACHE_FOLDER);
	if (!Platform_DirectoryExists(&folder)) {
		result = Platform_DirectoryCreate(&folder);
		ErrorHandler_CheckOrFail(result, "TextureCache_AddData - create directory");
	}

	void* file; result = Platform_FileCreate(&file, &path);
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "TextureCache_AddData - open file");
	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		Stream_Write(&stream, data, length);
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "TextureCache_AddData - close file");
}

void TextureCache_AddToTags(STRING_PURE String* url, STRING_PURE String* data, struct EntryList* list) {
	String crc32; TexCache_Crc32(url);
	UChar entryBuffer[String_BufferSize(2048)];
	String entry = String_InitAndClearArray(entryBuffer);
	String_Format2(&entry, "%s %s", &crc32, data);

	Int32 i;
	for (i = 0; i < list->Entries.Count; i++) {
		String curEntry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (!String_CaselessStarts(&curEntry, &crc32)) continue;

		StringsBuffer_Remove(&list->Entries, i);
		break;
	}
	EntryList_Add(list, &entry);
}

void TextureCache_AddETag(STRING_PURE String* url, STRING_PURE String* etag) {
	if (etag->length == 0) return;
	TextureCache_AddToTags(url, etag, &cache_eTags);
}

void TextureCache_AddLastModified(STRING_PURE String* url, DateTime* lastModified) {
	if (lastModified->Year == 0 && lastModified->Month == 0) return;
	Int64 ticks = DateTime_TotalMs(lastModified) * TEXCACHE_TICKS_PER_MS;

	UChar dataBuffer[String_BufferSize(STRING_SIZE)];
	String data = String_InitAndClearArray(dataBuffer);
	String_AppendUInt64(&data, ticks);
	TextureCache_AddToTags(url, &data, &cache_lastModified);
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
static void TexturePack_ProcessZipEntry(STRING_TRANSIENT String* path, struct Stream* stream, struct ZipEntry* entry) {
	String_MakeLowercase(path);
	String name = *path; Utils_UNSAFE_GetFilename(&name);
	String_Set(&stream->Name, &name);
	Event_RaiseStream(&TextureEvents_FileChanged, stream);
}

static ReturnCode TexturePack_ExtractZip(struct Stream* stream) {
	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (Gfx_LostContext) return 0;

	struct ZipState state;
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	return Zip_Extract(&state);
}

void TexturePack_ExtractZip_File(STRING_PURE String* filename) {
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format2(&path, "texpacks%r%s", &Platform_DirectorySeparator, filename);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	ErrorHandler_CheckOrFail(result, "TexturePack_Extract - opening file");
	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		result = TexturePack_ExtractZip(&stream);
		ErrorHandler_CheckOrFail(result, "TexturePack_Extract - extract content");
	}
	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "TexturePack_Extract - closing file");
}

ReturnCode TexturePack_ExtractTerrainPng(struct Stream* stream) {
	struct Bitmap bmp; 
	ReturnCode result = Bitmap_DecodePng(&bmp, stream);

	if (result == 0) {
		Event_RaiseVoid(&TextureEvents_PackChanged);
		if (Game_ChangeTerrainAtlas(&bmp)) return 0;
	}

	Platform_MemFree(&bmp.Scan0);
	return result;
}

void TexturePack_ExtractDefault(void) {
	UChar texPackBuffer[String_BufferSize(STRING_SIZE)];
	String texPack = String_InitAndClearArray(texPackBuffer);
	Game_GetDefaultTexturePack(&texPack);

	TexturePack_ExtractZip_File(&texPack);
	String_Clear(&World_TextureUrl);
}

void TexturePack_ExtractCurrent(STRING_PURE String* url) {
	if (url->length == 0) { TexturePack_ExtractDefault(); return; }

	struct Stream stream;
	if (!TextureCache_GetStream(url, &stream)) {
		/* e.g. 404 errors */
		if (World_TextureUrl.length > 0) TexturePack_ExtractDefault();
	} else {
		String zip = String_FromConst(".zip");
		ReturnCode result = 0;

		if (String_Equals(url, &World_TextureUrl)) {
		} else if (String_ContainsString(url, &zip)) {
			String_Set(&World_TextureUrl, url);
			result = TexturePack_ExtractZip(&stream);
		} else {
			String_Set(&World_TextureUrl, url);
			result = TexturePack_ExtractTerrainPng(&stream);
		}
		ErrorHandler_CheckOrFail(result, "TexturePack_ExtractCurrent - extract content");

		result = stream.Close(&stream);
		ErrorHandler_CheckOrFail(result, "TexturePack_ExtractCurrent - close stream");
	}
}

void TexturePack_Extract_Req(struct AsyncRequest* item) {
	String url = String_FromRawArray(item->URL);
	String_Set(&World_TextureUrl, &url);
	void* data = item->ResultData;
	UInt32 len = item->ResultSize;

	String etag = String_FromRawArray(item->Etag);
	TextureCache_AddData(&url, data, len);
	TextureCache_AddETag(&url, &etag);
	TextureCache_AddLastModified(&url, &item->LastModified);

	String id = String_FromRawArray(item->ID);
	struct Stream mem; Stream_ReadonlyMemory(&mem, data, len, &id);

	ReturnCode result;
	if (Bitmap_DetectPng(data, len)) {
		result = TexturePack_ExtractTerrainPng(&mem);
	} else {
		result = TexturePack_ExtractZip(&mem);
	}

	ErrorHandler_CheckOrFail(result, "TexturePack_Extract_Req - extract content");
	ASyncRequest_Free(item);
}