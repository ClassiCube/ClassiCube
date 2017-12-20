#ifndef CC_EVENT_H
#define CC_EVENT_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
#include "Vectors.h"
/* Helper method for managing events, and contains all events.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Maximum number of event handlers that can be registered. */
#define EVENT_MAX_CALLBACKS 32

typedef void(*Event_Void_Callback)(void);
typedef struct Event_Void_ {
	Event_Void_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Void;

typedef void(*Event_Int32_Callback)(Int32 argument);
typedef struct Event_Int32_ {
	Event_Int32_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Int32;

typedef void(*Event_Real32_Callback)(Real32 argument);
typedef struct Event_Real32_ {
	Event_Real32_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Real32;

typedef void(*Event_EntityID_Callback)(EntityID argument);
typedef struct Event_EntityID_ {
	Event_EntityID_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_EntityID;

typedef void(*Event_Stream_Callback)(Stream* stream);
typedef struct Event_Stream_ {
	Event_Stream_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Stream;

typedef void(*Event_Block_Callback)(Vector3I coords, BlockID oldBlock, BlockID block);
typedef struct Event_Block_ {
	Event_Block_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Block;

typedef void(*Event_MouseMove_Callback)(Int32 xDelta, Int32 yDelta);
typedef struct Event_MouseMove_ {
	Event_MouseMove_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_MouseMove;

typedef void(*Event_Chat_Callback)(String* msg, UInt8 msgType);
typedef struct Event_Chat_ {
	Event_Chat_Callback Handlers[EVENT_MAX_CALLBACKS]; UInt32 Count;
} Event_Chat;

void Event_RaiseVoid(Event_Void* handlers);
void Event_RegisterVoid(Event_Void* handlers, Event_Void_Callback handler);
void Event_UnregisterVoid(Event_Void* handlers, Event_Void_Callback handler);

void Event_RaiseInt32(Event_Int32* handlers, Int32 arg);
void Event_RegisterInt32(Event_Int32* handlers, Event_Int32_Callback handler);
void Event_UnregisterInt32(Event_Int32* handlers, Event_Int32_Callback handler);

void Event_RaiseReal32(Event_Real32* handlers, Real32 arg);
void Event_RegisterReal32(Event_Real32* handlers, Event_Real32_Callback handler);
void Event_UnregisterReal32(Event_Real32* handlers, Event_Real32_Callback handler);

void Event_RaiseEntityID(Event_EntityID* handlers, EntityID arg);
void Event_RegisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler);
void Event_UnregisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler);

void Event_RaiseStream(Event_Stream* handlers, Stream* stream);
void Event_RegisterStream(Event_Stream* handlers, Event_Stream_Callback handler);
void Event_UnregisterStream(Event_Stream* handlers, Event_Stream_Callback handler);

void Event_RaiseBlock(Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block);
void Event_RegisterBlock(Event_Block* handlers, Event_Block_Callback handler);
void Event_UnregisterBlock(Event_Block* handlers, Event_Block_Callback handler);

void Event_RaiseMouseMove(Event_MouseMove* handlers, Int32 xDelta, Int32 yDelta);
void Event_RegisterMouseMove(Event_MouseMove* handlers, Event_MouseMove_Callback handler);
void Event_UnregisterMouseMove(Event_MouseMove* handlers, Event_MouseMove_Callback handler);

void Event_RaiseChat(Event_Chat* handlers, String* msg, UInt8 msgType);
void Event_RegisterChat(Event_Chat* handlers, Event_Chat_Callback handler);
void Event_UnregisterChat(Event_Chat* handlers, Event_Chat_Callback handler);


Event_EntityID EntityEvents_Added;   /* Entity is spawned in the current world. */
Event_EntityID EntityEvents_Removed; /* Entity is despawned from the current world. */

Event_EntityID TabListEvents_Added;   /* Tab list entry is created. */
Event_EntityID TabListEvents_Changed; /* Tab list entry is modified. */
Event_EntityID TabListEvents_Removed; /* Tab list entry is removed. */

Event_Void TextureEvents_AtlasChanged;  /* Terrain atlas (terrain.png) is changed. */
Event_Void TextureEvents_PackChanged;   /* Texture pack is changed. */
Event_Stream TextureEvents_FileChanged; /* File in a texture pack is changed. (terrain.png, rain.png) */

Event_Void GfxEvents_ViewDistanceChanged; /* View/fog distance is changed. */
Event_Void GfxEvents_ProjectionChanged;   /* Projection matrix has changed. */
Event_Void GfxEvents_ContextLost;         /* Context is destroyed after having been previously created. */
Event_Void GfxEvents_ContextRecreated;    /* context is recreated after having been previously lost. */

Event_Block UserEvents_BlockChanged;          /* User changes a block. */
Event_Void UserEvents_HackPermissionsChanged; /* Hack permissions of the player changes. */
Event_Void UserEvents_HeldBlockChanged;       /* Held block in hotbar changes. */

Event_Void BlockEvents_PermissionsChanged; /* Block permissions (can place/delete) for a block changes. */
Event_Void BlockEvents_BlockDefChanged;    /* Block definition is changed or removed. */

Event_Void WorldEvents_NewMap;         /* Player begins loading a new world. */
Event_Real32 WorldEvents_MapLoading;   /* Portion of world is decompressed/generated. (Arg is progress from 0-1) */
Event_Void WorldEvents_MapLoaded;      /* New world has finished loading, player can now interact with it. */
Event_Int32 WorldEvents_EnvVarChanged; /* World environment variable changed by player/CPE/WoM config. */

Event_Void ChatEvents_FontChanged;  /* User changes whether system chat font used, and when the bitmapped font texture changes. */
Event_Chat ChatEvents_ChatReceived; /*  Raised when the server or a client-side command sends a message */


Event_Void WindowEvents_Moved;              /* Window is moved. */
Event_Void WindowEvents_Resized;            /* Window is resized. */
Event_Void WindowEvents_Closing;            /* Window is about to close. */
Event_Void WindowEvents_Closed;             /* Window has closed. */
Event_Void WindowEvents_VisibilityChanged;  /* Visibility of the window changes. */
Event_Void WindowEvents_FocusChanged;       /* Focus of the window changes. */
Event_Void WindowEvents_WindowStateChanged; /* WindowState of the window changes. */
Event_Void WindowEvents_MouseLeave;         /* Mouse cursor leaves window bounds. */
Event_Void WindowEvents_MouseEnter;         /* Mouse cursor re-enters window bounds. */

Event_Int32 KeyEvents_Press; /* Raised when a character is typed. Arg is a character. */
Event_Int32 KeyEvents_Down;  /* Raised when a key is pressed. Arg is a member of Key enumeration. */
Event_Int32 KeyEvents_Up;    /* Raised when a key is released. Arg is a member of Key enumeration. */

Event_MouseMove MouseEvents_Moved; /* Mouse position is changed. (Arg is delta from last position) */
Event_Int32 MouseEvents_Down;      /* Mouse button is pressed. (Arg is MouseButton member) */
Event_Int32 MouseEvents_Up;        /* Mouse button is released. (Arg is MouseButton member) */
Event_Real32 MouseEvents_Wheel;    /* Mouse wheel is moved/scrolled. (Arg is wheel delta) */
#endif