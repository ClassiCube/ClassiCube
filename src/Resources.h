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
	const char* Name;
	const char* Url;
	uint16_t Size;
	uint8_t Flag;
	bool Downloaded;
} Resources_Files[4];

extern struct ResourceTexture {
	const char* Filename;
	uint8_t Flags;
	bool Exists;
} Resources_Textures[19];

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

extern bool DigSoundsExist, StepSoundsExist;
/* Number of resources that need to be downloaded. */
extern int Resources_Count;
/* Total size of resources that need to be downloaded. */
extern int Resources_Size;
/* Returns flags of files that need to be fetched. */
int Resources_GetFetchFlags(void);
/* Checks existence of all assets. */
void Resources_CheckExistence(void);

/* Whether fetcher is currently downloading resources. */
extern bool Fetcher_Working;
/* Whether fethcer has finished. (downloaded all resources, or an error) */
extern bool Fetcher_Completed;
/* Number of resources that have been downloaded so far. */
extern int Fetcher_Downloaded;
/* Starts asynchronous download of required resources. */
void Fetcher_Run(void);
void Fetcher_Update(void);

#endif
