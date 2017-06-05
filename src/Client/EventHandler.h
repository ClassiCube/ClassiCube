#ifndef CS_EVENTHANDLER_H
#define CS_EVENTHANDLER_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
/* Helper method for managing events.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


/* Event handler that takes no arguments. */
typedef void(*Event_Void)(void);

/* Event handler that takes single 32 bit signed integer argument. */
typedef void(*Event_Int32)(Int32 argument);

/* Event handler that takes single floating-point argument. */
typedef void(*Event_Real32)(Real32 argument);

/* Event handler that takes an entity ID as an argument. */
typedef void(*Event_EntityID)(EntityID argument);

/* Event handler that takes stream as an argument. */
typedef void(*Event_Stream)(Stream* stream);


/* Maximum number of event handlers that can be registered. */
#define EventHandler_Size 32

/* Adds given event handler to handlers list for the given event. */
static void EventHandler_RegisterImpl(void** handlers, UInt32* count, void* handler);

/* Removes given event handler from handlers list of the given event. */
static void EventHandler_UnregisterImpl(void** handlers, UInt32* count, void* handler);


/* Calls handlers for an event that has no arguments.*/
void EventHandler_Raise_Void(Event_Void* handlers, UInt32 handlersCount);

#define EventHandler_RegisterVoid(eventName, handler) EventHandler_RegisterVoidImpl(eventName, &eventName ## Count, handler)
void EventHandler_RegisterVoidImpl(Event_Void* handlers, UInt32* count, Event_Void handler);

#define EventHandler_UnregisterVoid(eventName, handler) EventHandler_UnregisterVoidImpl(eventName, &eventName ## Count, handler)
void EventHandler_UnregisterVoidImpl(Event_Void* handlers, UInt32* count, Event_Void handler);


/* Calls handlers for an event that takes a 32-bit signed interger argument.*/
void EventHandler_Raise_Int32(Event_Int32* handlers, UInt32 handlersCount, Int32 arg);

#define EventHandler_RegisterInt32(eventName, handler) EventHandler_RegisterInt32Impl(eventName, &eventName ## Count, handler)
void EventHandler_RegisterInt32Impl(Event_Int32* handlers, UInt32* count, Event_Int32 handler);

#define EventHandler_UnregisterInt32(eventName, handler) EventHandler_UnregisterInt32Impl(eventName, &eventName ## Count, handler)
void EventHandler_UnregisterInt32Impl(Event_Int32* handlers, UInt32* count, Event_Int32 handler);


/* Calls handlers for an event that takes a 32-bit floating point argument.*/
void EventHandler_Raise_Real32(Event_Real32* handlers, UInt32 handlersCount, Real32 arg);

#define EventHandler_RegisterReal32(eventName, handler) EventHandler_RegisterFloat32Impl(eventName, &eventName ## Count, handler)
void EventHandler_RegisterReal32Impl(Event_Real32* handlers, UInt32* count, Event_Real32 handler);

#define EventHandler_UnregisterReal32(eventName, handler) EventHandler_UnregisterFloat32Impl(eventName, &eventName ## Count, handler)
void EventHandler_UnregisterReal32Impl(Event_Real32* handlers, UInt32* count, Event_Real32 handler);


/* Calls handlers for an event that takes an entity ID argument.*/
void EventHandler_Raise_EntityID(Event_EntityID* handlers, UInt32 handlersCount, EntityID arg);

#define EventHandler_RegisterEntityID(eventName, handler) EventHandler_RegisterEntityIDImpl(eventName, &eventName ## Count, handler)
void EventHandler_RegisterEntityIDImpl(Event_EntityID* handlers, UInt32* count, Event_EntityID handler);

#define EventHandler_UnregisterEntityID(eventName, handler) EventHandler_UnregisterEntityIDImpl(eventName, &eventName ## Count, handler)
void EventHandler_UnregisterEntityIDImpl(Event_EntityID* handlers, UInt32* count, Event_EntityID handler);


/* Calls handlers for an event that takes a stream as an argument.*/
void EventHandler_Raise_Stream(Event_Stream* handlers, UInt32 handlersCount, Stream* stream);

#define EventHandler_RegisterStream(eventName, handler) EventHandler_RegisterStreamImpl(eventName, &eventName ## Count, handler)
void EventHandler_RegisterStreamImpl(Event_Stream* handlers, UInt32* count, Event_Stream handler);

#define EventHandler_UnregisterStream(eventName, handler) EventHandler_UnregisterStreamImpl(eventName, &eventName ## Count, handler)
void EventHandler_UnregisterStreamImpl(Event_Stream* handlers, UInt32* count, Event_Stream handler);

#endif