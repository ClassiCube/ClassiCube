#ifndef CC_EVENT_H
#define CC_EVENT_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
#include "Vectors.h"
/* Helper method for managing events.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
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
#endif