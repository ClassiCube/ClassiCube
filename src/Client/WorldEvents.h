#ifndef CS_WORLDEVENTS_H
#define CS_WORLDEVENTS_H
#include "EventHandler.h"
#include "Typedefs.h"
/* World related events.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Raised when the player joins and begins loading a new world. */
Event_Void WorldEvents_NewMap[EventHandler_Size];
Int32 WorldEvents_NewMapCount;
#define WorldEvents_RaiseNewMap()\
EventHandler_Raise_Void(WorldEvents_NewMap, WorldEvents_NewMapCount);

/* Raised when a portion of the world is read and decompressed, or generated.
The floating point argument is progress (from 0 to 1). */
Event_Real32 WorldEvents_MapLoading[EventHandler_Size];
Int32 WorldEvents_MapLoadingCount;
#define WorldEvents_RaiseMapLoading(progress)\
EventHandler_Raise_Real32(WorldEvents_MapLoading, WorldEvents_MapLoadingCount, progress);

/* Raised when new world has finished loading and the player can now interact with it. */
Event_Void WorldEvents_MapLoaded[EventHandler_Size];
Int32 WorldEvents_MapLoadedCount;
#define WorldEvents_RaiseMapLoaded()\
EventHandler_Raise_Void(WorldEvents_MapLoaded, WorldEvents_MapLoadedCount);

/* Raised when an environment variable of the world is changed by the user, CPE, or WoM config. */
Event_Int32 WorldEvents_EnvVarChanged[EventHandler_Size];
Int32 WorldEvents_EnvVarChangedCount;
#define WorldEvents_RaiseEnvVariableChanged(envVar)\
EventHandler_Raise_Int32(WorldEvents_EnvVarChanged, WorldEvents_EnvVarChangedCount, envVar);


/* Environment variable identifiers*/
typedef Int32 EnvVar;
#define EnvVar_EdgeBlock 0
#define EnvVar_SidesBlock 1
#define EnvVar_EdgeHeight 2
#define EnvVar_SidesOffset 3
#define EnvVar_CloudsHeight 4
#define EnvVar_CloudsSpeed 5

#define EnvVar_WeatherSpeed 6
#define EnvVar_WeatherFade 7
#define EnvVar_Weather 8
#define EnvVar_ExpFog 9

#define EnvVar_SkyCol 10
#define EnvVar_CloudsCol 11
#define EnvVar_FogCol 12
#define EnvVar_SunCol 13
#define EnvVar_ShadowCol 14
#endif