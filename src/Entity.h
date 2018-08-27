#ifndef CC_ENTITY_H
#define CC_ENTITY_H
#include "EntityComponents.h"
#include "Physics.h"
#include "Constants.h"
#include "GraphicsCommon.h"
/* Represents an in-game entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IModel;
struct IGameComponent;
struct ScheduledTask;

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
extern const char* NameMode_Names[NAME_MODE_COUNT];

typedef enum ShadowMode_ {
	SHADOW_MODE_NONE, SHADOW_MODE_SNAP_TO_BLOCK, SHADOW_MODE_CIRCLE, SHADOW_MODE_CIRCLE_ALL, SHADOW_MODE_COUNT
} ShadowMode;
ShadowMode Entities_ShadowMode;
extern const char* ShadowMode_Names[SHADOW_MODE_COUNT];

#define ENTITY_TYPE_NONE 0
#define ENTITY_TYPE_PLAYER 1

#define LOCATIONUPDATE_FLAG_POS   0x01
#define LOCATIONUPDATE_FLAG_HEADX 0x02
#define LOCATIONUPDATE_FLAG_HEADY 0x04
#define LOCATIONUPDATE_FLAG_ROTX  0x08
#define LOCATIONUPDATE_FLAG_ROTZ  0x10
/* Represents a location update for an entity. Can be a relative position, full position, and/or an orientation update. */
struct LocationUpdate {
	Vector3 Pos;
	Real32 HeadX, HeadY, RotX, RotZ;
	UInt8 Flags;
	bool RelativePos;
};

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);
void LocationUpdate_MakeOri(struct LocationUpdate* update, Real32 rotY, Real32 headX);
void LocationUpdate_MakePos(struct LocationUpdate* update, Vector3 pos, bool rel);
void LocationUpdate_MakePosAndOri(struct LocationUpdate* update, Vector3 pos, Real32 rotY, Real32 headX, bool rel);

struct Entity;
struct EntityVTABLE {
	void (*Tick)(struct Entity* entity, Real64 delta);
	void (*SetLocation)(struct Entity* entity, struct LocationUpdate* update, bool interpolate);
	void (*RenderModel)(struct Entity* entity, Real64 deltaTime, Real32 t);
	void (*RenderName)(struct Entity* entity);
	void (*ContextLost)(struct Entity* entity);
	void (*ContextRecreated)(struct Entity* entity);
	void (*Despawn)(struct Entity* entity);
	PackedCol (*GetCol)(struct Entity* entity);
};

/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
struct Entity {
	struct EntityVTABLE* VTABLE;
	Vector3 Position;
	Real32 HeadX, HeadY, RotX, RotY, RotZ;
	Vector3 Velocity, OldVelocity;

	struct IModel* Model;
	BlockID ModelBlock; /* BlockID, if model name was originally a vaid block id. */
	bool ModelIsSheepNoFur; /* Hacky, but only sheep model relies on model name. So use just 1 byte. */
	struct AABB ModelAABB;
	Vector3 ModelScale, Size;
	Real32 StepSize;
	
	UInt8 SkinType, EntityType;
	bool NoShade, OnGround;
	GfxResourceID TextureId, MobTextureId;
	Real32 uScale, vScale;
	struct Matrix Transform;

	struct AnimatedComp Anim;
};

void Entity_Init(struct Entity* entity);
Vector3 Entity_GetEyePosition(struct Entity* entity);
Real32 Entity_GetEyeHeight(struct Entity* entity);
void Entity_GetTransform(struct Entity* entity, Vector3 pos, Vector3 scale, struct Matrix* m);
void Entity_GetPickingBounds(struct Entity* entity, struct AABB* bb);
void Entity_GetBounds(struct Entity* entity, struct AABB* bb);
void Entity_SetModel(struct Entity* entity, STRING_PURE String* model);
void Entity_UpdateModelBounds(struct Entity* entity);
bool Entity_TouchesAny(struct AABB* bb, bool(*touches_condition)(BlockID block__));
bool Entity_TouchesAnyRope(struct Entity* entity);	
bool Entity_TouchesAnyLava(struct Entity* entity);
bool Entity_TouchesAnyWater(struct Entity* entity);

struct Entity* Entities_List[ENTITIES_MAX_COUNT];
void Entities_Tick(struct ScheduledTask* task);
void Entities_RenderModels(Real64 delta, Real32 t);
void Entities_RenderNames(Real64 delta);
void Entities_RenderHoveredNames(Real64 delta);
void Entities_Init(void);
void Entities_Free(void);
void Entities_Remove(EntityID id);
EntityID Entities_GetCloset(struct Entity* src);
void Entities_DrawShadows(void);

#define TABLIST_MAX_NAMES 256
StringsBuffer TabList_Buffer;
UInt16 TabList_PlayerNames[TABLIST_MAX_NAMES];
UInt16 TabList_ListNames[TABLIST_MAX_NAMES];
UInt16 TabList_GroupNames[TABLIST_MAX_NAMES];
UInt8  TabList_GroupRanks[TABLIST_MAX_NAMES];
bool TabList_Valid(EntityID id);
bool TabList_Remove(EntityID id);
void TabList_Set(EntityID id, STRING_PURE String* player, STRING_PURE String* list, STRING_PURE String* group, UInt8 rank);
void TabList_MakeComponent(struct IGameComponent* comp);

#define TabList_UNSAFE_GetPlayer(id) StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_PlayerNames[id]);
#define TabList_UNSAFE_GetList(id)   StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_ListNames[id]);
#define TabList_UNSAFE_GetGroup(id)  StringsBuffer_UNSAFE_Get(&TabList_Buffer, TabList_GroupNames[id]);
#define Player_Layout struct Entity Base; char DisplayNameRaw[STRING_SIZE]; char SkinNameRaw[STRING_SIZE]; bool FetchedSkin; struct Texture NameTex;

/* Represents a player entity. */
struct Player { Player_Layout };
void Player_UpdateName(struct Player* player);
void Player_ResetSkin(struct Player* player);

/* Represents another entity in multiplayer */
struct NetPlayer {
	Player_Layout
	struct NetInterpComp Interp;
	bool ShouldRender;
};
void NetPlayer_Init(struct NetPlayer* player, STRING_PURE String* displayName, STRING_PURE String* skinName);
struct NetPlayer NetPlayers_List[ENTITIES_SELF_ID];

/* Represents the user/player's own entity. */
struct LocalPlayer {
	Player_Layout
	Vector3 Spawn;
	Real32 SpawnRotY, SpawnHeadX, ReachDistance;
	struct HacksComp Hacks;
	struct TiltComp Tilt;
	struct InterpComp Interp;
	struct CollisionsComp Collisions;
	struct PhysicsComp Physics;
};

struct LocalPlayer LocalPlayer_Instance;
void LocalPlayer_MakeComponent(struct IGameComponent* comp);
void LocalPlayer_Init(void);
Real32 LocalPlayer_JumpHeight(void);
void LocalPlayer_CheckHacksConsistency(void);
void LocalPlayer_SetInterpPosition(Real32 t);
bool LocalPlayer_HandlesKey(Int32 key);
#endif
