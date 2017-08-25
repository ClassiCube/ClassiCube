#ifndef CS_PLAYER_H
#define CS_PLAYER_H
#include "Typedefs.h"
#include "Entity.h"
/* Represents a player entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents a player entity. */
typedef struct Player_ {
	Entity Base;
} Player;


/* Represents the user/player's own entity. */
typedef struct LocalPlayer_ {
	Player Base;
	/* Position the player's position is set to when the 'respawn' key binding is pressed. */
	Vector3 Spawn;
	/* Orientation set to when player respawns.*/
	Real32 SpawnRotY, SpawnHeadX;
	/* Hacks state of this player. */
	HacksComponent Hacks;
} LocalPlayer;


/* Singleton instance of the local player. */
LocalPlayer LocalPlayer_Instance;
/* Initalises the local player instance. */
void LocalPlayer_Init(void);
#endif