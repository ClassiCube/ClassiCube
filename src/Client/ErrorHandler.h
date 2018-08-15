#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for checking and handling errors.
   NOTE: Methods here use raw characters pointers, DO NOT apply this style elsewhere.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

void ErrorHandler_Init(const UChar* logFile);
void ErrorHandler_Log(STRING_PURE String* msg);
void ErrorHandler_Log1(const UChar* format, const void* a1);
void ErrorHandler_Log2(const UChar* format, const void* a1, const void* a2);
void ErrorHandler_Log3(const UChar* format, const void* a1, const void* a2, const void* a3);
void ErrorHandler_Log4(const UChar* format, const void* a1, const void* a2, const void* a3, const void* a4);
void ErrorHandler_LogError(ReturnCode result, const UChar* place);

#define ErrorHandler_Check(returnCode) ((returnCode) == 0)
void ErrorHandler_Fail(const UChar* raw_msg);
void ErrorHandler_FailWithCode(ReturnCode returnCode, const UChar* raw_msg);
#define ErrorHandler_CheckOrFail(returnCode, raw_msg) if (returnCode != 0) { ErrorHandler_FailWithCode(returnCode, raw_msg); }
void ErrorHandler_ShowDialog(const UChar* title, const UChar* msg);
void ErrorHandler_Backtrace(STRING_TRANSIENT String* str);
#endif
