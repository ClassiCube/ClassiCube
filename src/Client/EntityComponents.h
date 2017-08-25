#ifndef CS_ENTITYCOMPS_H
#define CS_ENTITYCOMPS_H
#include "Typedefs.h"
#include "Vectors.h"
#include "String.h"
/* Contains various components for entities.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Entity component that performs model animation depending on movement speed and time. */
typedef struct AnimatedComp_ {
	Real32 BobbingHor, BobbingVer, BobbingModel;
	Real32 WalkTime, Swing, BobStrength;
	Real32 WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	Real32 LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	Real32 LeftArmX, LeftArmZ, RightArmX, RightArmZ;
} AnimatedComp;

/* Initalises default values for an animated component. */
void AnimatedComp_Init(AnimatedComp* anim);
/* Calculates the next model animation state based on old and new position. */
void AnimatedComp_Update(AnimatedComp* anim, Vector3 oldPos, Vector3 newPos, Real64 delta, bool onGround);
/*  Calculates the interpolated state between the last and next model animation state. */
void AnimatedComp_GetCurrent(AnimatedComp* anim, Real32 t, bool calcHumanAnims);


/* Entity component that performs tilt animation depending on movement speed and time. */
typedef struct TiltComp_ {
	Real32 TiltX, TiltY, VelTiltStrength;
	Real32 VelTiltStrengthO, VelTiltStrengthN;
} TiltComp;


/* Initalises default values for a tilt component. */
void TiltComp_Init(TiltComp* anim);
/* Calculates the next tilt animation state. */
void TiltComp_Update(TiltComp* anim, Real64 delta);
/*  Calculates the interpolated state between the last and next tilt animation state. */
void TiltComp_GetCurrent(TiltComp* anim, Real32 t);



/* Entity component that performs management of hack states. */
typedef struct HacksComponent_ {
	UInt8 UserType;
	/* Speed player move at, relative to normal speed, when the 'speeding' key binding is held down. */
	Real32 SpeedMultiplier;
	/* Whether blocks that the player places that intersect themselves, should cause the player to
	be pushed back in the opposite direction of the placed block. */
	bool PushbackPlacing;
	/* Whether the player should be able to step up whole blocks, instead of just slabs. */
	bool FullBlockStep;

	/* Whether the player has allowed hacks usage as an option. Note 'can use X' set by the server override this. */
	bool Enabled;
	/* Whether the player is allowed to use any type of hacks. */
	bool CanAnyHacks;
	/* Whether the player is allowed to use the types of cameras that use third person. */
	bool CanUseThirdPersonCamera;
	/* Whether the player is allowed to increase its speed beyond the normal walking speed. */
	bool CanSpeed;
	/* Whether the player is allowed to fly in the world. */
	bool CanFly;
	/* Whether the player is allowed to teleport to their respawn coordinates. */
	bool CanRespawn;
	/* Whether the player is allowed to pass through all blocks. */
	bool CanNoclip;
	/* Whether the player is allowed to use pushback block placing. */
	bool CanPushbackBlocks;
	/* Whether the player is allowed to see all entity name tags. */
	bool CanSeeAllNames;
	/* Whether the player is allowed to double jump. */
	bool CanDoubleJump;
	/* Maximum speed the entity can move at horizontally when CanSpeed is false. */
	Real32 MaxSpeedMultiplier;

	/* Whether the player should slide after letting go of movement buttons in noclip.  */
	bool NoclipSlide;
	/* Whether the player has allowed the usage of fast double jumping abilities. */
	bool WOMStyleHacks;

	/* Whether the player currently has noclip on. */
	bool Noclip;
	/* Whether the player currently has fly mode active. */
	bool Flying;
	/* Whether the player is currently flying upwards. */
	bool FlyingUp;
	/* Whether the player is currently flying downwards. */
	bool FlyingDown;
	/* Whether the player is currently walking at base speed * speed multiplier. */
	bool Speeding;
	/* Whether the player is currently walking at base speed * 0.5 * speed multiplier. */
	bool HalfSpeeding;

	UInt8 HacksFlagsBuffer[String_BufferSize(128)];
	/* The actual hack flags usage string.*/
	String HacksFlags;
} HacksComp;

/* Initalises the state of this HacksComponent. */
void HacksComponent_Init(HacksComp* hacks);

/* Returns whether hacks flags allow for jumping higher usage. */
bool HacksComponent_CanJumpHigher(HacksComp* hacks);

/* Returns whether noclip or flying modes are on.*/
bool HacksComponent_Floating(HacksComp* hacks);

/* Sets the user type of this user. This is used to control permissions for grass,
bedrock, water and lava blocks on servers that don't support CPE block permissions. */
void HacksComponent_SetUserType(HacksComp* hacks, UInt8 value);

/* Disables any hacks if their respective CanHackX value is set to false. */
void HacksComponent_CheckConsistency(HacksComp* hacks);

/* Updates ability to use hacks, and raises UserEvents_HackPermissionsChanged event.
Parses hack flags specified in the motd and/or name of the server.
Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz */
void HacksComponent_UpdateState(HacksComp* hacks);
#endif