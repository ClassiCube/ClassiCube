#include "Utils.h"
#include "Constants.h"

bool DateTime_IsLeapYear(Int32 year) {
	if ((year % 4) != 0) return false;
	if ((year % 100) != 0) return true;
	return (year % 400) == 0;
}

UInt16 DateTime_TotalDays[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
Int64 DateTime_TotalMs(DateTime* time) {
	/* Days from before this year */
	Int32 days = 365 * (time->Year - 1), i;
	/* Only need to check leap years for whether days need adding */
	for (i = 4; i < time->Year; i += 4) {
		if (DateTime_IsLeapYear(i)) days++;
	}

	/* Add days in this year */
	days += DateTime_TotalDays[time->Month - 1];
	/* Add Feb 29, if this is a leap year, and day of point in time is after Feb 28 */
	if (DateTime_IsLeapYear(time->Year) && time->Month > 2) days++;
	days += (time->Day - 1);

	Int64 seconds =
		((Int64)days) * DATETIME_SECONDS_PER_DAY +
		time->Hour    * DATETIME_SECONDS_PER_HOUR +
		time->Minute  * DATETIME_SECONDS_PER_MINUTE +
		time->Second;
	return seconds * DATETIME_MILLISECS_PER_SECOND + time->Milli;
}

Int64 DateTime_MsBetween(DateTime* start, DateTime* end) {
	Int64 msStart = DateTime_TotalMs(start);
	Int64 msEnd   = DateTime_TotalMs(end);
	return msEnd - msStart;
}

UInt32 Utils_ParseEnum(STRING_PURE String* text, UInt32 defValue, const UInt8** names, UInt32 namesCount) {
	UInt32 i;
	for (i = 0; i < namesCount; i++) {
		String name = String_FromReadonly(names[i]);
		if (String_CaselessEquals(text, &name)) return i;
	}
	return defValue;
}

bool Utils_IsValidInputChar(UInt8 c, bool supportsCP437) {
	return supportsCP437 || (Convert_CP437ToUnicode(c) == c);
}

bool Utils_IsUrlPrefix(STRING_PURE String* value, Int32 index) {
	String http = String_FromConst("http://");
	String https = String_FromConst("https://");
	return String_IndexOfString(value, &http) == index
		|| String_IndexOfString(value, &https) == index;
}

Int32 Utils_AccumulateWheelDelta(Real32* accmulator, Real32 delta) {
	/* Some mice may use deltas of say (0.2, 0.2, 0.2, 0.2, 0.2) */
	/* We must use rounding at final step, not at every intermediate step. */
	*accmulator += delta;
	Int32 steps = (Int32)(*accmulator);
	*accmulator -= steps;
	return steps;
}

UInt8 Utils_GetSkinType(Bitmap* bmp) {
	if (bmp->Width == bmp->Height * 2) return SKIN_TYPE_64x32;
	if (bmp->Width != bmp->Height)     return SKIN_TYPE_INVALID;

	/* Minecraft alex skins have this particular pixel with alpha of 0 */
	Int32 scale = bmp->Width / 64;
	UInt32 pixel = Bitmap_GetPixel(bmp, 54 * scale, 20 * scale);
	UInt8 alpha = (UInt8)(pixel >> 24);
	return alpha >= 127 ? SKIN_TYPE_64x64 : SKIN_TYPE_64x64_SLIM;
}