#ifndef CS_EVENTS_H
#define CS_EVENTS_H
#include "EventHandler.h"
#include "Typedefs.h"
/* Contains the various events that are raised by the client.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Raised when an entity is spawned in the current world. */
Event_EntityID EntityEvents_Added[EventHandler_Size];
Int32 EntityEvents_AddedCount;
#define EntityEvents_RaiseAdded(id)\
EventHandler_Raise_EntityID(EntityEvents_Added, EntityEvents_AddedCount, id);

/* Raised when an entity is despawned from the current world. */
Event_EntityID EntityEvents_Removed[EventHandler_Size];
Int32 EntityEvents_RemovedCount;
#define EntityEvents_RaiseRemoved(id)\
EventHandler_Raise_EntityID(EntityEvents_Removed, EntityEvents_RemovedCount, id);


/* Raised when a tab list entry is created. */
Event_EntityID TabListEvents_Added[EventHandler_Size];
Int32 TabListEvents_AddedCount;
#define TabListEvents_RaiseAdded(id)\
EventHandler_Raise_EntityID(TabListEvents_Added, TabListEvents_AddedCount, id);

/* Raised when a tab list entry is modified. */
Event_EntityID TabListEvents_Changed[EventHandler_Size];
Int32 TabListEvents_ChangedCount;
#define TabListEvents_RaiseChanged(id)\
EventHandler_Raise_EntityID(TabListEvents_Changed, TabListEvents_ChangedCount, id);

/* Raised when a tab list entry is removed. */
Event_EntityID TabListEvents_Removed[EventHandler_Size];
Int32 TabListEvents_RemovedCount;
#define TabListEvents_RaiseRemoved(id)\
EventHandler_Raise_EntityID(TabListEvents_Removed, TabListEvents_RemovedCount, id);


/* Raised when the terrain atlas ("terrain.png") is changed. */
Event_Void TextureEvents_AtlasChanged[EventHandler_Size];
Int32 TextureEvents_AtlasChangedCount;
#define TextureEvents_RaiseAtlasChanged()\
EventHandler_Raise_Void(TextureEvents_AtlasChanged, TextureEvents_AtlasChangedCount);

/* Raised when the texture pack is changed. */
Event_Void TextureEvents_PackChanged[EventHandler_Size];
Int32 TextureEvents_PackChangedCount;
#define TextureEvents_RaisePackChanged()\
EventHandler_Raise_Void(TextureEvents_PackChanged, TextureEvents_PackChangedCount);

/* Raised when a file in a texture pack is changed. (such as "terrain.png", "rain.png") */
Event_Stream TextureEvents_FileChanged[EventHandler_Size];
Int32 TextureEvents_FileChangedCount;
#define TextureEvents_RaiseFileChanged(stream)\
EventHandler_Raise_Stream(TextureEvents_TextureChanged, TextureEvents_FileChangedCount, stream);


/* Raised when the user changed their view/fog distance. */
Event_Void GfxEvents_ViewDistanceChanged[EventHandler_Size];
Int32 GfxEvents_ViewDistanceChangedCount;
#define GfxEvents_RaiseViewDistanceChanged()\
EventHandler_Raise_Void(GfxEvents_ViewDistanceChanged, GfxEvents_ViewDistanceCount);

/* Raised when the projection matrix changes. */
Event_Void GfxEvents_ProjectionChanged[EventHandler_Size];
Int32 GfxEvents_ProjectionChangedCount;
#define GfxEvents_RaiseProjectionChanged()\
EventHandler_Raise_Void(GfxEvents_ProjectionChanged, GfxEvents_ProjectionCount);

/* Raised when the user changes a block. */
Event_Block UserEvents_BlockChanged[EventHandler_Size];
Int32 UserEvents_BlockChangedCount;
#define UserEvents_BlockChangedChanged(coords, oldBlock, block)\
EventHandler_Raise_Block(UserEvents_BlockChanged, UserEvents_BlockChangedCount, coords, oldBlock, block);


/* Raised when the block permissions(can place or delete a block) for the player changes. */
Event_Void BlockEvents_PermissionsChanged[EventHandler_Size];
Int32 BlockEvents_PermissionsChangedCount;
#define BlockEvents_RaisePermissionsChanged()\
EventHandler_Raise_Void(BlockEvents_PermissionsChanged, BlockEvents_PermissionsChangedCount);

/* Raised when a block definition is changed. */
Event_Void BlockEvents_BlockDefChanged[EventHandler_Size];
Int32 BlockEvents_BlockDefChangedCount;
#define BlockEvents_RaiseBlockDefChanged()\
EventHandler_Raise_Void(BlockEvents_BlockDefChanged, BlockEvents_BlockDefChangedCount);


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