#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "Typedefs.h"
#include "String.h"
/* Support methods for checking and handling errors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Initalises required state for this error handler. */
void ErrorHandler_Init(void);
/* Logs a message to the error handler's log file. */
void ErrorHandler_Log(STRING_PURE String* msg);
/* Checks that the return code of a method is successful. */
#define ErrorHandler_Check(returnCode) (returnCode == 0)
/* Shows a message box to user, logs message to disc, then fails.
NOTE: raw pointer is used here for performance reasons, DO NOT apply this style elsewhere.*/
void ErrorHandler_Fail(const UInt8* raw_msg);
/* Shows a message box to user, logs message to disc, then fails.
NOTE: raw pointer is used here for performance reasons, DO NOT apply this style elsewhere.*/
void ErrorHandler_FailWithCode(ReturnCode returnCode, const UInt8* raw_msg);
/* Checks that the return code of a method is successful. Calls ErrorHandler_Fail if not.
NOTE: raw pointer is used here for performance reasons, DO NOT apply this style elsewhere.*/
#define ErrorHandler_CheckOrFail(returnCode, raw_msg) if (returnCode != 0) { ErrorHandler_FailWithCode(returnCode, raw_msg); }
#endif