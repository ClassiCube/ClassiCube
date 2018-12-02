#ifndef CC_LWEB_H
#define CC_LWEB_H
#include "AsyncDownloader.h"
#include "String.h"
/* Implements asynchronous web tasks for the launcher.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct LWebTask;
struct LWebTask {
	/* Whether the task has finished executing. */
	bool Completed;
	/* Whether the task completed successfully. */
	bool Success;
	ReturnCode Res;
	/* Unique identifier for this web task. */
	String Identifier;
	/* URL this task is downloading from/uploading to. */
	String URL;
	/* Point in time this task was started at. */
	TimeMS Start;
	/* Function called to begin downloading/uploading. */
	void (*Begin)(struct LWebTask* task);
	/* Called when task successfully downloaded/uploaded data. */
	void (*Handle)(struct LWebTask* task, uint8_t* data, uint32_t len);
};

void LWebTask_StartAsync(struct LWebTask* task);
void LWebTask_Tick(struct LWebTask* task);
#endif
