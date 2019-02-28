#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for logging errors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef void (*Logger_DoWarn)(const String* msg);
/* Informs the user about a non-fatal error. */
/* By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_DoWarn Logger_WarnFunc;
/* The title shown for warning dialogs. */
extern const char* Logger_DialogTitle;
void Logger_DialogWarn(const String* msg);

/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] */
void Logger_Warn(ReturnCode res, const char* place);
/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] 'path' */
void Logger_Warn2(ReturnCode res, const char* place, const String* path);

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
