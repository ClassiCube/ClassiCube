#ifndef CS_GAMESTRUCTS_H
#define CS_GAMESTRUCTS_H
#include "Typedefs.h"
/* Represents Game related structures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Callback function that takes no args. */
typedef void (*Void_Callback)(void);

/* Represents a game component. */
typedef struct IGameComponent {

	/* Called when the game is being loaded. */
	Void_Callback Init;

	/* Called when the component is being freed, due to game being closed. */
	Void_Callback Free;

	/* Called when the texture pack has been loaded and all components have been initalised. */
	Void_Callback Ready;

	/* Called to reset the component's state when the user is reconnecting to a server */
	Void_Callback Reset;

	/* Called to update the component's state when the user begins loading a new map. */
	Void_Callback OnNewMap;

	/* Called to update the component's state when the user has finished loading a new map. */
	Void_Callback OnNewMapLoaded;
} IGameComponent;

/* Makes an empty game component with all its function pointers initalised to null. */
IGameComponent IGameComponent_MakeEmpty(void);


/* Represents a task that runs on the main thread every certain interval. */
typedef struct ScheduledTask {

	/* How much time has elapsed since callback was last invoked. */
	Real64 Accumulator;
	
	/* How long (in seconds) between invocations of the callback. */
	Real64 Interval;

	/* Callback function that is periodically invoked. */
	void (*Callback)(struct ScheduledTask* task);
} ScheduledTask;
#endif