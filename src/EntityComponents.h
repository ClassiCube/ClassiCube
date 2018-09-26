#ifndef CC_ENTITY_COMPONENTS_H
#define CC_ENTITY_COMPONENTS_H
#include "Vectors.h"
#include "String.h"
#include "Constants.h"
/* Various components for entities.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct Entity;
struct LocationUpdate;

/* Entity component that performs model animation depending on movement speed and time */
struct AnimatedComp {
	float BobbingHor, BobbingVer, BobbingModel;
	float WalkTime, Swing, BobStrength;
	float WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	float LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	float LeftArmX, LeftArmZ, RightArmX, RightArmZ;
};

void AnimatedComp_Init(struct AnimatedComp* anim);
void AnimatedComp_Update(struct Entity* entity, Vector3 oldPos, Vector3 newPos, double delta);
void AnimatedComp_GetCurrent(struct Entity* entity, float t);

/* Entity component that performs tilt animation depending on movement speed and time */
struct TiltComp {
	float TiltX, TiltY, VelTiltStrength;
	float VelTiltStrengthO, VelTiltStrengthN;
};

void TiltComp_Init(struct TiltComp* anim);
void TiltComp_Update(struct TiltComp* anim, double delta);
void TiltComp_GetCurrent(struct TiltComp* anim, float t);

/* Entity component that performs management of hack states */
struct HacksComp {
	UInt8 UserType;
	/* Speed player move at, relative to normal speed, when the 'speeding' key binding is held down */
	float SpeedMultiplier;
	/* Whether blocks that the player places that intersect themselves, should cause the player to
	be pushed back in the opposite direction of the placed block */
	bool PushbackPlacing;
	/* Whether the player should be able to step up whole blocks, instead of just slabs */
	bool FullBlockStep;
	/* Whether the player has allowed hacks usage as an option. Note 'can use X' set by the server override this */
	bool Enabled;

	bool CanAnyHacks, CanUseThirdPersonCamera, CanSpeed, CanFly;
	bool CanRespawn, CanNoclip, CanPushbackBlocks,CanSeeAllNames;
	bool CanDoubleJump, CanBePushed;
	float BaseHorSpeed;
	/* Max amount of jumps the player can perform */
	Int32 MaxJumps;

	/* Whether the player should slide after letting go of movement buttons in noclip */
	bool NoclipSlide;
	/* Whether the player has allowed the usage of fast double jumping abilities */
	bool WOMStyleHacks;

	bool Noclip, Flying, FlyingUp, FlyingDown, Speeding, HalfSpeeding;
	bool Floating; /* true if NoClip or Flying */
	String HacksFlags;
	char __HacksFlagsBuffer[STRING_SIZE * 2];	
};

void HacksComp_Init(struct HacksComp* hacks);
bool HacksComp_CanJumpHigher(struct HacksComp* hacks);
bool HacksComp_Floating(struct HacksComp* hacks);
void HacksComp_SetUserType(struct HacksComp* hacks, UInt8 value, bool setBlockPerms);
void HacksComp_CheckConsistency(struct HacksComp* hacks);
void HacksComp_UpdateState(struct HacksComp* hacks);

/* Represents a position and orientation state */
struct InterpState { Vector3 Pos; float HeadX, HeadY, RotX, RotZ; };

#define InterpComp_Layout \
struct InterpState Prev, Next; float PrevRotY, NextRotY; \
Int32 RotYCount; float RotYStates[15];

/* Base entity component that performs interpolation of position and orientation */
struct InterpComp { InterpComp_Layout };

void InterpComp_LerpAngles(struct InterpComp* interp, struct Entity* entity, float t);

void LocalInterpComp_SetLocation(struct InterpComp* interp, struct LocationUpdate* update, bool interpolate);
void LocalInterpComp_AdvanceState(struct InterpComp* interp);

/* Entity component that performs interpolation for network players */
struct NetInterpComp {
	InterpComp_Layout
	/* Last known position and orientation sent by the server */
	struct InterpState Cur;
	Int32 StatesCount;
	struct InterpState States[10];
};

void NetInterpComp_SetLocation(struct NetInterpComp* interp, struct LocationUpdate* update, bool interpolate);
void NetInterpComp_AdvanceState(struct NetInterpComp* interp);

/* Entity component that draws square and circle shadows beneath entities */

bool ShadowComponent_BoundShadowTex;
GfxResourceID ShadowComponent_ShadowTex;
void ShadowComponent_Draw(struct Entity* entity);

/* Entity component that performs collision detection */
struct CollisionsComp {
	struct Entity* Entity;
	bool HitXMin, HitYMin, HitZMin, HitXMax, HitYMax, HitZMax, WasOn;
};
bool Collisions_HitHorizontal(struct CollisionsComp* comp);
void Collisions_MoveAndWallSlide(struct CollisionsComp* comp);

/* Entity component that performs collisions */
struct PhysicsComp {
	bool UseLiquidGravity; /* used by BlockDefinitions */
	bool CanLiquidJump, Jumping;
	Int32 MultiJumps;
	struct Entity* Entity;

	float JumpVel, UserJumpVel, ServerJumpVel;
	struct HacksComp* Hacks;
	struct CollisionsComp* Collisions;
};

void PhysicsComp_Init(struct PhysicsComp* comp, struct Entity* entity);
void PhysicsComp_UpdateVelocityState(struct PhysicsComp* comp);
void PhysicsComp_DoNormalJump(struct PhysicsComp* comp);
void PhysicsComp_PhysicsTick(struct PhysicsComp* comp, Vector3 vel);
void PhysicsComp_CalculateJumpVelocity(struct PhysicsComp* comp, float jumpHeight);
double PhysicsComp_GetMaxHeight(float u);
void PhysicsComp_DoEntityPush(struct Entity* entity);

/* Entity component that plays block step sounds */
void SoundComp_Tick(bool wasOnGround);
#endif
