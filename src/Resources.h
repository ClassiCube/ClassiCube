#ifndef CC_RESOURCES_H
#define CC_RESOURCES_H
#include "LWeb.h"
/* Implements checking, fetching, and patching the default game assets.
	Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

#define FLAG_CLASSIC 0x01 /* file depends on classic.jar */
#define FLAG_MODERN  0x02 /* file depends on modern jar */
#define FLAG_GUI     0x04 /* file depends on patched gui.png */
#define FLAG_TERRAIN 0x08 /* file depends on patched terrain.png */

extern struct ResourceFile {
	const char* name;
	const char* url;
	uint16_t size;
	bool downloaded;
	/* downloaded archive */
	uint8_t* data; uint32_t len;
} Resources_Files[4];

extern struct ResourceTexture {
	const char* filename;
	/* zip data */
	uint32_t size, offset, crc32;
} Resources_Textures[20];

extern struct ResourceSound {
	const char* name;
	const char* hash;
} Resources_Sounds[59];

extern struct ResourceMusic {
	const char* name;
	const char* hash;
	uint16_t size;
	bool downloaded;
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
/* HTTP status code of last failed resource download */
extern int Fetcher_StatusCode;
/* Error code of last failed resource download. */
extern ReturnCode Fetcher_Result;
/* Whether a resource failed to download. */
extern bool Fetcher_Failed;

/* Starts asynchronous download of missing resources. */
void Fetcher_Run(void);
/* Checks if any resources have finished downloading. */
/* If any have, performs required patching and saving. */
void Fetcher_Update(void);
#endif
