#ifndef CC_ERRORHANDLER_H
#define CC_ERRORHANDLER_H
#include "String.h"
/* Support methods for logging errors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef bool (*Logger_DescribeError)(ReturnCode res, String* dst);
typedef void (*Logger_DoWarn)(const String* msg);
/* Informs the user about a non-fatal error. */
/* By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_DoWarn Logger_WarnFunc;
/* The title shown for warning dialogs. */
extern const char* Logger_DialogTitle;
void Logger_DialogWarn(const String* msg);

/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] */
void Logger_OldWarn(ReturnCode res, const char* place);
/* Informs the user about a non-fatal error, with a message of form: "Error [result] when [place] '[path]' */
void Logger_OldWarn2(ReturnCode res, const char* place, const String* path);
/* Informs the user about a non-fatal error, with a message of either: 
"Error [error desc] ([result]) when [place] or "Error [result] when [place] */
void Logger_SysWarn(ReturnCode res, const char* place, Logger_DescribeError describeErr);
/* Informs the user about a non-fatal error, with a message of either:
"Error [error desc] ([result]) when [place] 'path' or "Error [result] when [place] 'path' */
void Logger_SysWarn2(ReturnCode res, const char* place, const String* path, Logger_DescribeError describeErr);

/* Shortcut for Logger_SysWarn2 with DynamicLib_DescribeError */
void Logger_DynamicLibWarn2(ReturnCode res, const char* place, const String* path);
/* Shortcut for Logger_SysWarn with Platform_DescribeError */
void Logger_Warn(ReturnCode res, const char* place);
/* Shortcut for Logger_SysWarn2 with Platform_DescribeError */
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
