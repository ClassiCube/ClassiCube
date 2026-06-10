#ifndef CC_MOBS_H
#define CC_MOBS_H
#include "Core.h"
#include "Entity.h"
#include "EntityComponents.h"
CC_BEGIN_HEADER

/* Classic Survival Test mob system.
   Implements zombie, skeleton, spider, creeper, pig, sheep mobs.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Mobs_Component;

/* Mob type IDs */
#define MOB_TYPE_PIG      0
#define MOB_TYPE_SHEEP    1
#define MOB_TYPE_ZOMBIE   2
#define MOB_TYPE_SKELETON 3
#define MOB_TYPE_SPIDER   4
#define MOB_TYPE_CREEPER  5
#define MOB_TYPE_COUNT    6

/* Maximum number of mobs in the world at once */
#define MAX_MOBS 15
/* Entity IDs used by mobs (0 to MAX_MOBS-1) */
#define MOB_ENTITY_ID_START 0

/* Health per mob type */
#define MOB_HP_PIG      10
#define MOB_HP_SHEEP    10
#define MOB_HP_ZOMBIE   20
#define MOB_HP_SKELETON 20
#define MOB_HP_SPIDER   16
#define MOB_HP_CREEPER  20

/* Score per mob type (matching c0.30-s) */
#define MOB_SCORE_PIG      10
#define MOB_SCORE_SHEEP    10
#define MOB_SCORE_ZOMBIE   80
#define MOB_SCORE_SKELETON 120
#define MOB_SCORE_SPIDER   105
#define MOB_SCORE_CREEPER  200

/* A mob entity - extends Entity with AI and physics */
struct MobEntity {
    struct Entity      Base;       /* MUST be first field */
    struct PhysicsComp Physics;
    struct CollisionsComp Collisions;
    int  MobType;
    int  Health;
    int  MaxHealth;
    int  Score;
    cc_bool IsAlive;
    cc_bool IsHostile;
    float AttackTimer;  /* countdown until next attack */
    float AITimer;      /* general AI timer */
    float WalkX, WalkZ; /* current heading direction */
    int   EntityID;     /* index in Entities.List */
};

extern struct MobEntity MobEntities[MAX_MOBS];

/* Damages a mob, may kill it */
void Mob_Damage(struct MobEntity* mob, int amount);
/* Kills a mob, drops items and gives score */
void Mob_Kill(struct MobEntity* mob);
/* Spawn a mob of the given type at position */
void Mob_Spawn(int mobType, float x, float y, float z);
/* Remove all mobs from the world */
void Mobs_ClearAll(void);

CC_END_HEADER
#endif
