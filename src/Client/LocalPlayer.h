#ifndef CS_LOCALPLAYER_H
#define CS_LOCALPLAYER_H
#include "Typedefs.h"
#include "Player.h"
/* Represents the user/player's own entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct LocalPlayer_ {
	Player Base;

	/* Position the player's position is set to when the 'respawn' key binding is pressed. */
	Vector3 Spawn;

	/* Orientation set to when player respawns.*/
	Real32 SpawnRotY, SpawnHeadX;
} LocalPlayer;


/* Singleton instance of the local player. */
LocalPlayer LocalPlayer_Instance;

/* Initalises the local player instance. */
void LocalPlayer_Init(void);
#endif