#ifndef CS_EVENT_H
#define CS_EVENT_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
#include "Vectors.h"
/* Helper method for managing events.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Maximum number of event handlers that can be registered. */
#define Event_MaxCallbacks 32

/* Event that takes no arguments. */
typedef void(*Event_Void_Callback)(void);
typedef struct Event_Void_ {
	Event_Void_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_Void;

/* Event that takes single 32 bit signed integer argument. */
typedef void(*Event_Int32_Callback)(Int32 argument);
typedef struct Event_Int32_ {
	Event_Int32_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_Int32;

/* Event handler that takes single floating-point argument. */
typedef void(*Event_Real32_Callback)(Real32 argument);
typedef struct Event_Real32_ {
	Event_Real32_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_Real32;

/* Event handler that takes an entity ID as an argument. */
typedef void(*Event_EntityID_Callback)(EntityID argument);
typedef struct Event_EntityID_ {
	Event_EntityID_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_EntityID;

/* Event handler that takes stream as an argument. */
typedef void(*Event_Stream_Callback)(Stream* stream);
typedef struct Event_Stream_ {
	Event_Stream_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_Stream;

/* Event handler that takes a block change argument. */
typedef void(*Event_Block_Callback)(Vector3I coords, BlockID oldBlock, BlockID block);
typedef struct Event_Block_ {
	Event_Block_Callback Handlers[Event_MaxCallbacks]; Int32 HandlersCount;
} Event_Block;


/* Adds given event handler to handlers list for the given event. */
static void Event_RegisterImpl(Event_Void* handlers, Event_Void_Callback handler);
/* Removes given event handler from handlers list of the given event. */
static void Event_UnregisterImpl(Event_Void* handlers, Event_Void_Callback handler);

/* Calls handlers for an event that has no arguments.*/
void Event_RaiseVoid(Event_Void* handlers);
void Event_RegisterVoid(Event_Void* handlers, Event_Void_Callback handler);
void Event_UnregisterVoid(Event_Void* handlers, Event_Void_Callback handler);

/* Calls handlers for an event that takes a 32-bit signed interger argument.*/
void Event_RaiseInt32(Event_Int32* handlers, Int32 arg);
void Event_RegisterInt32(Event_Int32* handlers, Event_Int32_Callback handler);
void Event_UnregisterInt32(Event_Int32* handlers, Event_Int32_Callback handler);

/* Calls handlers for an event that takes a 32-bit floating point argument.*/
void Event_RaiseReal32(Event_Real32* handlers, Real32 arg);
void Event_RegisterReal32(Event_Real32* handlers, Event_Real32_Callback handler);
void Event_UnregisterReal32(Event_Real32* handlers, Event_Real32_Callback handler);

/* Calls handlers for an event that takes an entity ID argument.*/
void Event_RaiseEntityID(Event_EntityID* handlers, EntityID arg);
void Event_RegisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler);
void Event_UnregisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler);

/* Calls handlers for an event that takes a stream as an argument.*/
void Event_RaiseStream(Event_Stream* handlers, Stream* stream);
void Event_RegisterStream(Event_Stream* handlers, Event_Stream_Callback handler);
void Event_UnregisterStream(Event_Stream* handlers, Event_Stream_Callback handler);

/* Calls handlers for an event that takes a block change as an argument.*/
void Event_RaiseBlock(Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block);
void Event_RegisterBlock(Event_Block* handlers, Event_Block_Callback handler);
void Event_UnregisterBlock(Event_Block* handlers, Event_Block_Callback handler);
#endif