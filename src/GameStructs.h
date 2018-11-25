#ifndef CC_GAMESTRUCTS_H
#define CC_GAMESTRUCTS_H
#include "Core.h"
/* Represents Game related structures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

/* Represents a game component. */
struct IGameComponent {
	/* Called when the game is being loaded. */
	void (*Init)(void);
	/* Called when the component is being freed. (e.g. due to game being closed) */
	void (*Free)(void);
	/* Called to reset the component's state. (e.g. reconnecting to server) */
	void (*Reset)(void);
	/* Called to update the component's state when the user begins loading a new map. */
	void (*OnNewMap)(void);
	/* Called to update the component's state when the user has finished loading a new map. */
	void (*OnNewMapLoaded)(void);
	/* Called when the texture pack has been loaded and all components have been initialised. */
	void (*Ready)(void);
	/* Next component in linked list of components. */
	struct IGameComponent* Next;
};
CC_NOINLINE void Game_AddComponent(struct IGameComponent* comp);

/* Represents a task that periodically runs on the main thread every specified interval. */
struct ScheduledTask;
struct ScheduledTask {
	/* How long (in seconds) has elapsed since callback was last invoked */
	double Accumulator;
	/* How long (in seconds) between invocations of the callback */
	double Interval;
	/* Callback function that is periodically invoked */
	void (*Callback)(struct ScheduledTask* task);
};

typedef void (*ScheduledTaskCallback)(struct ScheduledTask* task);
CC_NOINLINE int ScheduledTask_Add(double interval, ScheduledTaskCallback callback);
#endif
