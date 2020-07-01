#ifndef CC_UTILS_H
#define CC_UTILS_H
#include "String.h"
#include "Bitmap.h"
/* Implements various utility functions.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/

/* Represents a particular instance in time in some timezone. Not necessarily UTC time. */
/* NOTE: TimeMS and DateTime_CurrentUTC_MS() should almost always be used instead. */
/* This struct should only be used when actually needed. (e.g. log message time) */
struct DateTime {
	int year;   /* Year,   ranges from 0 to 65535 */
	int month;  /* Month,  ranges from 1 to 12 */
	int day;    /* Day,    ranges from 1 to 31 */
	int hour;   /* Hour,   ranges from 0 to 23 */
	int minute; /* Minute, ranges from 0 to 59 */
	int second; /* Second, ranges from 0 to 59 */
	int __milli;/* Was milliseconds, unused now */
};

#define MILLIS_PER_SEC 1000
#define SECS_PER_MIN 60
#define SECS_PER_HOUR (60 * 60)
#define SECS_PER_DAY (60 * 60 * 24)

CC_NOINLINE int Utils_ParseEnum(const String* text, int defValue, const char* const* names, int namesCount);
/* Returns whether value starts with http:// or https:// */
cc_bool Utils_IsUrlPrefix(const String* value);

/* Creates the directory if it doesn't exist. (logs failure using Logger_Warn2) */
cc_bool Utils_EnsureDirectory(const char* dirName);
/* Gets the filename portion of a path. (e.g. "dir/file.txt" -> "file.txt") */
void Utils_UNSAFE_GetFilename(STRING_REF String* path);
/* Gets rid of first directory in a path. (e.g. "dx/gl/aa.txt" -> "gl/aa.txt" */
void Utils_UNSAFE_TrimFirstDirectory(STRING_REF String* path);
int Utils_AccumulateWheelDelta(float* accumulator, float delta);
#define Utils_AdjViewDist(value) ((int)(1.4142135f * (value)))

cc_uint8 Utils_CalcSkinType(const Bitmap* bmp);
cc_uint32 Utils_CRC32(const cc_uint8* data, cc_uint32 length);
/* CRC32 lookup table, for faster CRC32 calculations. */
/* NOTE: This cannot be just indexed by byte value - see Utils_CRC32 implementation. */
extern const cc_uint32 Utils_Crc32Table[256];
CC_NOINLINE void Utils_Resize(void** buffer, int* capacity, cc_uint32 elemSize, int defCapacity, int expandElems);
CC_NOINLINE cc_bool Utils_ParseIP(const String* ip, cc_uint8* data);

/* Converts blocks of 3 bytes into 4 ASCII characters. (pads if needed) */
/* Returns the number of ASCII characters written. */
/* NOTE: You MUST ensure that dst is appropriately sized. */
int Convert_ToBase64(const cc_uint8* src, int len, char* dst);
/* Converts blocks of 4 ASCII characters into 3 bytes. */
/* Returns the number of bytes written. */
/* NOTE: You MUST ensure that dst is appropriately sized. */
int Convert_FromBase64(const char* src, int len, cc_uint8* dst);

struct EntryList {
	char separator;
	struct StringsBuffer entries;
};
typedef cc_bool (*EntryList_Filter)(const String* entry);

/* Loads the entries from disc. */
CC_NOINLINE void EntryList_Load(struct EntryList* list, const char* file, EntryList_Filter filter);
/* Saves the entries to disc. */
CC_NOINLINE void EntryList_Save(struct EntryList* list, const char* file);
/* Removes the entry whose key caselessly equals the given key. */
CC_NOINLINE int  EntryList_Remove(struct EntryList* list, const String* key);
/* Replaces the entry whose key caselessly equals the given key, or adds a new entry. */
CC_NOINLINE void EntryList_Set(struct EntryList* list, const String* key, const String* value);
/* Returns the value of the entry whose key caselessly equals the given key. */
CC_NOINLINE STRING_REF String EntryList_UNSAFE_Get(struct EntryList* list, const String* key);
/* Finds the index of the entry whose key caselessly equals the given key. */
CC_NOINLINE int EntryList_Find(struct EntryList* list, const String* key);
/* Initialises the EntryList and loads the entries from disc. */
void EntryList_Init(struct EntryList* list, const char* file, char separator);
#endif
