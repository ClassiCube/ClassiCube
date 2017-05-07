#ifndef CS_ERRORHANDLER_H
#define CS_ERRORHANDLER_H
#include "Typedefs.h"
#include "String.h"
/* Support methods for checking and handling errors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef UInt32 ReturnCode;

/* Initalises required state for this error handler. */
void ErrorHandler_Init();

/* Logs a message to the error handler's log file. */
void* ErrorHandler_Log(String msg);

/* Checks that the return code of a method is successful. */
bool ErrorHandler_Check(ReturnCode returnCode);

/* Shows a message box to user, logs message to disc, then fails. */
void ErrorHandler_Fail(String msg);

/* Checks that the return code of a method is successful. Calls ErrorHandler_Fail if not.
NOTE: raw pointer is used here for performance reasons, DO NOT apply this style elsewhere.*/
void ErrorHandler_CheckOrFail(ReturnCode returnCode, const UInt8* raw_msg);
#endif