#ifndef CC_EVENT_H
#define CC_EVENT_H
#include "Stream.h"
#include "Vectors.h"
/* Helper method for managing events, and contains all events.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Maximum number of event handlers that can be registered. */
#define EVENT_MAX_CALLBACKS 32

typedef void (*Event_Void_Callback)(void* obj);
struct Event_Void {
	Event_Void_Callback Handlers[EVENT_MAX_CALLBACKS]; 
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_Int_Callback)(void* obj, Int32 argument);
struct Event_Int {
	Event_Int_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_Real_Callback)(void* obj, Real32 argument);
struct Event_Real {
	Event_Real_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_Stream_Callback)(void* obj, struct Stream* stream);
struct Event_Stream {
	Event_Stream_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_Block_Callback)(void* obj, Vector3I coords, BlockID oldBlock, BlockID block);
struct Event_Block {
	Event_Block_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_MouseMove_Callback)(void* obj, Int32 xDelta, Int32 yDelta);
struct Event_MouseMove {
	Event_MouseMove_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

typedef void (*Event_Chat_Callback)(void* obj, String* msg, Int32 msgType);
struct Event_Chat {
	Event_Chat_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; UInt32 Count;
};

void Event_RaiseVoid(struct Event_Void* handlers);
void Event_RegisterVoid(struct Event_Void* handlers, void* obj, Event_Void_Callback handler);
void Event_UnregisterVoid(struct Event_Void* handlers, void* obj, Event_Void_Callback handler);

void Event_RaiseInt(struct Event_Int* handlers, Int32 arg);
void Event_RegisterInt(struct Event_Int* handlers, void* obj, Event_Int_Callback handler);
void Event_UnregisterInt(struct Event_Int* handlers, void* obj, Event_Int_Callback handler);

void Event_RaiseReal(struct Event_Real* handlers, Real32 arg);
void Event_RegisterReal(struct Event_Real* handlers, void* obj, Event_Real_Callback handler);
void Event_UnregisterReal(struct Event_Real* handlers, void* obj, Event_Real_Callback handler);

void Event_RaiseStream(struct Event_Stream* handlers, struct Stream* stream);
void Event_RegisterStream(struct Event_Stream* handlers, void* obj, Event_Stream_Callback handler);
void Event_UnregisterStream(struct Event_Stream* handlers, void* obj, Event_Stream_Callback handler);

void Event_RaiseBlock(struct Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block);
void Event_RegisterBlock(struct Event_Block* handlers, void* obj, Event_Block_Callback handler);
void Event_UnregisterBlock(struct Event_Block* handlers, void* obj, Event_Block_Callback handler);

void Event_RaiseMouseMove(struct Event_MouseMove* handlers, Int32 xDelta, Int32 yDelta);
void Event_RegisterMouseMove(struct Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler);
void Event_UnregisterMouseMove(struct Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler);

void Event_RaiseChat(struct Event_Chat* handlers, String* msg, Int32 msgType);
void Event_RegisterChat(struct Event_Chat* handlers, void* obj, Event_Chat_Callback handler);
void Event_UnregisterChat(struct Event_Chat* handlers, void* obj, Event_Chat_Callback handler);

struct Event_Int EntityEvents_Added;    /* Entity is spawned in the current world. */
struct Event_Int EntityEvents_Removed;  /* Entity is despawned from the current world. */
struct Event_Int TabListEvents_Added;   /* Tab list entry is created. */
struct Event_Int TabListEvents_Changed; /* Tab list entry is modified. */
struct Event_Int TabListEvents_Removed; /* Tab list entry is removed. */

struct Event_Void TextureEvents_AtlasChanged;  /* Terrain atlas (terrain.png) is changed. */
struct Event_Void TextureEvents_PackChanged;   /* Texture pack is changed. */
struct Event_Stream TextureEvents_FileChanged; /* File in a texture pack is changed. (terrain.png, rain.png) */

struct Event_Void GfxEvents_ViewDistanceChanged; /* View/fog distance is changed. */
struct Event_Void GfxEvents_ProjectionChanged;   /* Projection matrix has changed. */
struct Event_Void GfxEvents_ContextLost;         /* Context is destroyed after having been previously created. */
struct Event_Void GfxEvents_ContextRecreated;    /* context is recreated after having been previously lost. */

struct Event_Block UserEvents_BlockChanged;          /* User changes a block. */
struct Event_Void UserEvents_HackPermissionsChanged; /* Hack permissions of the player changes. */
struct Event_Void UserEvents_HeldBlockChanged;       /* Held block in hotbar changes. */

struct Event_Void BlockEvents_PermissionsChanged; /* Block permissions (can place/delete) for a block changes. */
struct Event_Void BlockEvents_BlockDefChanged;    /* Block definition is changed or removed. */

struct Event_Void WorldEvents_NewMap;         /* Player begins loading a new world. */
struct Event_Real WorldEvents_Loading;   /* Portion of world is decompressed/generated. (Arg is progress from 0-1) */
struct Event_Void WorldEvents_MapLoaded;      /* New world has finished loading, player can now interact with it. */
struct Event_Int WorldEvents_EnvVarChanged; /* World environment variable changed by player/CPE/WoM config. */

struct Event_Void ChatEvents_FontChanged;     /* User changes whether system chat font used, and when the bitmapped font texture changes. */
struct Event_Chat ChatEvents_ChatReceived;    /* Raised when the server or a client-side command sends a message */
struct Event_Int ChatEvents_ColCodeChanged; /* Raised when a colour code changes */

struct Event_Void WindowEvents_Redraw;             /* Window contents invalidated, should be redrawn */
struct Event_Void WindowEvents_Moved;              /* Window is moved. */
struct Event_Void WindowEvents_Resized;            /* Window is resized. */
struct Event_Void WindowEvents_Closing;            /* Window is about to close. */
struct Event_Void WindowEvents_Closed;             /* Window has closed. */
struct Event_Void WindowEvents_VisibilityChanged;  /* Visibility of the window changes. */
struct Event_Void WindowEvents_FocusChanged;       /* Focus of the window changes. */
struct Event_Void WindowEvents_WindowStateChanged; /* WindowState of the window changes. */

struct Event_Int KeyEvents_Press; /* Raised when a character is typed. Arg is a character. */
struct Event_Int KeyEvents_Down;  /* Raised when a key is pressed. Arg is a member of Key enumeration. */
struct Event_Int KeyEvents_Up;    /* Raised when a key is released. Arg is a member of Key enumeration. */

struct Event_MouseMove MouseEvents_Moved; /* Mouse position is changed. (Arg is delta from last position) */
struct Event_Int MouseEvents_Down;      /* Mouse button is pressed. (Arg is MouseButton member) */
struct Event_Int MouseEvents_Up;        /* Mouse button is released. (Arg is MouseButton member) */
struct Event_Real MouseEvents_Wheel;    /* Mouse wheel is moved/scrolled. (Arg is wheel delta) */
#endif
