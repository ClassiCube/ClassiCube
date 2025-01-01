#ifndef CC_ENTITY_COMPONENTS_H
#define CC_ENTITY_COMPONENTS_H
#include "Vectors.h"
#include "Constants.h"
CC_BEGIN_HEADER

/* Various components for entities.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

struct Entity;
struct LocationUpdate;
struct LocalPlayer;

/* Entity component that performs model animation depending on movement speed and time */
struct AnimatedComp {
	float BobbingHor, BobbingVer, BobbingModel;
	float WalkTime, Swing, BobStrength;
	float WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	float LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	float LeftArmX, LeftArmZ, RightArmX, RightArmZ;
};

void AnimatedComp_Init(struct AnimatedComp* anim);
void AnimatedComp_Update(struct Entity* entity, Vec3 oldPos, Vec3 newPos, float delta);
void AnimatedComp_GetCurrent(struct Entity* entity, float t);

/* Entity component that performs tilt animation depending on movement speed and time */
struct TiltComp {
	float TiltX, TiltY, VelTiltStrength;
	float VelTiltStrengthO, VelTiltStrengthN;
};

void TiltComp_Init(struct TiltComp* anim);
void TiltComp_Update(struct LocalPlayer* p, struct TiltComp* anim, float delta);
void TiltComp_GetCurrent(struct LocalPlayer* p, struct TiltComp* anim, float t);

/* Entity component that performs management of hack states */
struct HacksComp {
	cc_bool IsOp;
	cc_bool Floating; /* true if NoClip or Flying */
	cc_bool _noclipping;
	/* Speed player move at, relative to normal speed, when the 'speeding' input binding is active */
	float SpeedMultiplier;
	/* Whether blocks that the player places that intersect themselves, should cause the player to
	be pushed back in the opposite direction of the placed block */
	cc_bool PushbackPlacing;
	/* Whether the player should be able to step up whole blocks, instead of just slabs */
	cc_bool FullBlockStep;
	/* Whether user has allowed hacks as an option. Note 'can use X' set by the server override this */
	cc_bool Enabled;

	cc_bool CanAnyHacks, CanUseThirdPerson, CanSpeed, CanFly;
	cc_bool CanRespawn, CanNoclip, CanPushbackBlocks,CanSeeAllNames;
	cc_bool CanDoubleJump, CanBePushed;
	float BaseHorSpeed;
	/* Max amount of jumps the player can perform */
	int MaxJumps;

	/* Whether the player should slide after letting go of movement buttons in noclip */
	cc_bool NoclipSlide;
	/* Whether the player has allowed the usage of fast double jumping abilities */
	cc_bool WOMStyleHacks; 

	cc_bool Noclip, Flying, FlyingUp, FlyingDown, Speeding, HalfSpeeding;
	float MaxHorSpeed;
	cc_string HacksFlags;
	char __HacksFlagsBuffer[STRING_SIZE * 2];	
};

void HacksComp_Init(struct HacksComp* hacks);
cc_bool HacksComp_CanJumpHigher(struct HacksComp* hacks);
/* Determines hacks permissions based on flags, then calls HacksComp_Update */
/* e.g. +ophax allows all hacks if op, -push disables entity pushing */
void HacksComp_RecheckFlags(struct HacksComp* hacks);
/* Updates state based on permissions (e.g. Flying set to false if CanFly is false) */
/* Raises UserEvents.HackPermsChanged */
void HacksComp_Update(struct HacksComp* hacks);
void HacksComp_SetFlying(struct HacksComp* hacks, cc_bool flying);
void HacksComp_SetNoclip(struct HacksComp* hacks, cc_bool noclip);
float HacksComp_CalcSpeedFactor(struct HacksComp* hacks, cc_bool canSpeed);

#define InterpComp_Layout int RotYCount; float RotYStates[15];
/* Base entity component that performs interpolation of position and orientation */
struct InterpComp { InterpComp_Layout };

void LocalInterpComp_SetLocation(struct InterpComp* interp, struct LocationUpdate* update, struct Entity* e);
void LocalInterpComp_AdvanceState(struct InterpComp* interp, struct Entity* e);

/* Represents a network orientation state */
struct NetInterpAngles { float Pitch, Yaw, RotX, RotZ; };

/* Entity component that performs interpolation for network players */
struct NetInterpComp {
	InterpComp_Layout
	/* Last known position and orientation sent by the server */
	Vec3 CurPos; struct NetInterpAngles CurAngles;
	/* Interpolated position and orientation state */
	int PositionsCount, AnglesCount;
	Vec3 Positions[10]; struct NetInterpAngles Angles[10];
};

void NetInterpComp_SetLocation(struct NetInterpComp* interp, struct LocationUpdate* update, struct Entity* e);
void NetInterpComp_AdvanceState(struct NetInterpComp* interp, struct Entity* e);

/* Entity component that performs collision detection */
struct CollisionsComp {
	struct Entity* Entity;
	cc_bool HitXMin, HitYMin, HitZMin, HitXMax, HitYMax, HitZMax, WasOn;
	float StepSize;
};
cc_bool Collisions_HitHorizontal(struct CollisionsComp* comp);
void Collisions_MoveAndWallSlide(struct CollisionsComp* comp);

/* Entity component that performs collisions */
struct PhysicsComp {
	cc_bool UseLiquidGravity; /* used by BlockDefinitions */
	cc_bool CanLiquidJump, Jumping;
	int MultiJumps;
	struct Entity* Entity;

	float JumpVel, UserJumpVel, ServerJumpVel;
	struct HacksComp* Hacks;
	struct CollisionsComp* Collisions;
};

void PhysicsComp_Init(struct PhysicsComp* comp, struct Entity* entity);
void PhysicsComp_UpdateVelocityState(struct PhysicsComp* comp);
void PhysicsComp_DoNormalJump(struct PhysicsComp* comp);
void PhysicsComp_PhysicsTick(struct PhysicsComp* comp, Vec3 vel);
float PhysicsComp_CalcJumpVelocity(float jumpHeight);
double PhysicsComp_CalcMaxHeight(float u);
void PhysicsComp_DoEntityPush(struct Entity* entity);

/* Entity component that plays block step sounds */
void SoundComp_Tick(struct LocalPlayer* p, cc_bool wasOnGround);

CC_END_HEADER
#endif
