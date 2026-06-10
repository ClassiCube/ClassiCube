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
/* Whether the player is currently dead (waiting to respawn) */
extern cc_bool Survival_Dead;

#define SURVIVAL_MAX_HEALTH 20
#define SURVIVAL_MAX_AIR    300

/* Deals damage to the player, respecting invincibility frames */
void Survival_Damage(int amount);
/* Kills the player immediately */
void Survival_Die(void);
/* Respawns the player at spawn with full health */
void Survival_Respawn(void);
/* Adds points to the player's score */
void Survival_AddScore(int points);
/* Updates the HUD health/air display */
void Survival_UpdateHUD(void);

CC_END_HEADER
#endif
