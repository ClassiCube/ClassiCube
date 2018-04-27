#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for checking and handling errors.
   NOTE: Methods here use raw characters pointers, DO NOT apply this style elsewhere.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

void ErrorHandler_Init(const UInt8* logFile);
void ErrorHandler_Log(STRING_PURE String* msg);
#define ErrorHandler_Check(returnCode) ((returnCode) == 0)
void ErrorHandler_Fail(const UInt8* raw_msg);
void ErrorHandler_FailWithCode(ReturnCode returnCode, const UInt8* raw_msg);
#define ErrorHandler_CheckOrFail(returnCode, raw_msg) if (returnCode != 0) { ErrorHandler_FailWithCode(returnCode, raw_msg); }
void ErrorHandler_ShowDialog(const UInt8* title, const UInt8* msg);
void ErrorHandler_Backtrace(STRING_TRANSIENT String* str);
#endif