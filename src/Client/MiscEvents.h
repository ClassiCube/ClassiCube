#ifndef CS_MISCEVENTS_H
#define CS_MISCEVENTS_H
#include "EventHandler.h"
#include "Typedefs.h"
/* Misc Events.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


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
#endif