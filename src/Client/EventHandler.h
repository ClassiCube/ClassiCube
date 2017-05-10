#ifndef CS_EVENTHANDLER_H
#define CS_EVENTHANDLER_H
#include "Typedefs.h"
/* Helper method for managing events.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


/* Event handler that takes no arguments. */
typedef void (*Event_Void)(void);

/* Event handler that takes single 32 bit signed integer argument. */
typedef void (*Event_Int32)(Int32 argument);

/* Event handler that takes single floating-point argument. */
typedef void(*Event_Float32)(Real32 argument);


/* Maximum number of event handlers that can be registered. */
#define EventHandler_Size 32

/* Adds given event handler to handlers list for the given event. */
void EventHandler_Register(void** handlers, Int32* count, void* handler);

/* Removes given event handler from handlers list of the given event. */
void EventHandler_Unregister(void** handlers, Int32* count, void* handler);


/* Calls handlers for an event that has no arguments.*/
void EventHandler_Raise_Void(Event_Void* handlers, Int32 handlersCount);

/* Calls handlers for an event that takes a 32-bit signed interger argument.*/
void EventHandler_Raise_Int32(Event_Int32* handlers, Int32 handlersCount, Int32 arg);

/* Calls handlers for an event that takes a 32-bit floating point argument.*/
void EventHandler_Raise_Float32(Event_Float32* handlers, Int32 handlersCount, Real32 arg);
#endif