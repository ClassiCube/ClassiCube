#ifndef CC_EVENT_H
#define CC_EVENT_H
#include "Vectors.h"
/* Helper methods for using events, and contains all events.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
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

typedef void (*Event_Entry_Callback)(void* obj, struct Stream* stream, const cc_string* name);
struct Event_Entry {
	Event_Entry_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Block_Callback)(void* obj, IVec3 coords, BlockID oldBlock, BlockID block);
struct Event_Block {
	Event_Block_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Chat_Callback)(void* obj, const cc_string* msg, int msgType);
struct Event_Chat {
	Event_Chat_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_Input_Callback)(void* obj, int key, cc_bool repeating);
struct Event_Input {
	Event_Input_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_String_Callback)(void* obj, const cc_string* str);
struct Event_String {
	Event_String_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

typedef void (*Event_RawMove_Callback)(void* obj, float xDelta, float yDelta);
struct Event_RawMove {
	Event_RawMove_Callback Handlers[EVENT_MAX_CALLBACKS];
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

/* "data" will be 64 bytes in length. */
typedef void (*Event_PluginMessage_Callback)(void* obj, cc_uint8 channel, cc_uint8* data);
struct Event_PluginMessage {
	Event_PluginMessage_Callback Handlers[EVENT_MAX_CALLBACKS]; 
	void* Objs[EVENT_MAX_CALLBACKS]; int Count;
};

/* Registers a callback function for the given event. */
/* NOTE: Trying to register a callback twice or over EVENT_MAX_CALLBACKS callbacks will terminate the game. */
CC_API void Event_Register(struct Event_Void* handlers,   void* obj, Event_Void_Callback handler);
/* Unregisters a callback function for the given event. */
CC_API void Event_Unregister(struct Event_Void* handlers, void* obj, Event_Void_Callback handler);
#define Event_Register_(handlers,   obj, handler) Event_Register((struct Event_Void*)(handlers),   obj, (Event_Void_Callback)(handler))
#define Event_Unregister_(handlers, obj, handler) Event_Unregister((struct Event_Void*)(handlers), obj, (Event_Void_Callback)(handler))

/* Calls all registered callback for an event with no arguments. */
CC_API void Event_RaiseVoid(struct Event_Void* handlers);
/* Calls all registered callback for an event which has an int argument. */
/* NOTE: The actual argument "type" may be char, Key, cc_uint8 etc */
CC_API void Event_RaiseInt(struct Event_Int* handlers, int arg);
/* Calls all registered callbacks for an event which has a float argument. */
CC_API void Event_RaiseFloat(struct Event_Float* handlers, float arg);

/* Calls all registered callbacks for an event which has data stream and name arguments. */
void Event_RaiseEntry(struct Event_Entry* handlers, struct Stream* stream, const cc_string* name);
/* Calls all registered callbacks for an event which takes block change arguments. */
/* These are the coordinates/location of the change, block there before, block there now. */
void Event_RaiseBlock(struct Event_Block* handlers, IVec3 coords, BlockID oldBlock, BlockID block);
/* Calls all registered callbacks for an event which has chat message type and contents. */
/* See MsgType enum in Chat.h for what types of messages there are. */
void Event_RaiseChat(struct Event_Chat* handlers, const cc_string* msg, int msgType);
/* Calls all registered callbacks for an event which has keyboard key/mouse button. */
/* repeating is whether the key/button was already pressed down. (i.e. user is holding down key) */
void Event_RaiseInput(struct Event_Input* handlers, int key, cc_bool repeating);
/* Calls all registered callbacks for an event which has a string argument. */
void Event_RaiseString(struct Event_String* handlers, const cc_string* str);
/* Calls all registered callbacks for an event which has raw pointer movement arguments. */
void Event_RaiseRawMove(struct Event_RawMove* handlers, float xDelta, float yDelta);
/* Calls all registered callbacks for an event which has a channel and a 64 byte data argument. */
void Event_RaisePluginMessage(struct Event_PluginMessage* handlers, cc_uint8 channel, cc_uint8* data);

void Event_UnregisterAll(void);
/* NOTE: Event_UnregisterAll MUST be updated when events lists are changed */

/* Event API version supported by the client */
/*  Version 1 - Added NetEvents.PluginMessageReceived */
/*  Version 2 - Added WindowEvents.Redrawing */
/*  Version 3 - Changed InputEvent.Press from code page 437 to unicode character */
/* You MUST CHECK the event API version before attempting to use the events listed above, */
/*  as otherwise if the player is using an older client that lacks some of the above events, */
/*  you will be calling Event_Register on random data instead of the expected EventsList struct */
CC_VAR extern int EventAPIVersion;

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
	struct Event_Block BlockChanged;      /* User changes a block */
	struct Event_Void  HackPermsChanged;  /* Hack permissions of the player changes */
	struct Event_Void  HeldBlockChanged;  /* Held block in hotbar changes */
	struct Event_Void  HacksStateChanged; /* Hack states changed (e.g. stops flying) */
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
	struct Event_Void RedrawNeeded; /* Window contents invalidated and will need to be redrawn */
	struct Event_Void Resized;      /* Window is resized */
	struct Event_Void Closing;      /* Window is about to close (should free resources/save state/etc here) */
	struct Event_Void FocusChanged; /* Focus of the window changed */
	struct Event_Void StateChanged; /* State of the window changed (e.g. minimised, fullscreen) */
	struct Event_Void Created;      /* Window has been created, Window_Handle is valid now. */
	struct Event_Void InactiveChanged; /* Inactive/background state of the window changed */
	struct Event_Void Redrawing;    /* Window contents should be redrawn (as they are about to be displayed) */
} WindowEvents;

CC_VAR extern struct _InputEventsList {
	struct Event_Int    Press; /* Key input character is typed. Arg is a unicode character */
	struct Event_Input  Down;  /* Key or button is pressed. Arg is a member of Key enumeration */
	struct Event_Int    Up;    /* Key or button is released. Arg is a member of Key enumeration */
	struct Event_Float  Wheel; /* Mouse wheel is moved/scrolled (Arg is wheel delta) */
	struct Event_String TextChanged; /* Text in the on-screen input keyboard changed (for Mobile) */
} InputEvents;

CC_VAR extern struct _PointerEventsList {
	struct Event_Int        Moved; /* Pointer position changed (Arg is index) */
	struct Event_Int         Down; /* Left mouse or touch is pressed (Arg is index) */
	struct Event_Int         Up;   /* Left mouse or touch is released (Arg is index) */
	struct Event_RawMove RawMoved; /* Raw pointer position changed (Arg is delta) */
} PointerEvents;

CC_VAR extern struct _NetEventsList {
	struct Event_Void Connected;    /* Connection to a server was established */
	struct Event_Void Disconnected; /* Connection to the server was lost */
	struct Event_PluginMessage PluginMessageReceived; /* Received a PluginMessage packet from the server */
} NetEvents;
#endif
