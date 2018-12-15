#ifndef CC_RESOURCES_H
#define CC_RESOURCES_H
#include "LWeb.h"
/* Implements checking, fetching, and patching the default game assets.
	Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define FLAG_CLASSIC 0x01 /* file depends on classic.jar */
#define FLAG_MODERN  0x02 /* file depends on modern jar */
#define FLAG_GUI     0x04 /* file depends on patched gui.png */
#define FLAG_TERRAIN 0x08 /* file depends on patched terrain.png */

extern struct ResourceFile {
	const char* Filename;
	uint8_t Flags;
	bool Exists;
} Resources_Files[19];

extern bool DigSoundsExist, StepSoundsExist;
/* Number and total size of resources that need to be downloaded. */
extern int Resources_Size, Resources_Count;
/* Returns flags of files that need to be fetched. */
int Resources_GetFetchFlags(void);
/* Checks existence of all assets. */
void Resources_CheckExistence(void);

extern struct ResourceSound {
	const char* Name;
	const char* Hash;
} Resources_Dig[31], Resources_Step[28];

extern struct ResourceMusic {
	const char* Name;
	const char* Hash;
	uint16_t Size;
	bool Exists;
} Resources_Music[7];

typedef void (*FetchResourcesStatus)(const String* status);
extern struct FetchResourcesData {
	struct LWebTask Base;
	FetchResourcesStatus SetStatus;
} FetchResourcesTask;
void FetchResourcesTask_Run(FetchResourcesStatus setStatus);

#endif
