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

/* Represents a location update for an entity.
This can be a relative position, full position, and/or an orientation update. */
typedef struct LocationUpdate_ {
	/* Position of the update (if included). */
	Vector3 Pos;
	/* Orientation of the update (if included). If not, has the value of LOCATIONUPDATE_EXCLUDED. */
	Real32 RotX, RotY, RotZ, HeadX;
	/* Whether this update includes an absolute or relative position. */
	bool IncludesPosition;
	/* Whether the positon is absolute, or relative to the last positionreceived from the server. */
	bool RelativePosition;
} LocationUpdate;

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);

/* Constructs a location update with values for every field.
You should generally prefer using the alternative constructors. */
void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos);
/* Constructs a location update that does not have any position or orientation information. */
void LocationUpdate_Empty(LocationUpdate* update);
/* Constructs a location update that only consists of orientation information. */
void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX);
/* Constructs a location update that only consists of position information. */
void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel);
/* Constructs a location update that consists of position and orientation information. */
void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel);


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


/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity_ {
	/* Position of the entity in the world.*/
	Vector3 Position;
	/* Rotation of the entity in the world. */
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
	/* Velocity the entity is moving at. */
	Vector3 Velocity, OldVelocity;
	
	/* Texture IDs for this entity's skin. */
	GfxResourceID TextureId, MobTextureId;
	/* Animation component of the entity. */
	AnimatedComp Anim;

	/* Skin type of this entity. */
	UInt8 SkinType;
	/* U and V scale of this entity, usually 1. */
	Real32 uScale, vScale;

	/* The model of this entity. (used for collision detection and rendering) */
	IModel* Model;
	UInt8 ModelNameBuffer[String_BufferSize(ENTITY_MAX_MODEL_LENGTH)];
	/* The name of the model of this entity. Is "block" for all block ID models. */
	String ModelName;		
	/* BlockID, if model name was originally a vaid block id. Avoids needing to repeatedly parse ModelName as a byte. */
	BlockID ModelBlock;
	/* AABB of the model. Centred around the origin, NOT the position. */
	AABB ModelAABB;
	/* Scale applied to the model on each axis. */
	Vector3 ModelScale;
	/* Returns the size of the model that is used for collision detection. */
	Vector3 Size;

	/* Transformation matrix of this entity when rendering */
	Matrix Transform;
	/* Whether no shading should be applied to the faces of model parts of this entity's model. */
	bool NoShade;

	/* TODO: SHOULD THESE BE A SEPARATE VTABLE STRUCT? (only need 1 shared pointer that way) */
	/* Sets the location of this entity. */
	void (*SetLocation)(struct Entity_* entity, LocationUpdate* update, bool interpolate);
	/* Gets the colour of this entity for rendering. */
	PackedCol (*GetCol)(struct Entity_* entity);
} Entity;


/* Initalises the given entity. */
void Entity_Init(Entity* entity);
/* Gets the position of the player's eye in the world. */
Vector3 Entity_GetEyePosition(Entity* entity);
/* Calculates the transformation matrix for the given entity. */
void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale);
/* Returns the bounding box that contains the model, without any rotations applied. */
void Entity_GetPickingBounds(Entity* entity, AABB* bb);
/* Bounding box of the model that collision detection is performed with, in world coordinates. */
void Entity_GetBounds(Entity* entity, AABB* bb);
/* Sets the model associated with this entity. ('name' or 'name|scale') */
void Entity_SetModel(Entity* entity, STRING_PURE String* model);
void Entity_UpdateModelBounds(Entity* entity);
/* Determines whether any of the blocks that intersect the given bounding box satisfy the given condition. */
bool Entity_TouchesAny(AABB* bb, TouchesAny_Condition condition);
/* Determines whether any of the blocks that intersect the AABB of this entity are rope. */
bool Entity_TouchesAnyRope(Entity* entity);	
/* Determines whether any of the blocks that intersect the AABB of this entity are lava or still lava. */
bool Entity_TouchesAnyLava(Entity* entity);
/* Determines whether any of the blocks that intersect the AABB of this entity are water or still water. */
bool Entity_TouchesAnyWater(Entity* entity);


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
	/* Whether the player can be pushed by other players. */
	bool CanBePushed;
	/* Maximum speed the entity can move at horizontally when CanSpeed is false. */
	Real32 MaxSpeedMultiplier;
	/* Max amount of jumps the player can perform. */
	Int32 MaxJumps;

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
void HacksComp_Init(HacksComp* hacks);
/* Returns whether hacks flags allow for jumping higher usage. */
bool HacksComp_CanJumpHigher(HacksComp* hacks);
/* Returns whether noclip or flying modes are on.*/
bool HacksComp_Floating(HacksComp* hacks);
/* Sets the user type of this user. This is used to control permissions for grass,
bedrock, water and lava blocks on servers that don't support CPE block permissions. */
void HacksComp_SetUserType(HacksComp* hacks, UInt8 value);
/* Disables any hacks if their respective CanHackX value is set to false. */
void HacksComp_CheckConsistency(HacksComp* hacks);
/* Updates ability to use hacks, and raises UserEvents_HackPermissionsChanged event.
Parses hack flags specified in the motd and/or name of the server.
Recognises +/-hax, +/-fly, +/-noclip, +/-speed, +/-respawn, +/-ophax, and horspeed=xyz */
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