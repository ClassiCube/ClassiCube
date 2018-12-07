#ifndef CC_LWEB_H
#define CC_LWEB_H
#include "AsyncDownloader.h"
#include "String.h"
/* Implements asynchronous web tasks for the launcher.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* State for parsing JSON text */
struct JsonContext {
	char* Cur;     /* Pointer to current character in JSON stream being inspected. */
	int Left;      /* Number of characters left to be inspected. */
	bool Failed;   /* Whether there was an error parsing the JSON. */
	String CurKey; /* Key/Name of current member */
	
	void (*OnNewArray)(void);       /* Invoked when start of an array is read.  */
	void (*OnNewObject)(void);      /* Invoked when start of an object is read. */
	void (*OnValue)(String* value); /* Invoked on each member value in an object/array. */
	String _tmp; /* temp value used for reading String values */
	char _tmpBuffer[STRING_SIZE];
};
/* Initialises state of JSON parser. */
void Json_Init(struct JsonContext* ctx, String* str);
/* Parses the JSON text, invoking callbacks when value/array/objects are read. */
/* NOTE: DO NOT persist the value argument in OnValue. */
void Json_Parse(struct JsonContext* ctx);

/* Represents all known details about a server. */
struct ServerInfo {
	String Hash, Name, Flag, IP, Port, Mppass, Software;
	int Players, MaxPlayers, Uptime;
	bool Featured;
	char _Buffer[7][STRING_SIZE];
};

struct LWebTask;
struct LWebTask {
	bool Completed; /* Whether the task has finished executing. */
	bool Working;   /* Whether the task is currently in progress, or is scheduled to be. */
	bool Success;   /* Whether the task completed successfully. */
	ReturnCode Res;
	
	String Identifier; /* Unique identifier for this web task. */
	String URL;        /* URL this task is downloading from/uploading to. */
	TimeMS Start;      /* Point in time this task was started at. */
	/* Function called to begin downloading/uploading. */
	void (*Begin)(struct LWebTask* task);
	/* Called when task successfully downloaded/uploaded data. */
	void (*Handle)(struct LWebTask* task, uint8_t* data, uint32_t len);
};
void LWebTask_Tick(struct LWebTask* task);


extern struct GetCSRFTokenTaskData {
	struct LWebTask Base;
	String Token; /* Random CSRF token for logging in. */
} GetCSRFTokenTask;
void GetCSRFTokenTask_Run(void);

extern struct SignInTaskData {
	struct LWebTask Base;
	String Username; /* Username to sign in as. Changed to case correct username. */
	String Error;    /* If sign in fails, the reason as to why. */
} SignInTask;
void SignInTask_Run(const String* user, const String* pass);


extern struct FetchServerData {
	struct LWebTask Base;
	struct ServerInfo Server; /* Details about the given server on success. */
} FetchServerTask;
void FetchServerTask_Run(const String* hash);


extern struct FetchServersData {
	struct LWebTask Base;
	struct ServerInfo* Servers; /* List of all public servers on server list. */
	int NumServers;             /* Number of public servers. */
} FetchServersTask;
void FetchServersTask_Run(void);


extern struct UpdateCheckTaskData {
	struct LWebTask Base;
	/* Timestamp latest commit/dev build and release were at. */
	TimeMS DevTimestamp, ReleaseTimestamp;
	/* Version of latest release. */
	String LatestRelease;
} UpdateCheckTask; /* TODO: Work out the JSON for this.. */
void UpdateCheckTask_Run(void);


extern struct UpdateDownloadTaskData {
	struct LWebTask Base;
	uint8_t* Data; /* The raw downloaded executable. */
	uint32_t Size; /* Size of data in bytes. */
} UpdateDownloadTask;
void UpdateDownloadTask_Run(const String* file);


extern struct FetchFlagsTaskData {
	struct LWebTask Base;
	int NumDownloaded; /* Number of flags that have been downloaded. */
	Bitmap* Bitmaps;   /* Raw pixels for each downloaded flag. */
	String* Names;     /* Name for each downloaded flag.*/
} FetchFlagsTask;
void FetchFlagsTask_Run(void);
void FetchFlagsTask_Add(const String* name);
#endif
