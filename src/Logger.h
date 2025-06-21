#ifndef CC_LOGGER_H
#define CC_LOGGER_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Logs warnings/errors and also abstracts platform specific logging for fatal errors
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

typedef cc_bool (*Logger_DescribeError)(cc_result res, cc_string* dst);
typedef void (*Logger_DoWarn)(const cc_string* msg);
/* Informs the user about a non-fatal error. */
/*  By default this shows a message box, but changes to in-game chat when game is running. */
extern Logger_DoWarn Logger_WarnFunc;
/* The title shown for warning dialogs. */
extern const char* Logger_DialogTitle;
/* Shows a warning message box with the given message. */
void Logger_DialogWarn(const cc_string* msg);

/* Format: "Error [result] when [action]  \n  Error meaning: [desc]" */
/* If describeErr returns false, then 'Error meaning' line is omitted. */
void Logger_FormatWarn(cc_string* msg, cc_result res, const char* action, Logger_DescribeError describeErr);
/* Format: "Error [result] when [action] '[path]'  \n  Error meaning: [desc]" */
/* If describeErr returns false, then 'Error meaning' line is omitted. */
void Logger_FormatWarn2(cc_string* msg, cc_result res, const char* action, const cc_string* path, Logger_DescribeError describeErr);
/* Informs the user about a non-fatal error, with an optional meaning message. */
void Logger_Warn(cc_result res, const char* action, Logger_DescribeError describeErr);
/* Informs the user about a non-fatal error, with an optional meaning message. */
void Logger_Warn2(cc_result res, const char* action, const cc_string* path, Logger_DescribeError describeErr);

/* Shortcut for Logger_Warn with no error describer function */
void Logger_SimpleWarn(cc_result res, const char* action);
/* Shorthand for Logger_Warn2 with no error describer function */
void Logger_SimpleWarn2(cc_result res, const char* action, const cc_string* path);
/* Shows a warning for a failed DynamicLib_Load2/Get2 call. */
/* Format: "Error [action] '[path]'  \n  Error meaning: [desc]" */
void Logger_DynamicLibWarn(const char* action, const cc_string* path);
/* Shortcut for Logger_Warn with Platform_DescribeError */
void Logger_SysWarn(cc_result res, const char* action);
/* Shortcut for Logger_Warn2 with Platform_DescribeError */
void Logger_SysWarn2(cc_result res, const char* action, const cc_string* path);

/* Generates a backtrace based on the platform-specific CPU context. */
/* NOTE: The provided CPU context may be modified (e.g on Windows) */
void Logger_Backtrace(cc_string* trace, void* ctx);
/* Logs a message to client.log on disc. */
/* NOTE: The message is written raw, it is NOT converted to unicode (unlike Stream_WriteLine) */
void Logger_Log(const cc_string* msg);

void Logger_Hook(void);
void Logger_DoAbort(cc_result result, const char* raw_msg, void* ctx);
void Logger_FailToStart(const char* raw_msg);

CC_END_HEADER
#endif
