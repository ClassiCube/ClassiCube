#ifndef CC_ENTITY_H
#define CC_ENTITY_H
#include "EntityComponents.h"
#include "Physics.h"
#include "Constants.h"
#include "Input.h"
#include "PackedCol.h"
/* Represents an in-game entity.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct Model;
struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent TabList_Component;
extern struct IGameComponent Entities_Component;

/* Offset used to avoid floating point roundoff errors. */
#define ENTITY_ADJUSTMENT 0.001f
#define ENTITIES_MAX_COUNT 256
#define ENTITIES_SELF_ID 255

enum NameMode {
	NAME_MODE_NONE, NAME_MODE_HOVERED, NAME_MODE_ALL, NAME_MODE_ALL_HOVERED, NAME_MODE_ALL_UNSCALED, NAME_MODE_COUNT
};
extern const char* NameMode_Names[NAME_MODE_COUNT];

enum ShadowMode {
	SHADOW_MODE_NONE, SHADOW_MODE_SNAP_TO_BLOCK, SHADOW_MODE_CIRCLE, SHADOW_MODE_CIRCLE_ALL, SHADOW_MODE_COUNT
};
extern const char* ShadowMode_Names[SHADOW_MODE_COUNT];

enum EntityType { ENTITY_TYPE_NONE, ENTITY_TYPE_PLAYER };

#define LOCATIONUPDATE_FLAG_POS   0x01
#define LOCATIONUPDATE_FLAG_HEADX 0x02
#define LOCATIONUPDATE_FLAG_HEADY 0x04
#define LOCATIONUPDATE_FLAG_ROTX  0x08
#define LOCATIONUPDATE_FLAG_ROTZ  0x10
/* Represents a location update for an entity. Can be a relative position, full position, and/or an orientation update. */
struct LocationUpdate {
	Vector3 Pos;
	float HeadX, HeadY, RotX, RotZ;
	uint8_t Flags;
	bool RelativePos;
};

/* Clamps the given angle so it lies between [0, 360). */
float LocationUpdate_Clamp(float degrees);
/* Makes a location update only containing yaw and pitch. */
void LocationUpdate_MakeOri(struct LocationUpdate* update, float rotY, float headX);
/* Makes a location update only containing position */
void LocationUpdate_MakePos(struct LocationUpdate* update, Vector3 pos, bool rel);
/* Makes a location update containing position, yaw and pitch. */
void LocationUpdate_MakePosAndOri(struct LocationUpdate* update, Vector3 pos, float rotY, float headX, bool rel);

struct Entity;
struct EntityVTABLE {
	void (*Tick)(struct Entity* e, double delta);
	void (*Despawn)(struct Entity* e);
	void (*SetLocation)(struct Entity* e, struct LocationUpdate* update, bool interpolate);
	PackedCol (*GetCol)(struct Entity* e);
	void (*RenderModel)(struct Entity* e, double deltaTime, float t);
	void (*RenderName)(struct Entity* e);
	void (*ContextLost)(struct Entity* e);
	void (*ContextRecreated)(struct Entity* e);
};

/* Contains a model, along with position, velocity, and rotation. May also contain other fields and properties. */
struct Entity {
	struct EntityVTABLE* VTABLE;
	Vector3 Position;
	float HeadX, HeadY, RotX, RotY, RotZ;
	Vector3 Velocity;

	struct Model* Model;
	BlockID ModelBlock; /* BlockID, if model name was originally a valid block. */
	struct AABB ModelAABB;
	Vector3 ModelScale, Size;
	float StepSize;
	
	uint8_t SkinType, EntityType;
	bool NoShade, OnGround;
	GfxResourceID TextureId, MobTextureId;
	float uScale, vScale;
	struct Matrix Transform;

	struct AnimatedComp Anim;
	char SkinNameRaw[STRING_SIZE];
};
typedef bool (*Entity_TouchesCondition)(BlockID block);

/* Initialises non-zero fields of the given entity. */
void Entity_Init(struct Entity* e);
/* Gets the position of the eye of the given entity's model. */
Vector3 Entity_GetEyePosition(struct Entity* e);
/* Returns the height of the eye of the given entity's model. */
/* (i.e. distance to eye from feet/base of the model) */
float Entity_GetEyeHeight(struct Entity* e);
/* Calculates the transformation matrix applied when rendering the given entity. */
void Entity_GetTransform(struct Entity* e, Vector3 pos, Vector3 scale, struct Matrix* m);
void Entity_GetPickingBounds(struct Entity* e, struct AABB* bb);
/* Gets the current collision bounds of the given entity. */
void Entity_GetBounds(struct Entity* e, struct AABB* bb);
/* Sets the model of the entity. (i.e its appearance) */
void Entity_SetModel(struct Entity* e, const String* model);
/* Updates cached Size and ModelAABB of the given entity. */
/* NOTE: Only needed when manually changing Model or ModelScale. */
/* Entity_SetModel implicitly call this method. */
void Entity_UpdateModelBounds(struct Entity* e);

/* Whether the given entity is touching any blocks meeting the given condition .*/
bool Entity_TouchesAny(struct AABB* bb, Entity_TouchesCondition cond);
bool Entity_TouchesAnyRope(struct Entity* e);	
bool Entity_TouchesAnyLava(struct Entity* e);
bool Entity_TouchesAnyWater(struct Entity* e);

/* Global data for all entities */
/* (Actual entities may point to NetPlayers_List or elsewhere) */
extern struct _EntitiesData {
	struct Entity* List[ENTITIES_MAX_COUNT];
	uint8_t NamesMode, ShadowsMode;
} Entities;

/* Ticks all entities. */
void Entities_Tick(struct ScheduledTask* task);
/* Renders all entities. */
void Entities_RenderModels(double delta, float t);
/* Renders the name tags of entities, depending on Entities.NamesMode. */
void Entities_RenderNames(double delta);
/* Renders hovered entity name tags. (these appears through blocks) */
void Entities_RenderHoveredNames(double delta);
/* Removes the given entity, raising EntityEvents.Removed event. */
void Entities_Remove(EntityID id);
/* Gets the ID of the closest entity to the given entity. */
EntityID Entities_GetCloset(struct Entity* src);
/* Draws shadows under entities, depending on Entities.ShadowsMode */
void Entities_DrawShadows(void);

#define TABLIST_MAX_NAMES 256
/* Data for all entries in tab list */
extern struct _TabListData {
	uint16_t PlayerNames[TABLIST_MAX_NAMES];
	uint16_t ListNames[TABLIST_MAX_NAMES];
	uint16_t GroupNames[TABLIST_MAX_NAMES];
	uint8_t  GroupRanks[TABLIST_MAX_NAMES];
	StringsBuffer Buffer;
} TabList;

/* Returns whether the tab list entry with the given ID is used at all. */
bool TabList_Valid(EntityID id);
/* Removes the tab list entry with the given ID, raising TabListEvents.Removed event. */
void TabList_Remove(EntityID id);
void TabList_Set(EntityID id, const String* player, const String* list, const String* group, uint8_t rank);

#define TabList_UNSAFE_GetPlayer(id) StringsBuffer_UNSAFE_Get(&TabList.Buffer, TabList.PlayerNames[id]);
#define TabList_UNSAFE_GetList(id)   StringsBuffer_UNSAFE_Get(&TabList.Buffer, TabList.ListNames[id]);
#define TabList_UNSAFE_GetGroup(id)  StringsBuffer_UNSAFE_Get(&TabList.Buffer, TabList.GroupNames[id]);
#define Player_Layout struct Entity Base; char DisplayNameRaw[STRING_SIZE]; bool FetchedSkin; struct Texture NameTex;

/* Represents a player entity. */
struct Player { Player_Layout };
/* Sets the display name (name tag above entity) and skin name of the given player. */
void Player_SetName(struct Player* player, const String* name, const String* skin);
/* Remakes the texture for the name tag of the entity. */
void Player_UpdateNameTex(struct Player* player);
/* Resets the skin of the entity to default. */
void Player_ResetSkin(struct Player* player);

/* Represents another entity in multiplayer */
struct NetPlayer {
	Player_Layout
	struct NetInterpComp Interp;
	bool ShouldRender;
};
void NetPlayer_Init(struct NetPlayer* player, const String* displayName, const String* skinName);
extern struct NetPlayer NetPlayers_List[ENTITIES_SELF_ID];

/* Represents the user/player's own entity. */
struct LocalPlayer {
	Player_Layout
	Vector3 Spawn, OldVelocity;
	float SpawnRotY, SpawnHeadX, ReachDistance;
	struct HacksComp Hacks;
	struct TiltComp Tilt;
	struct InterpComp Interp;
	struct CollisionsComp Collisions;
	struct PhysicsComp Physics;
	bool _WarnedRespawn, _WarnedFly, _WarnedNoclip;
};

extern struct LocalPlayer LocalPlayer_Instance;
/* Returns how high (in blocks) the player can jump. */
float LocalPlayer_JumpHeight(void);
/* Checks consistency of user's enabled hacks. */
/* (e.g. if Hacks.CanNoclip is false, Hacks.Noclip is set to false) */
void LocalPlayer_CheckHacksConsistency(void);
/* Interpolates current position and orientation between Interp.Prev and Interp.Next */
void LocalPlayer_SetInterpPosition(float t);
/* Returns whether local player handles a key being pressed. */
/* e.g. for respawn, toggle fly, etc. */
bool LocalPlayer_HandlesKey(Key key);
#endif
