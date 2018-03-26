#ifndef CC_GAMESTRUCTS_H
#define CC_GAMESTRUCTS_H
#include "Typedefs.h"
/* Represents Game related structures.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents a game component. */
typedef struct IGameComponent_ {
	/* Called when the game is being loaded. */
	void (*Init)(void);
	/* Called when the component is being freed, due to game being closed. */
	void (*Free)(void);
	/* Called when the texture pack has been loaded and all components have been initalised. */
	void (*Ready)(void);
	/* Called to reset the component's state when the user is reconnecting to a server */
	void (*Reset)(void);
	/* Called to update the component's state when the user begins loading a new map. */
	void (*OnNewMap)(void);
	/* Called to update the component's state when the user has finished loading a new map. */
	void (*OnNewMapLoaded)(void);
} IGameComponent;

IGameComponent IGameComponent_MakeEmpty(void);
#define GAME_MAX_COMPONENTS 26
IGameComponent Game_Components[GAME_MAX_COMPONENTS];
Int32 Game_ComponentsCount;
void Game_AddComponent(IGameComponent* comp);

/* Represents a task that periodically runs on the main thread every specified interval. */
typedef struct ScheduledTask_ {
	/* How long (in seconds) has elapsed since callback was last invoked. */
	Real64 Accumulator;	
	/* How long (in seconds) between invocations of the callback. */
	Real64 Interval;
	/* Callback function that is periodically invoked. */
	void (*Callback)(struct ScheduledTask_* task);
} ScheduledTask;

typedef void (*ScheduledTaskCallback)(ScheduledTask* task);
#endif