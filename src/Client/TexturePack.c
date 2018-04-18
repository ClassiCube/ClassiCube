#include "Typedefs.h"
#include "String.h"
#include "Constants.h"
#include "Platform.h"
#include "ErrorHandler.h"
#include "Stream.h"
#include "Bitmap.h"

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
	String_Format3(&path, "%s%b%s", &folder, &Platform_DirectorySeparator, &filename);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) return;
	/* TODO: Should we just log failure to save? */
	ErrorHandler_CheckOrFail(result, "EntryList_Load - open file");

	Stream stream; Stream_FromFile(&stream, file, &path);
	while (Stream_ReadLine(&stream, &path)) {
		String_TrimStart(&path);
		String_TrimEnd(&path);

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
	String_Format3(&path, "%s%b%s", &folder, &Platform_DirectorySeparator, &filename);

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
	String_Format3(path, "%c%b%s", TEXCACHE_FOLDER, &Platform_DirectorySeparator, &crc32);
}

bool TextureCache_HasUrl(STRING_PURE String* url) {
	String path; TexCache_InitAndMakePath(url);
	return Platform_FileExists(url);
}

bool TextureCache_GetStream(STRING_PURE String* url, Stream* stream) {
	String path; TexCache_InitAndMakePath(url);

	void* file;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result == ReturnCode_FileNotFound) {
		Platform_MemSet(stream, 0, sizeof(Stream));
		return false;
	}

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
		return Platform_FileGetWriteTime(path);
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