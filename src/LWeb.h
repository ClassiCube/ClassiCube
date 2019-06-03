#ifndef CC_LWEB_H
#define CC_LWEB_H
#include "Http.h"
#include "String.h"
/* Implements asynchronous web tasks for the launcher.
	Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

struct JsonContext;
typedef void (*JsonOnValue)(struct JsonContext* ctx, const String* v);
typedef void (*JsonOnNew)(struct JsonContext* ctx);

/* State for parsing JSON text */
struct JsonContext {
	char* Cur;     /* Pointer to current character in JSON stream being inspected. */
	int Left;      /* Number of characters left to be inspected. */
	bool Failed;   /* Whether there was an error parsing the JSON. */
	String CurKey; /* Key/Name of current member */
	
	JsonOnNew OnNewArray;  /* Invoked when start of an array is read. */
	JsonOnNew OnNewObject; /* Invoked when start of an object is read. */
	JsonOnValue OnValue;   /* Invoked on each member value in an object/array. */
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
	String Hash, Name, IP, Mppass, Software, Country;
	int Players, MaxPlayers, Port, Uptime;
	bool Featured;
	int _order; /* (internal) order in servers table after filtering */
	char _hashBuffer[32],   _nameBuffer[STRING_SIZE];
	char _ipBuffer[16],     _mppassBuffer[STRING_SIZE];
	char _countryBuffer[2], _softBuffer[STRING_SIZE];
};

struct LWebTask;
struct LWebTask {
	bool Completed; /* Whether the task has finished executing. */
	bool Working;   /* Whether the task is currently in progress, or is scheduled to be. */
	bool Success;   /* Whether the task completed successfully. */
	ReturnCode Res; /* Error returned (e.g. for DNS failure) */
	int Status;     /* HTTP return code for the request */
	
	String Identifier; /* Unique identifier for this web task. */
	String URL;        /* URL this task is downloading from/uploading to. */
	TimeMS Start;      /* Point in time this task was started at. */
	/* Called when task successfully downloaded/uploaded data. */
	void (*Handle)(uint8_t* data, uint32_t len);
};
void LWebTask_Tick(struct LWebTask* task);


extern struct GetTokenTaskData {
	struct LWebTask Base;
	String Token; /* Random CSRF token for logging in. */
} GetTokenTask;
void GetTokenTask_Run(void);

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
	uint16_t* Orders;           /* Order of each server (after sorting) */
	int NumServers;             /* Number of public servers. */
} FetchServersTask;
void FetchServersTask_Run(void);
void FetchServersTask_ResetOrder(void);
#define Servers_Get(i) (&FetchServersTask.Servers[FetchServersTask.Orders[i]])


extern struct CheckUpdateData {
	struct LWebTask Base;
	/* Timestamp latest commit/dev build and release were at. */
	TimeMS DevTimestamp, RelTimestamp;
	/* Version of latest release. */
	String LatestRelease;
} CheckUpdateTask; /* TODO: Work out the JSON for this.. */
void CheckUpdateTask_Run(void);


extern struct FetchUpdateData {
	struct LWebTask Base;
	/* Timestamp downloaded build was originally built at. */
	TimeMS Timestamp;
} FetchUpdateTask;
void FetchUpdateTask_Run(bool release, bool d3d9);


extern struct FetchFlagsData { 
	struct LWebTask Base;
	/* Number of flags downloaded. */
	int Count;
} FetchFlagsTask;
void FetchFlagsTask_Add(const String* name);
/* Gets the downloaded bitmap for the given flag. */
Bitmap* Flags_Get(const String* name);
/* Frees all flag bitmaps. */
void Flags_Free(void);
#endif
