#ifndef CC_SURVIVAL_H
#define CC_SURVIVAL_H
#include "Core.h"
CC_BEGIN_HEADER

/* Classic Survival Test (c0.30-s) mechanics.
   Implements health, fall/lava/drowning damage, block drops, score.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Survival_Component;

/* Whether survival mode is active (singleplayer only) */
extern cc_bool Survival_Active;
/* Current health: 0-20, where 20 = 10 full hearts */
extern int Survival_Health;
/* Current air supply: 0-300, 300 = full (15 seconds) */
extern int Survival_Air;
/* Total score accumulated this session */
extern int Survival_Score;
/* Whether the player has died (Game Over - no respawn in c0.30-s) */
extern cc_bool Survival_Dead;
/* Player's arrow supply (starts at 20, fired with Tab key) */
extern int Survival_Arrows;
/* Player's TNT supply (starts at 10, placed then breaks to explode) */
extern int Survival_TNT;
/* Set true while a creeper/TNT explosion is destroying blocks so that
   the block-drop handler doesn't award items for explosion-cleared blocks */
extern cc_bool Survival_InExplosion;

#define SURVIVAL_MAX_HEALTH   20
#define SURVIVAL_MAX_AIR      300
#define SURVIVAL_START_ARROWS 20
#define SURVIVAL_START_TNT    10
/* TNT/creeper explosion parameters (same as original c0.30-s) */
#define EXPLOSION_RADIUS      4
#define EXPLOSION_MAX_DMG     12

/* Deals damage to the player, respecting invincibility frames */
void Survival_Damage(int amount);
/* Kills the player immediately (Game Over) */
void Survival_Die(void);
/* Adds points to the player's score (no-op during explosions = indirect kill) */
void Survival_AddScore(int points);
/* Updates the HUD health/air/score/arrows display */
void Survival_UpdateHUD(void);
/* Destroys blocks in a sphere and damages nearby player -- used by creeper and TNT */
void Survival_Explode(float cx, float cy, float cz, int radius, int maxDmg);

CC_END_HEADER
#endif
