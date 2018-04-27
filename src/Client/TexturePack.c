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

/*########################################################################################################################*
*--------------------------------------------------------ZipEntry---------------------------------------------------------*
*#########################################################################################################################*/
String Zip_ReadFixedString(Stream* stream, UInt8* buffer, UInt16 length) {
	String fileName;
	fileName.buffer = buffer;
	fileName.length = length;
	fileName.capacity = length;

	Stream_Read(stream, buffer, length);
	buffer[length] = NULL; /* Ensure null terminated */
	return fileName;
}

void Zip_ReadLocalFileHeader(ZipState* state, ZipEntry* entry) {
	Stream* stream = state->Input;
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
	UInt8 filenameBuffer[String_BufferSize(UInt16_MaxValue)];
	String filename = Zip_ReadFixedString(stream, filenameBuffer, fileNameLen);
	if (!state->SelectEntry(&filename)) return;

	ReturnCode code = Stream_Skip(stream, extraFieldLen);
	ErrorHandler_CheckOrFail(code, "Zip - skipping local header extra");
	if (versionNeeded > 20) {
		Int32 version = versionNeeded;
		Platform_Log1("May not be able to properly extract a .zip enty with version %i", &version);
	}

	Stream portion, compStream;
	if (compressionMethod == 0) {
		Stream_ReadonlyPortion(&portion, stream, uncompressedSize);
		state->ProcessEntry(&filename, &portion, entry);
	} else if (compressionMethod == 8) {
		InflateState inflate;
		Stream_ReadonlyPortion(&portion, stream, compressedSize);
		Inflate_MakeStream(&compStream, &inflate, &portion);
		state->ProcessEntry(&filename, &compStream, entry);
	} else {
		Int32 method = compressionMethod;
		Platform_Log1("Unsupported.zip entry compression method: %i", &method);
	}
}

void Zip_ReadCentralDirectory(ZipState* state, ZipEntry* entry) {
	Stream* stream = state->Input;
	Stream_ReadU16_LE(stream); /* OS */
	UInt16 versionNeeded = Stream_ReadU16_LE(stream);
	UInt16 flags = Stream_ReadU16_LE(stream);
	UInt16 compressionMethod = Stream_ReadU16_LE(stream);
	Stream_ReadU32_LE(stream); /* last modified */
	entry->Crc32 = Stream_ReadU32_LE(stream);
	entry->CompressedDataSize = Stream_ReadI32_LE(stream);
	entry->UncompressedDataSize = Stream_ReadI32_LE(stream);

	UInt16 fileNameLen = Stream_ReadU16_LE(stream);
	UInt16 extraFieldLen = Stream_ReadU16_LE(stream);
	UInt16 fileCommentLen = Stream_ReadU16_LE(stream);
	UInt16 diskNum = Stream_ReadU16_LE(stream);
	UInt16 internalAttributes = Stream_ReadU16_LE(stream);
	UInt32 externalAttributes = Stream_ReadU32_LE(stream);
	entry->LocalHeaderOffset = Stream_ReadI32_LE(stream);

	UInt32 extraDataLen = fileNameLen + extraFieldLen + fileCommentLen;
	ReturnCode code = Stream_Skip(stream, extraDataLen);
	ErrorHandler_CheckOrFail(code, "Zip - skipping central header extra");
}

void Zip_ReadEndOfCentralDirectory(ZipState* state, Int32* centralDirectoryOffset) {
	Stream* stream = state->Input;
	UInt16 diskNum = Stream_ReadU16_LE(stream);
	UInt16 diskNumStart = Stream_ReadU16_LE(stream);
	UInt16 diskEntries = Stream_ReadU16_LE(stream);
	state->EntriesCount = Stream_ReadU16_LE(stream);
	Int32 centralDirectorySize = Stream_ReadI32_LE(stream);
	*centralDirectoryOffset = Stream_ReadI32_LE(stream);
	UInt16 commentLength = Stream_ReadU16_LE(stream);
}

#define ZIP_ENDOFCENTRALDIR 0x06054b50UL
#define ZIP_CENTRALDIR      0x02014b50UL
#define ZIP_LOCALFILEHEADER 0x04034b50UL

void Zip_DefaultProcessor(STRING_TRANSIENT String* path, Stream* data, ZipEntry* entry) { }
bool Zip_DefaultSelector(STRING_TRANSIENT String* path) { return true; }
void Zip_Init(ZipState* state, Stream* input) {
	state->Input = input;
	state->EntriesCount = 0;
	state->ProcessEntry = Zip_DefaultProcessor;
	state->SelectEntry  = Zip_DefaultSelector;
}

void Zip_Extract(ZipState* state) {
	state->EntriesCount = 0;
	Stream* stream = state->Input;
	ReturnCode result = stream->Seek(stream, -22, STREAM_SEEKFROM_END);
	ErrorHandler_CheckOrFail(result, "ZIP - Seek to end of central directory");

	UInt32 sig = Stream_ReadU32_LE(stream);
	if (sig != ZIP_ENDOFCENTRALDIR) {
		ErrorHandler_Fail("ZIP - Comment in .zip file not supported");
		return;
	}

	Int32 centralDirectoryOffset;
	Zip_ReadEndOfCentralDirectory(state, &centralDirectoryOffset);
	result = stream->Seek(stream, centralDirectoryOffset, STREAM_SEEKFROM_BEGIN);
	ErrorHandler_CheckOrFail(result, "ZIP - Seek to central directory");
	if (state->EntriesCount > ZIP_MAX_ENTRIES) {
		ErrorHandler_Fail("ZIP - Max of 2048 entries supported");
	}

	/* Read all the central directory entries */
	Int32 count = 0;
	while (count < state->EntriesCount) {
		sig = Stream_ReadU32_LE(stream);
		if (sig == ZIP_CENTRALDIR) {
			Zip_ReadCentralDirectory(state, &state->Entries[count]);
			count++;
		} else if (sig == ZIP_ENDOFCENTRALDIR) {
			break;
		} else {
			String sigMsg = String_FromConst("ZIP - Unsupported signature found, aborting");
			ErrorHandler_Log(&sigMsg);
			return;
		}
	}

	/* Now read the local file header entries */
	Int32 i;
	for (i = 0; i < count; i++) {
		ZipEntry* entry = &state->Entries[i];
		result = stream->Seek(stream, entry->LocalHeaderOffset, STREAM_SEEKFROM_BEGIN);
		ErrorHandler_CheckOrFail(result, "ZIP - Seek to local file header");

		sig = Stream_ReadU32_LE(stream);
		if (sig != ZIP_LOCALFILEHEADER) {
			String sigMsg = String_FromConst("ZIP - Invalid entry found, skipping");
			ErrorHandler_Log(&sigMsg);
			continue;
		}
		Zip_ReadLocalFileHeader(state, entry);
	}
}


/*########################################################################################################################*
*--------------------------------------------------------EntryList--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct EntryList_ {
	UInt8 FolderBuffer[String_BufferSize(STRING_SIZE)];
	UInt8 FileBuffer[String_BufferSize(STRING_SIZE)];
	StringsBuffer Entries;
} EntryList;

void EntryList_Load(EntryList* list) {
	String folder = String_FromRawArray(list->FolderBuffer);
	String filename = String_FromRawArray(list->FileBuffer);
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format3(&path, "%s%r%s", &folder, &Platform_DirectorySeparator, &filename);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) return;
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "EntryList_Load - open file");

	Stream stream; Stream_FromFile(&stream, file, &path);
	while (Stream_ReadLine(&stream, &path)) {
		String_UNSAFE_TrimStart(&path);
		String_UNSAFE_TrimEnd(&path);

		if (path.length == 0) continue;
		StringsBuffer_Add(&list->Entries, &path);
	}

	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "EntryList_Load - close file");
}

void EntryList_Save(EntryList* list) {
	String folder = String_FromRawArray(list->FolderBuffer);
	String filename = String_FromRawArray(list->FileBuffer);
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
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

	Stream stream; Stream_FromFile(&stream, file, &path);
	Int32 i;
	for (i = 0; i < list->Entries.Count; i++) {
		String entry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		Stream_WriteLine(&stream, &entry);
	}

	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "EntryList_Save - close file");
}

void EntryList_Add(EntryList* list, STRING_PURE String* entry) {
	StringsBuffer_Add(&list->Entries, entry);
	EntryList_Save(list);
}

bool EntryList_Has(EntryList* list, STRING_PURE String* entry) {
	Int32 i;
	for (i = 0; i < list->Entries.Count; i++) {
		String curEntry = StringsBuffer_UNSAFE_Get(&list->Entries, i);
		if (String_Equals(&curEntry, entry)) return true;
	}
	return false;
}

void EntryList_Make(EntryList* list, STRING_PURE const UInt8* folder, STRING_PURE const UInt8* file) {
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
EntryList cache_accepted, cache_denied, cache_eTags, cache_lastModified;

#define TexCache_InitAndMakePath(url) \
UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)]; \
path = String_InitAndClearArray(pathBuffer); \
TextureCache_MakePath(&path, url);

#define TexCache_Crc32(url) \
UInt8 crc32Buffer[STRING_INT_CHARS];\
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

void TextureCache_MakePath(STRING_TRANSIENT String* path, STRING_PURE String* url) {
	String crc32; TexCache_Crc32(url);
	String_Format2(path, TEXCACHE_FOLDER "%r%s", &Platform_DirectorySeparator, &crc32);
}

bool TextureCache_HasUrl(STRING_PURE String* url) {
	String path; TexCache_InitAndMakePath(url);
	return Platform_FileExists(url);
}

bool TextureCache_GetStream(STRING_PURE String* url, Stream* stream) {
	String path; TexCache_InitAndMakePath(url);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) return false;

	ErrorHandler_CheckOrFail(result, "TextureCache - GetStream");
	Stream_FromFile(stream, file, &path);
	return true;
}

void TexturePack_GetFromTags(STRING_PURE String* url, STRING_TRANSIENT String* result, EntryList* list) {
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
	UInt8 entryBuffer[String_BufferSize(STRING_SIZE)];
	String entry = String_InitAndClearArray(entryBuffer);
	TexturePack_GetFromTags(url, &entry, &cache_lastModified);

	Int64 ticks;
	if (entry.length > 0 && Convert_TryParseInt64(&entry, &ticks)) {
		*time = DateTime_FromTotalMs(ticks / TEXCACHE_TICKS_PER_MS);
	} else {
		String path; TexCache_InitAndMakePath(url);
		ReturnCode result = Platform_FileGetWriteTime(&path, time);
		ErrorHandler_CheckOrFail(result, "TextureCache - get file last modified time")
	}
}

void TextureCache_GetETag(STRING_PURE String* url, STRING_PURE String* etag) {
	TexturePack_GetFromTags(url, etag, &cache_eTags);
}

void* TextureCache_CreateFile(STRING_PURE String* path) {
	String folder = String_FromConst(TEXCACHE_FOLDER);
	if (!Platform_DirectoryExists(&folder)) {
		ReturnCode dirResult = Platform_DirectoryCreate(&folder);
		ErrorHandler_CheckOrFail(dirResult, "TextureCache_CreateFile - create directory");
	}

	void* file;
	ReturnCode result = Platform_FileCreate(&file, path);
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "TextureCache_CreateFile - open file");
	return file;
}

void TextureCache_AddImage(STRING_PURE String* url, Bitmap* bmp) {
	String path; TexCache_InitAndMakePath(url);
	void* file = TextureCache_CreateFile(&path);
	Stream stream; Stream_FromFile(&stream, file, &path);

	Bitmap_EncodePng(bmp, &stream);
	ReturnCode result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "TextureCache_AddImage - close file");
}

void TextureCache_AddData(STRING_PURE String* url, UInt8* data, UInt32 length) {
	String path; TexCache_InitAndMakePath(url);
	void* file = TextureCache_CreateFile(&path);
	Stream stream; Stream_FromFile(&stream, file, &path);

	Stream_Write(&stream, data, length);
	ReturnCode result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "TextureCache_AddData - close file");
}

void TextureCache_AddToTags(STRING_PURE String* url, STRING_PURE String* data, EntryList* list) {
	String crc32; TexCache_Crc32(url);
	UInt8 entryBuffer[String_BufferSize(2048)];
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

	UInt8 dataBuffer[String_BufferSize(STRING_SIZE)];
	String data = String_InitAndClearArray(dataBuffer);
	String_AppendInt64(&data, ticks);
	TextureCache_AddToTags(url, &data, &cache_lastModified);
}


/*########################################################################################################################*
*-------------------------------------------------------TexturePack-------------------------------------------------------*
*#########################################################################################################################*/
void TexturePack_ProcessZipEntry(STRING_TRANSIENT String* path, Stream* stream, ZipEntry* entry) {
	/* Ignore directories: convert x/name to name and x\name to name. */
	String_MakeLowercase(path);
	String name = *path;
	Int32 i;

	i = String_LastIndexOf(&name, '\\');
	if (i >= 0) { name = String_UNSAFE_SubstringAt(&name, i + 1); }
	i = String_LastIndexOf(&name, '/');
	if (i >= 0) { name = String_UNSAFE_SubstringAt(&name, i + 1); }

	String_Set(&stream->Name, &name);
	Event_RaiseStream(&TextureEvents_FileChanged, stream);
}

void TexturePack_ExtractZip(Stream* stream) {
	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (Gfx_LostContext) return;

	ZipState state;
	Zip_Init(&state, stream);
	state.ProcessEntry = TexturePack_ProcessZipEntry;
	Zip_Extract(&state);
}

void TexturePack_ExtractZip_File(STRING_PURE String* filename) {
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format2(&path, "texpacks%r%s", &Platform_DirectorySeparator, filename);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	ErrorHandler_CheckOrFail(result, "TexturePack_Extract - opening file");

	Stream stream; 
	Stream_FromFile(&stream, file, &path);
	TexturePack_ExtractZip(&stream);

	result = stream.Close(&stream);
	ErrorHandler_CheckOrFail(result, "TexturePack_Extract - closing file");
}

void TexturePack_ExtractTerrainPng(Stream* stream) {
	Bitmap bmp; Bitmap_DecodePng(&bmp, stream);
	Event_RaiseVoid(&TextureEvents_PackChanged);

	if (Game_ChangeTerrainAtlas(&bmp)) return;
	Platform_MemFree(&bmp.Scan0);
}

void TexturePack_ExtractDefault(void) {
	UInt8 texPackBuffer[String_BufferSize(STRING_SIZE)];
	String texPack = String_InitAndClearArray(texPackBuffer);
	Game_GetDefaultTexturePack(&texPack);

	TexturePack_ExtractZip_File(&texPack);
	String_Clear(&World_TextureUrl);
}

void TexturePack_ExtractCurrent(STRING_PURE String* url) {
	if (url->length == 0) { TexturePack_ExtractDefault(); return; }

	Stream stream;
	if (!TextureCache_GetStream(url, &stream)) {
		/* e.g. 404 errors */
		if (World_TextureUrl.length > 0) TexturePack_ExtractDefault();
	} else {
		String zip = String_FromConst(".zip");
		if (String_Equals(url, &World_TextureUrl)) {
		} else if (String_ContainsString(url, &zip)) {
			String_Set(&World_TextureUrl, url);
			TexturePack_ExtractZip(&stream);
		} else {
			String_Set(&World_TextureUrl, url);
			TexturePack_ExtractTerrainPng(&stream);
		}

		ReturnCode result = stream.Close(&stream);
		ErrorHandler_CheckOrFail(result, "TexturePack_ExtractCurrent - slow stream");
	}
}

void TexturePack_ExtractTerrainPng_Req(AsyncRequest* item) {
	if (item->ResultBitmap.Scan0 == NULL) return;
	String url = String_FromRawArray(item->URL);
	String_Set(&World_TextureUrl, &url);
	Bitmap bmp = item->ResultBitmap;

	String etag = String_FromRawArray(item->Etag);
	TextureCache_AddImage(&url, &bmp);
	TextureCache_AddETag(&url, &etag);
	TextureCache_AddLastModified(&url, &item->LastModified);

	Event_RaiseVoid(&TextureEvents_PackChanged);
	if (!Game_ChangeTerrainAtlas(&bmp)) {
		Platform_MemFree(&bmp.Scan0);
	}
}

void TexturePack_ExtractTexturePack_Req(AsyncRequest* item) {
	if (item->ResultData.Ptr == NULL) return;
	String url = String_FromRawArray(item->URL);
	String_Set(&World_TextureUrl, &url);
	void* data = item->ResultData.Ptr;
	UInt32 len = item->ResultData.Size;

	String etag = String_FromRawArray(item->Etag);
	TextureCache_AddData(&url, data, len);
	TextureCache_AddETag(&url, &etag);
	TextureCache_AddLastModified(&url, &item->LastModified);

	String id = String_FromRawArray(item->ID);
	Stream stream; Stream_ReadonlyMemory(&stream, data, len, &id);
	TexturePack_ExtractZip(&stream);
	stream.Close(&stream);
}