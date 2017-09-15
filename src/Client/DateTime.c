#include "DateTime.h"

bool DateTime_IsLeapYear(Int32 year) {
	if ((year % 4) != 0) return false;
	if ((year % 100) != 0) return true;
	return (year % 400) == 0;
}

UInt16 DateTime_TotalDays[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
Int64 DateTime_TotalMilliseconds(DateTime* time) {
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

	Int64 seconds = ((Int64)days) * DATETIME_SECONDS_PER_DAY +
		time->Hour * DATETIME_SECONDS_PER_HOUR +
		time->Minute * DATETIME_SECONDS_PER_MINUTE +
		time->Second;
	return ((Int64)seconds) * DATETIME_MILLISECS_PER_SECOND + time->Milliseconds;
}

Int64 DateTime_MillisecondsBetween(DateTime* start, DateTime* end) {
	Int64 msStart = DateTime_TotalMilliseconds(start);
	Int64 msEnd   = DateTime_TotalMilliseconds(end);
	return msEnd - msStart;
}