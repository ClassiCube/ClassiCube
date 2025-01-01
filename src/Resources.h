#ifndef CC_RESOURCES_H
#define CC_RESOURCES_H
#include "Core.h"
CC_BEGIN_HEADER

/* Implements checking, fetching, and patching the default game assets.
	Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct HttpRequest;
typedef void (*FetcherErrorCallback)(struct HttpRequest* req);

/* Number of resources that need to be downloaded */
extern int Resources_MissingCount;
/* Total size of resources that need to be downloaded */
extern int Resources_MissingSize;
/* Whether required resources need to be downloaded */
extern cc_bool Resources_MissingRequired;
/* Checks existence of all assets */
void Resources_CheckExistence(void);

/* Whether fetcher is currently downloading resources */
extern cc_bool Fetcher_Working;
/* Whether fetcher has finished (downloaded all resources, or an error) */
extern cc_bool Fetcher_Completed;
/* Number of resources that have been downloaded so far */
extern int Fetcher_Downloaded;
/* Whether a resource failed to download */
extern cc_bool Fetcher_Failed;
/* Callback function invoked if a resource fails to download */
extern FetcherErrorCallback Fetcher_ErrorCallback;

/* Finds name of resource associated with given http request. */
const char* Fetcher_RequestName(int reqID);
/* Starts asynchronous download of missing resources. */
void Fetcher_Run(void);
/* Checks if any resources have finished downloading. */
/* If any have, performs required patching and saving. */
void Fetcher_Update(void);

CC_END_HEADER
#endif
