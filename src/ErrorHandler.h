#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for checking and handling errors.
   NOTE: Methods here use raw characters pointers, DO NOT apply this style elsewhere.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

void ErrorHandler_Init(const UChar* logFile);
void ErrorHandler_Log(STRING_PURE String* msg);
void ErrorHandler_Fail(const UChar* raw_msg);
void ErrorHandler_FailWithCode(ReturnCode returnCode, const UChar* raw_msg);
#define ErrorHandler_CheckOrFail(returnCode, raw_msg) if (returnCode) { ErrorHandler_FailWithCode(returnCode, raw_msg); }
void ErrorHandler_ShowDialog(const UChar* title, const UChar* msg);
void ErrorHandler_Backtrace(STRING_TRANSIENT String* str);
#endif
