#ifndef CC_UTILS_H
#define CC_UTILS_H
#include "String.h"
/* Implements various utility functions.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents a particular instance in time in some timezone. Not necessarily UTC time. */
typedef struct DateTime_ {
	UInt16 Year;  /* Year,   ranges from 0 to 65535 */
	UInt8 Month;  /* Month,  ranges from 1 to 12 */
	UInt8 Day;    /* Day,    ranges from 1 to 31 */
	UInt8 Hour;   /* Hour,   ranges from 0 to 23 */
	UInt8 Minute; /* Minute, ranges from 0 to 59 */
	UInt8 Second; /* Second, ranges from 0 to 59 */
	UInt16 Milli; /* Milliseconds, ranges from 0 to 999 */
} DateTime;
struct Bitmap;

#define DATETIME_MILLISECS_PER_SECOND 1000
Int64 DateTime_TotalMs(DateTime* time);
Int64 DateTime_MsBetween(DateTime* start, DateTime* end);
void DateTime_FromTotalMs(DateTime* time, Int64 ms);
void DateTime_HttpDate(DateTime* value, STRING_TRANSIENT String* str);

UInt32 Utils_ParseEnum(STRING_PURE String* text, UInt32 defValue, const char** names, UInt32 namesCount);
bool Utils_IsValidInputChar(char c, bool supportsCP437);
bool Utils_IsUrlPrefix(STRING_PURE String* value, Int32 index);

bool Utils_EnsureDirectory(STRING_PURE const char* dirName);
void Utils_UNSAFE_GetFilename(STRING_TRANSIENT String* str);
Int32 Utils_AccumulateWheelDelta(Real32* accmulator, Real32 delta);
#define Utils_AdjViewDist(value) ((Int32)(1.4142135f * (value)))

UInt8 Utils_GetSkinType(struct Bitmap* bmp);
UInt32 Utils_CRC32(UInt8* data, UInt32 length);
extern UInt32 Utils_Crc32Table[256];
void Utils_Resize(void** buffer, UInt32* maxElems, UInt32 elemSize, UInt32 defElems, UInt32 expandElems);
#endif
