#ifndef CC_EVENT_H
#define CC_EVENT_H
#include "String.h"
#include "Vectors.h"
/* Helper methods for using events, and contains all events.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Max callbacks that can be registered for an event. */
#define EVENT_MAX_CALLBACKS 32
struct Stream;

typedef void (*Event_Void_Callback)(void* obj);
struct Event_Void {
	Event_Void_Callback Handlers[EVENT_MAX_CALLBACKS]; 
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Int_Callback)(void* obj, int argument);
struct Event_Int {
	Event_Int_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Float_Callback)(void* obj, float argument);
struct Event_Float {
	Event_Float_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Entry_Callback)(void* obj, struct Stream* stream, const String* name);
struct Event_Entry {
	Event_Entry_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Block_Callback)(void* obj, Vector3I coords, BlockID oldBlock, BlockID block);
struct Event_Block {
	Event_Block_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_MouseMove_Callback)(void* obj, int xDelta, int yDelta);
struct Event_MouseMove {
	Event_MouseMove_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Chat_Callback)(void* obj, const String* msg, int msgType);
struct Event_Chat {
	Event_Chat_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Input_Callback)(void* obj, int key, bool repeating);
struct Event_Input {
	Event_Input_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

/* Registers a callback function for the given event. */
/* NOTE: Trying to register a callback twice or over EVENT_MAX_CALLBACKS callbacks will terminate the game. */
CC_API void Event_Register(struct Event_Void* handlers,   void* obj, Event_Void_Callback handler);
/* Unregisters a callback function for the given event. */
/* NOTE: Trying to unregister a non-registered callback will terminate the game. */
CC_API void Event_Unregister(struct Event_Void* handlers, void* obj, Event_Void_Callback handler);
#define Event_RegisterMacro(handlers,   obj, handler) Event_Register((struct Event_Void*)(handlers),   obj, (Event_Void_Callback)(handler))
#define Event_UnregisterMacro(handlers, obj, handler) Event_Unregister((struct Event_Void*)(handlers), obj, (Event_Void_Callback)(handler))

/* Calls all registered callback for an event with no arguments. */
CC_API void Event_RaiseVoid(struct Event_Void* handlers);
#define Event_RegisterVoid(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterVoid(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callback for an event which has an int argument. */
/* NOTE: The actual argument "type" may be char, Key, uint8_t etc */
CC_API void Event_RaiseInt(struct Event_Int* handlers, int arg);
#define Event_RegisterInt(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterInt(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which has a float argument. */
CC_API void Event_RaiseFloat(struct Event_Float* handlers, float arg);
#define Event_RegisterFloat(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterFloat(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which has data stream and name argumenst. */
/* This is (currently) only used for processing entries from default.zip */
void Event_RaiseEntry(struct Event_Entry* handlers, struct Stream* stream, const String* name);
#define Event_RegisterEntry(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterEntry(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which takes block change arguments. */
/* These are the coordinates/location of the change, block there before, block there now. */
void Event_RaiseBlock(struct Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block);
#define Event_RegisterBlock(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterBlock(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which has mouse movement arguments. */
/* This is simply delta since last move. Use Mouse_X and Mouse_Y to get the mouse position. */
void Event_RaiseMouseMove(struct Event_MouseMove* handlers, int xDelta, int yDelta);
#define Event_RegisterMouseMove(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterMouseMove(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which has chat message type and contents. */
/* See MsgType enum in Chat.h for what types of messages there are. */
void Event_RaiseChat(struct Event_Chat* handlers, const String* msg, int msgType);
#define Event_RegisterChat(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterChat(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

/* Calls all registered callbacks for an event which has keyboard key/mouse button. */
/* repeating is whether the key/button was already pressed down. (i.e. user is holding down key) */
/* NOTE: 'pressed up'/'released' events will always have repeating as false. */
void Event_RaiseInput(struct Event_Input* handlers, int key, bool repeating);
#define Event_RegisterInput(handlers,   obj, handler) Event_RegisterMacro(handlers,   obj, handler)
#define Event_UnregisterInput(handlers, obj, handler) Event_UnregisterMacro(handlers, obj, handler)

CC_VAR extern struct _EntityEventsList {
	struct Event_Int Added;    /* Entity is spawned in the current world */
	struct Event_Int Removed;  /* Entity is despawned from the current world */
} EntityEvents;

CC_VAR extern struct _TabListEventsList {
	struct Event_Int Added;   /* Tab list entry is created */
	struct Event_Int Changed; /* Tab list entry is modified */
	struct Event_Int Removed; /* Tab list entry is removed */
} TabListEvents;

CC_VAR extern struct _TextureEventsList {
	struct Event_Void  AtlasChanged; /* Terrain atlas (terrain.png) is changed */
	struct Event_Void  PackChanged;  /* Texture pack is changed */
	struct Event_Entry FileChanged;  /* File in a texture pack is changed (terrain.png, rain.png) */
} TextureEvents;

CC_VAR extern struct _GfxEventsList {
	struct Event_Void ViewDistanceChanged; /* View/fog distance is changed */
	struct Event_Void LowVRAMDetected;     /* Insufficient VRAM detected, need to free some GPU resources */
	struct Event_Void ProjectionChanged;   /* Projection matrix has changed */
	struct Event_Void ContextLost;         /* Context is destroyed after having been previously created */
	struct Event_Void ContextRecreated;    /* Context is recreated after having been previously lost */
} GfxEvents;

CC_VAR extern struct _UserEventsList {
	struct Event_Block BlockChanged;           /* User changes a block */
	struct Event_Void  HackPermissionsChanged; /* Hack permissions of the player changes */
	struct Event_Void  HeldBlockChanged;       /* Held block in hotbar changes */
} UserEvents;

CC_VAR extern struct _BlockEventsList {
	struct Event_Void PermissionsChanged; /* Block permissions (can place/delete) for a block changes */
	struct Event_Void BlockDefChanged;    /* Block definition is changed or removed */
} BlockEvents;

CC_VAR extern struct _WorldEventsList {
	struct Event_Void  NewMap;        /* Player begins loading a new world */
	struct Event_Float Loading;       /* Portion of world is decompressed/generated (Arg is progress from 0-1) */
	struct Event_Void  MapLoaded;     /* New world has finished loading, player can now interact with it */
	struct Event_Int   EnvVarChanged; /* World environment variable changed by player/CPE/WoM config */
} WorldEvents;

CC_VAR extern struct _ChatEventsList {
	struct Event_Void FontChanged;   /* User changes whether system chat font used, and when the bitmapped font texture changes */
	struct Event_Chat ChatReceived;  /* Raised when message is being added to chat */
	struct Event_Chat ChatSending;   /* Raised when user sends a message */
	struct Event_Int  ColCodeChanged; /* Raised when a colour code changes */
} ChatEvents;

CC_VAR extern struct _WindowEventsList {
	struct Event_Void Redraw;             /* Window contents invalidated, should be redrawn */
	struct Event_Void Moved;              /* Window is moved */
	struct Event_Void Resized;            /* Window is resized */
	struct Event_Void Closing;            /* Window is about to close */
	struct Event_Void Closed;             /* Window has closed */
	struct Event_Void VisibilityChanged;  /* Visibility of the window changed */
	struct Event_Void FocusChanged;       /* Focus of the window changed */
	struct Event_Void StateChanged;       /* WindowState of the window changed */
} WindowEvents;

CC_VAR extern struct _KeyEventsList {
	struct Event_Int Press;  /* Raised when a character is typed. Arg is a character */
	struct Event_Input Down; /* Raised when a key is pressed. Arg is a member of Key enumeration */
	struct Event_Int Up;     /* Raised when a key is released. Arg is a member of Key enumeration */
} KeyEvents;

CC_VAR extern struct _MouseEventsList {
	struct Event_MouseMove Moved; /* Mouse position is changed (Arg is delta from last position) */
	struct Event_Int Down;        /* Mouse button is pressed (Arg is MouseButton member) */
	struct Event_Int Up;          /* Mouse button is released (Arg is MouseButton member) */
	struct Event_Float Wheel;     /* Mouse wheel is moved/scrolled (Arg is wheel delta) */
} MouseEvents;

CC_VAR extern struct _NetEventsList {
	struct Event_Void Connected;    /* Connection to a server was established. */
	struct Event_Void Disconnected; /* Connection to the server was lost. */
} NetEvents;
#endif
