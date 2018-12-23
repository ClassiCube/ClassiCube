#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for logging errors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef void (*Logger_WarnFunc)(ReturnCode result, const char* place);
typedef void (*Logger_Warn2Func)(ReturnCode result, const char* place, const String* path);

/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] */
/* By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_WarnFunc Logger_Warn;
/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] 'path' */
/* By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_Warn2Func Logger_Warn2;

void Logger_DialogWarn(ReturnCode res, const char* place);
void Logger_DialogWarn2(ReturnCode res, const char* place, const String* path);

/* Hooks the operating system's unhandled error callback/signal. */
/* This is used to attempt to log some information about a crash due to invalid memory read, etc. */
void Logger_Hook(void);
/* Logs a message to client.log on disc. */
void Logger_Log(const String* msg);
/* Displays a message box with raw_msg body, logs state to disc, then immediately terminates/quits. */
/* Typically used to abort due to an unrecoverable error. (e.g. out of memory) */
void Logger_Abort(const char* raw_msg);
/* Displays a message box with raw_msg body, logs state to disc, then immediately terminates/quits. */
/* Typically used to abort due to an unrecoverable error. (e.g. out of memory) */
CC_NOINLINE void Logger_Abort2(ReturnCode result, const char* raw_msg);
#endif
