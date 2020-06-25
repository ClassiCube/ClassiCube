#ifndef CC_LOGGER_H
#define CC_LOGGER_H
#include "String.h"
/* Support methods for logging errors.
   Copyright 2014-2020 ClassiCube | Licensed under BSD-3
*/

typedef cc_bool (*Logger_DescribeError)(cc_result res, String* dst);
typedef void (*Logger_DoWarn)(const String* msg);
/* Informs the user about a non-fatal error. */
/* By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_DoWarn Logger_WarnFunc;
/* The title shown for warning dialogs. */
extern const char* Logger_DialogTitle;
/* Shows a warning message box with the given message. */
void Logger_DialogWarn(const String* msg);

/* Informs the user about a non-fatal error. */
/* Format: "Error [result] when [place]" */
void Logger_SimpleWarn(cc_result res, const char* place);
/* Informs the user about a non-fatal error. */
/* Format: "Error [result] when [place] '[path]'" */
void Logger_SimpleWarn2(cc_result res, const char* place, const String* path);
/* Informs the user about a non-fatal error, with an optional meaning message. */
/* Format: "Error [result] when [place]  \n  Error meaning: [desc]" */
void Logger_SysWarn(cc_result res, const char* place, Logger_DescribeError describeErr);
/* Informs the user about a non-fatal error, with an optional meaning message. */
/* Format: "Error [result] when [place] '[path]'  \n  Error meaning: [desc]" */
void Logger_SysWarn2(cc_result res, const char* place, const String* path, Logger_DescribeError describeErr);

/* Shows a warning for a failed DynamicLib_Load2/Get2 call. */
/* Format: "Error [place] '[path]'  \n  Error meaning: [desc]" */
void Logger_DynamicLibWarn(const char* place, const String* path);
/* Shortcut for Logger_SysWarn with Platform_DescribeError */
void Logger_Warn(cc_result res, const char* place);
/* Shortcut for Logger_SysWarn2 with Platform_DescribeError */
void Logger_Warn2(cc_result res, const char* place, const String* path);

/* Hooks the operating system's unhandled error callback/signal. */
/* This is used to attempt to log some information about a crash due to invalid memory read, etc. */
void Logger_Hook(void);
/* Generates a backtrace based on the platform-specific CPU context. */
void Logger_Backtrace(String* trace, void* ctx);
/* Logs a message to client.log on disc. */
/* NOTE: The message is written raw, it is NOT converted to unicode (unlike Stream_WriteLine) */
void Logger_Log(const String* msg);
/* Displays a message box with raw_msg body, logs state to disc, then immediately terminates/quits. */
/* Typically used to abort due to an unrecoverable error. (e.g. out of memory) */
void Logger_Abort(const char* raw_msg);
/* Displays a message box with raw_msg body, logs state to disc, then immediately terminates/quits. */
/* Typically used to abort due to an unrecoverable error. (e.g. out of memory) */
CC_NOINLINE void Logger_Abort2(cc_result result, const char* raw_msg);
#endif
