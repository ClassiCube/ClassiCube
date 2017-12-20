#ifndef CC_PLAYER_H
#define CC_PLAYER_H
#include "Typedefs.h"
#include "Entity.h"
#include "Texture.h"
/* Represents a player entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Represents a player entity. */
typedef struct Player_ {
	Entity Base;
	UInt8 DisplayNameRaw[String_BufferSize(STRING_SIZE)];
	UInt8 SkinNameRaw[String_BufferSize(STRING_SIZE)];
	Texture NameTex;
	bool FetchedSkin;
} Player;
void Player_UpdateName(Player* player);
void Player_ResetSkin(Player* player);

/* Represents another entity in multiplayer */
typedef struct NetPlayer_ {
	Player Base;
	NetInterpComp Interp;
	bool ShouldRender;
} NetPlayer;

/* Represents the user/player's own entity. */
typedef struct LocalPlayer_ {
	Player Base;
	Vector3 Spawn;
	Real32 SpawnRotY, SpawnHeadX;	
	HacksComp Hacks; 	
	Real32 ReachDistance;
	TiltComp Tilt; 	
	InterpComp Interp;
} LocalPlayer;


LocalPlayer LocalPlayer_Instance;
void LocalPlayer_Init(void);
void NetPlayer_Init(NetPlayer* player);
#endif