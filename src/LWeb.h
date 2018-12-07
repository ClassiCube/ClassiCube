#ifndef CC_LWEB_H
#define CC_LWEB_H
#include "AsyncDownloader.h"
#include "String.h"
/* Implements asynchronous web tasks for the launcher.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents all known details about a server. */
struct ServerListEntry {
	String Hash, Name, Flag, IP, Port, Mppass, Software;
	int Players, MaxPlayers, Uptime;
	bool Featured;
	char _Buffer[7][STRING_SIZE];
};

struct LWebTask;
struct LWebTask {
	/* Whether the task has finished executing. */
	bool Completed;
	/* Whether the task is currently downloading/uploading, or is scheduled to be. */
	bool Working;
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
void LWebTask_Tick(struct LWebTask* task);


extern struct GetCSRFTokenTaskData {
	struct LWebTask Base;
	/* Random CSRF token for logging in. */
	String Token;
} GetCSRFTokenTask;
void GetCSRFTokenTask_Run(void);

extern struct SignInTaskData {
	struct LWebTask Base;
	/* Username to sign in as. Changed to case correct username. */
	String Username;
	/* If sign in fails, the reason as to why. */
	String Error;
} SignInTask;
void SignInTask_Run(const String* user, const String* pass);


extern struct FetchServerData {
	struct LWebTask Base;
	/* Details about the given server on success. */
	struct ServerListEntry Server;
} FetchServerTask;
void FetchServerTask_Run(const String* hash);


extern struct FetchServersData {
	struct LWebTask Base;
	/* List of all public servers on classicube.net server list. */
	struct ServerListEntry* Servers;
	/* Number of public servers that have been fetched. */
	int NumServers;
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
	/* The raw downloaded executable. */
	uint8_t* Data;
	/* Size of data in bytes. */
	uint32_t DataLen;
} UpdateDownloadTask;
void UpdateDownloadTask_Run(const String* file);


extern struct FetchFlagsTaskData {
	struct LWebTask Base;
	/* Number of flags that have been downloaded. */
	int NumDownloaded;
	/* Raw pixels for each downloaded flag. */
	Bitmap* Bitmaps;
	/* Name for each downloaded flag.*/
	String* Names;
} FetchFlagsTask;
void FetchFlagsTask_Run(void);
void FetchFlagsTask_Add(const String* name);
#endif
