#ifndef CC_PLAYER_H
#define CC_PLAYER_H
#include "Typedefs.h"
#include "Entity.h"
#include "Texture.h"
/* Represents a player entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

#define Player_Layout Entity Base; UInt8 DisplayNameRaw[String_BufferSize(STRING_SIZE)]; \
UInt8 SkinNameRaw[String_BufferSize(STRING_SIZE)]; bool FetchedSkin; Texture NameTex;

/* Represents a player entity. */
typedef struct Player_ { Player_Layout } Player;
void Player_UpdateName(Player* player);
void Player_ResetSkin(Player* player);

/* Represents another entity in multiplayer */
typedef struct NetPlayer_ {
	Player_Layout
	NetInterpComp Interp;
	bool ShouldRender;
} NetPlayer;

/* Represents the user/player's own entity. */
typedef struct LocalPlayer_ {
	Player_Layout
	Vector3 Spawn;
	Real32 SpawnRotY, SpawnHeadX;	
	Real32 ReachDistance;
	HacksComp Hacks; 	
	TiltComp Tilt; 	
	InterpComp Interp;
} LocalPlayer;


LocalPlayer LocalPlayer_Instance;
void LocalPlayer_Init(void);
void NetPlayer_Init(NetPlayer* player);
#endif