#ifndef CC_ENTITY_H
#define CC_ENTITY_H
#include "IModel.h"
#include "Typedefs.h"
#include "Vectors.h"
#include "AABB.h"
#include "GraphicsEnums.h"
#include "Matrix.h"
/* Represents an in-game entity. Also contains various components for entities.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct IModel_ IModel; /* Forward declaration */

/* Offset used to avoid floating point roundoff errors. */
#define ENTITY_ADJUSTMENT 0.001f
/* Special value specifying an angle is not included in an orientation update. */
#define LOCATIONUPDATE_EXCLUDED -100000.31415926535f
/* Maxmimum number of characters in a model name. */
#define ENTITY_MAX_MODEL_LENGTH 10

typedef bool (*TouchesAny_Condition)(BlockID block);

/* Represents a location update for an entity. Can be a relative position, full position, and/or an orientation update. */
typedef struct LocationUpdate_ {
	/* Position of the update (if included). */
	Vector3 Pos;
	/* Orientation of the update (if included). If not, has the value of LOCATIONUPDATE_EXCLUDED. */
	Real32 RotX, RotY, RotZ, HeadX;
	bool IncludesPosition, RelativePosition;
} LocationUpdate;

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);

void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos);
void LocationUpdate_Empty(LocationUpdate* update);
void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX);
void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel);
void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel);


/* Entity component that performs model animation depending on movement speed and time. */
typedef struct AnimatedComp_ {
	Real32 BobbingHor, BobbingVer, BobbingModel;
	Real32 WalkTime, Swing, BobStrength;
	Real32 WalkTimeO, WalkTimeN, SwingO, SwingN, BobStrengthO, BobStrengthN;

	Real32 LeftLegX, LeftLegZ, RightLegX, RightLegZ;
	Real32 LeftArmX, LeftArmZ, RightArmX, RightArmZ;
} AnimatedComp;

void AnimatedComp_Init(AnimatedComp* anim);
void AnimatedComp_Update(AnimatedComp* anim, Vector3 oldPos, Vector3 newPos, Real64 delta, bool onGround);
void AnimatedComp_GetCurrent(AnimatedComp* anim, Real32 t, bool calcHumanAnims);


/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity_ {
	Vector3 Position;
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
	Vector3 Velocity, OldVelocity;

	GfxResourceID TextureId, MobTextureId;
	AnimatedComp Anim;
	UInt8 SkinType;
	Real32 uScale, vScale;

	IModel* Model;
	UInt8 ModelNameRaw[String_BufferSize(ENTITY_MAX_MODEL_LENGTH)];
	BlockID ModelBlock; /* BlockID, if model name was originally a vaid block id. */
	AABB ModelAABB;
	Vector3 ModelScale;
	Vector3 Size;

	Matrix Transform;
	bool NoShade;

	/* TODO: SHOULD THESE BE A SEPARATE VTABLE STRUCT? (only need 1 shared pointer that way) */
	void (*SetLocation)(struct Entity_* entity, LocationUpdate* update, bool interpolate);
	PackedCol (*GetCol)(struct Entity_* entity);
} Entity;

void Entity_Init(Entity* entity);
Vector3 Entity_GetEyePosition(Entity* entity);
void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale);
void Entity_GetPickingBounds(Entity* entity, AABB* bb);
void Entity_GetBounds(Entity* entity, AABB* bb);
void Entity_SetModel(Entity* entity, STRING_PURE String* model);
void Entity_UpdateModelBounds(Entity* entity);
bool Entity_TouchesAny(AABB* bb, TouchesAny_Condition condition);
bool Entity_TouchesAnyRope(Entity* entity);	
bool Entity_TouchesAnyLava(Entity* entity);
bool Entity_TouchesAnyWater(Entity* entity);


/* Entity component that performs tilt animation depending on movement speed and time. */
typedef struct TiltComp_ {
	Real32 TiltX, TiltY, VelTiltStrength;
	Real32 VelTiltStrengthO, VelTiltStrengthN;
} TiltComp;

void TiltComp_Init(TiltComp* anim);
void TiltComp_Update(TiltComp* anim, Real64 delta);
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

	bool CanAnyHacks, CanUseThirdPersonCamera, CanSpeed, CanFly;
	bool CanRespawn, CanNoclip, CanPushbackBlocks,CanSeeAllNames;
	bool CanDoubleJump, CanBePushed;
	/* Maximum speed the entity can move at horizontally when CanSpeed is false. */
	Real32 BaseHorSpeed;
	/* Max amount of jumps the player can perform. */
	Int32 MaxJumps;

	/* Whether the player should slide after letting go of movement buttons in noclip.  */
	bool NoclipSlide;
	/* Whether the player has allowed the usage of fast double jumping abilities. */
	bool WOMStyleHacks;

	bool Noclip, Flying,FlyingUp, FlyingDown, Speeding, HalfSpeeding;
	UInt8 HacksFlagsBuffer[String_BufferSize(128)];
	String HacksFlags;
} HacksComp;

void HacksComp_Init(HacksComp* hacks);
bool HacksComp_CanJumpHigher(HacksComp* hacks);
bool HacksComp_Floating(HacksComp* hacks);
void HacksComp_SetUserType(HacksComp* hacks, UInt8 value);
void HacksComp_CheckConsistency(HacksComp* hacks);
void HacksComp_UpdateState(HacksComp* hacks);


/* Represents a position and orientation state. */
typedef struct InterpState_ {
	Vector3 Pos;
	Real32 HeadX, HeadY, RotX, RotZ;
} InterpState;

/* Base entity component that performs interpolation of position and orientation. */
typedef struct InterpComp_ {
	InterpState Prev, Next;
	Real32 PrevRotY, NextRotY;

	Int32 RotYCount;
	Real32 RotYStates[15];
} InterpComp;

void InterpComp_LerpAngles(InterpComp* interp, Entity* entity, Real32 t);

void LocalInterpComp_SetLocation(InterpComp* interp, LocationUpdate* update, bool interpolate);
void LocalInterpComp_AdvanceState(InterpComp* interp);

/* Entity component that performs interpolation for network players. */
typedef struct NetInterpComp_ {
	InterpComp Base;
	/* Last known position and orientation sent by the server. */
	InterpState Cur;
	Int32 StatesCount;
	InterpState States[10];
} NetInterpComp;

void NetInterpComp_SetLocation(NetInterpComp* interp, LocationUpdate* update, bool interpolate);
void NetInterpComp_AdvanceState(NetInterpComp* interp);
#endif