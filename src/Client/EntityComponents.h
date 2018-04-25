#ifndef CC_ENTITY_COMPONENTS_H
#define CC_ENTITY_COMPONENTS_H
#include "Vectors.h"
#include "String.h"
/* Various components for entities.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Entity_ Entity;
typedef struct LocationUpdate_ LocationUpdate;

/* Entity component that performs model animation depending on movement speed and time */
typedef struct AnimatedComp_ {
	Real32 BobbingHor, BobbingVer, BobbingModel;
	Real32 WalkTime, Swing, BobStrength;
	Real32 WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	Real32 LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	Real32 LeftArmX, LeftArmZ, RightArmX, RightArmZ;
} AnimatedComp;

void AnimatedComp_Init(AnimatedComp* anim);
void AnimatedComp_Update(Entity* entity, Vector3 oldPos, Vector3 newPos, Real64 delta);
void AnimatedComp_GetCurrent(Entity* entity, Real32 t);

/* Entity component that performs tilt animation depending on movement speed and time */
typedef struct TiltComp_ {
	Real32 TiltX, TiltY, VelTiltStrength;
	Real32 VelTiltStrengthO, VelTiltStrengthN;
} TiltComp;

void TiltComp_Init(TiltComp* anim);
void TiltComp_Update(TiltComp* anim, Real64 delta);
void TiltComp_GetCurrent(TiltComp* anim, Real32 t);

/* Entity component that performs management of hack states */
typedef struct HacksComponent_ {
	UInt8 UserType;
	/* Speed player move at, relative to normal speed, when the 'speeding' key binding is held down */
	Real32 SpeedMultiplier;
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
	/* Maximum speed the entity can move at horizontally when CanSpeed is false */
	Real32 BaseHorSpeed;
	/* Max amount of jumps the player can perform */
	Int32 MaxJumps;

	/* Whether the player should slide after letting go of movement buttons in noclip */
	bool NoclipSlide;
	/* Whether the player has allowed the usage of fast double jumping abilities */
	bool WOMStyleHacks;

	bool Noclip, Flying, FlyingUp, FlyingDown, Speeding, HalfSpeeding;
	bool Floating; /* true if NoClip or Flying */
	UInt8 HacksFlagsBuffer[String_BufferSize(128)];
	String HacksFlags;
} HacksComp;

void HacksComp_Init(HacksComp* hacks);
bool HacksComp_CanJumpHigher(HacksComp* hacks);
bool HacksComp_Floating(HacksComp* hacks);
void HacksComp_SetUserType(HacksComp* hacks, UInt8 value, bool setBlockPerms);
void HacksComp_CheckConsistency(HacksComp* hacks);
void HacksComp_UpdateState(HacksComp* hacks);

/* Represents a position and orientation state */
typedef struct InterpState_ { Vector3 Pos; Real32 HeadX, HeadY, RotX, RotZ; } InterpState;

#define InterpComp_Layout \
InterpState Prev, Next; Real32 PrevRotY, NextRotY; \
Int32 RotYCount; Real32 RotYStates[15];

/* Base entity component that performs interpolation of position and orientation */
typedef struct InterpComp_ { InterpComp_Layout } InterpComp;

void InterpComp_LerpAngles(InterpComp* interp, Entity* entity, Real32 t);

void LocalInterpComp_SetLocation(InterpComp* interp, LocationUpdate* update, bool interpolate);
void LocalInterpComp_AdvanceState(InterpComp* interp);

/* Entity component that performs interpolation for network players */
typedef struct NetInterpComp_ {
	InterpComp_Layout
	/* Last known position and orientation sent by the server */
	InterpState Cur;
	Int32 StatesCount;
	InterpState States[10];
} NetInterpComp;

void NetInterpComp_SetLocation(NetInterpComp* interp, LocationUpdate* update, bool interpolate);
void NetInterpComp_AdvanceState(NetInterpComp* interp);

/* Entity component that draws square and circle shadows beneath entities */

bool ShadowComponent_BoundShadowTex;
GfxResourceID ShadowComponent_ShadowTex;
void ShadowComponent_Draw(Entity* entity);

/* Entity component that performs collision detection */
typedef struct CollisionsComp_ {
	Entity* Entity;
	bool HitXMin, HitYMin, HitZMin, HitXMax, HitYMax, HitZMax, WasOn;
} CollisionsComp;
bool Collisions_HitHorizontal(CollisionsComp* comp);
void Collisions_MoveAndWallSlide(CollisionsComp* comp);

/* Entity component that performs collisions */
typedef struct PhysicsComp_ {
	bool UseLiquidGravity; /* used by BlockDefinitions */
	bool CanLiquidJump, Jumping;
	Int32 MultiJumps;
	Entity* Entity;

	Real32 JumpVel, UserJumpVel, ServerJumpVel;
	HacksComp* Hacks;
	CollisionsComp* Collisions;
} PhysicsComp;

void PhysicsComp_Init(PhysicsComp* comp, Entity* entity);
void PhysicsComp_UpdateVelocityState(PhysicsComp* comp);
void PhysicsComp_DoNormalJump(PhysicsComp* comp);
void PhysicsComp_PhysicsTick(PhysicsComp* comp, Vector3 vel);
void PhysicsComp_CalculateJumpVelocity(PhysicsComp* comp, Real32 jumpHeight);
Real64 PhysicsComp_GetMaxHeight(Real32 u);
void PhysicsComp_DoEntityPush(Entity* entity);

/* Entity component that plays block step sounds */
void SoundComp_Tick(bool wasOnGround);
#endif