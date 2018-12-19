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
	bool Downloaded;
	/* downloaded archive */
	uint8_t* Data; uint32_t Len;
} Resources_Files[4];

extern struct ResourceTexture {
	const char* Filename;
	/* zip data */
	uint32_t Size, Offset, Crc32;
} Resources_Textures[20];

extern struct ResourceSound {
	const char* Name;
	const char* Hash;
} Resources_Sounds[59];

extern struct ResourceMusic {
	const char* Name;
	const char* Hash;
	uint16_t Size;
	bool Downloaded;
} Resources_Music[7];

/* Whether all textures exist. */
extern bool Textures_AllExist;
/* Whether all sounds exist. */
extern bool Sounds_AllExist;
/* Number of resources that need to be downloaded. */
extern int Resources_Count;
/* Total size of resources that need to be downloaded. */
extern int Resources_Size;
/* Checks existence of all assets. */
void Resources_CheckExistence(void);

/* Whether fetcher is currently downloading resources. */
extern bool Fetcher_Working;
/* Whether fethcer has finished. (downloaded all resources, or an error) */
extern bool Fetcher_Completed;
/* Number of resources that have been downloaded so far. */
extern int Fetcher_Downloaded;
/* HTTP error (if any) that occurs when downloaded resources. */
extern int Fetcher_StatusCode;
/* Error (if any) that occurs when downloaded resources. */
extern ReturnCode Fetcher_Error;

/* Starts asynchronous download of missing resources. */
void Fetcher_Run(void);
/* Checks if any resources have finished downloading. */
/* If any have, performs required patching and saving. */
void Fetcher_Update(void);

#endif
