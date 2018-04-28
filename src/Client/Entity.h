#ifndef CC_ENTITY_H
#define CC_ENTITY_H
#include "Texture.h"
#include "EntityComponents.h"
#include "Physics.h"
#include "GameStructs.h"
#include "Constants.h"
/* Represents an in-game entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
typedef struct IModel_ IModel;

/* Offset used to avoid floating point roundoff errors. */
#define ENTITY_ADJUSTMENT 0.001f
/* Maxmimum number of characters in a model name. */
#define ENTITY_MAX_MODEL_LENGTH 11

#define ENTITIES_MAX_COUNT 256
#define ENTITIES_SELF_ID 255

typedef enum NameMode_ {
	NAME_MODE_NONE, NAME_MODE_HOVERED, NAME_MODE_ALL, NAME_MODE_ALL_HOVERED, NAME_MODE_ALL_UNSCALED, NAME_MODE_COUNT
} NameMode;
NameMode Entities_NameMode;
extern const UInt8* NameMode_Names[NAME_MODE_COUNT];

typedef enum ShadowMode_ {
	SHADOW_MODE_NONE, SHADOW_MODE_SNAP_TO_BLOCK, SHADOW_MODE_CIRCLE, SHADOW_MODE_CIRCLE_ALL, SHADOW_MODE_COUNT
} ShadowMode;
ShadowMode Entities_ShadowMode;
extern const UInt8* ShadowMode_Names[SHADOW_MODE_COUNT];

#define ENTITY_TYPE_NONE 0
#define ENTITY_TYPE_PLAYER 1

#define LOCATIONUPDATE_FLAG_POS   0x01
#define LOCATIONUPDATE_FLAG_HEADX 0x02
#define LOCATIONUPDATE_FLAG_HEADY 0x04
#define LOCATIONUPDATE_FLAG_ROTX  0x08
#define LOCATIONUPDATE_FLAG_ROTZ  0x10
/* Represents a location update for an entity. Can be a relative position, full position, and/or an orientation update. */
typedef struct LocationUpdate_ {
	Vector3 Pos;
	Real32 HeadX, HeadY, RotX, RotZ;
	UInt8 Flags;
	bool RelativePos;
} LocationUpdate;

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);
void LocationUpdate_Empty(LocationUpdate* update);
void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX);
void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel);
void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel);

typedef struct Entity_ Entity;
typedef struct EntityVTABLE_ {
	void (*Tick)(Entity* entity, Real64 delta);
	void (*SetLocation)(Entity* entity, LocationUpdate* update, bool interpolate);
	void (*RenderModel)(Entity* entity, Real64 deltaTime, Real32 t);
	void (*RenderName)(Entity* entity);
	void (*ContextLost)(Entity* entity);
	void (*ContextRecreated)(Entity* entity);
	void (*Despawn)(Entity* entity);
	PackedCol (*GetCol)(Entity* entity);
} EntityVTABLE;

/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
typedef struct Entity_ {
	EntityVTABLE* VTABLE;
	Vector3 Position;
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
	Vector3 Velocity, OldVelocity;

	IModel* Model;
	UInt8 ModelNameRaw[String_BufferSize(ENTITY_MAX_MODEL_LENGTH)];
	BlockID ModelBlock; /* BlockID, if model name was originally a vaid block id. */
	AABB ModelAABB;
	Vector3 ModelScale, Size;
	Real32 StepSize;
	
	UInt8 SkinType, EntityType;
	bool NoShade, OnGround;
	GfxResourceID TextureId, MobTextureId;
	Real32 uScale, vScale;
	Matrix Transform;

	AnimatedComp Anim;	
} Entity;

void Entity_Init(Entity* entity);
Vector3 Entity_GetEyePosition(Entity* entity);
void Entity_GetTransform(Entity* entity, Vector3 pos, Vector3 scale);
void Entity_GetPickingBounds(Entity* entity, AABB* bb);
void Entity_GetBounds(Entity* entity, AABB* bb);
void Entity_SetModel(Entity* entity, STRING_PURE String* model);
void Entity_UpdateModelBounds(Entity* entity);
bool Entity_TouchesAny(AABB* bb, bool(*touches_condition)(BlockID block__));
bool Entity_TouchesAnyRope(Entity* entity);	
bool Entity_TouchesAnyLava(Entity* entity);
bool Entity_TouchesAnyWater(Entity* entity);

Entity* Entities_List[ENTITIES_MAX_COUNT];
void Entities_Tick(ScheduledTask* task);
void Entities_RenderModels(Real64 delta, Real32 t);
void Entities_RenderNames(Real64 delta);
void Entities_RenderHoveredNames(Real64 delta);
void Entities_Init(void);
void Entities_Free(void);
void Entities_Remove(EntityID id);
EntityID Entities_GetCloset(Entity* src);
void Entities_DrawShadows(void);

#define TABLIST_MAX_NAMES 256
StringsBuffer TabList_Buffer;
UInt32 TabList_PlayerNames[TABLIST_MAX_NAMES];
UInt32 TabList_ListNames[TABLIST_MAX_NAMES];
UInt32 TabList_GroupNames[TABLIST_MAX_NAMES];
UInt8 TabList_GroupRanks[TABLIST_MAX_NAMES];
bool TabList_Valid(EntityID id);
bool TabList_Remove(EntityID id);
void TabList_Set(EntityID id, STRING_PURE String* player, STRING_PURE String* list,  STRING_PURE String* group, UInt8 rank);
IGameComponent TabList_MakeComponent(void);

#define TabList_UNSAFE_GetPlayer(id) StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_PlayerNames[id]);
#define TabList_UNSAFE_GetList(id)   StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_ListNames[id]);
#define TabList_UNSAFE_GetGroup(id)  StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_GroupNames[id]);


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
void NetPlayer_Init(NetPlayer* player, STRING_PURE String* displayName, STRING_PURE String* skinName);
NetPlayer NetPlayers_List[ENTITIES_SELF_ID];

/* Represents the user/player's own entity. */
typedef struct LocalPlayer_ {
	Player_Layout
	Vector3 Spawn;
	Real32 SpawnRotY, SpawnHeadX, ReachDistance;
	HacksComp Hacks;
	TiltComp Tilt;
	InterpComp Interp;
	CollisionsComp Collisions;
	PhysicsComp Physics;
} LocalPlayer;

LocalPlayer LocalPlayer_Instance;
IGameComponent LocalPlayer_MakeComponent(void);
void LocalPlayer_Init(void);
Real32 LocalPlayer_JumpHeight(void);
void LocalPlayer_CheckHacksConsistency(void);
void LocalPlayer_SetInterpPosition(Real32 t);
bool LocalPlayer_HandlesKey(Int32 key);
#endif