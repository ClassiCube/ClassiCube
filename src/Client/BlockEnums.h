#ifndef CS_BLOCKENUMS_H
#define CS_BLOCKENUMS_H
#include "Typedefs.h"
/* Block related enumerations.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


/* Sides of a block. TODO: Map this to CPE PlayerClicked blockface enums. */
typedef UInt8 Face;
/* Face X = 0. */
#define Face_XMin 0
/* Face X = 1. */
#define Face_XMax 1
/* Face Z = 0. */
#define Face_ZMin 2
/* Face Z = 1. */
#define Face_ZMax 3
/* Face Y = 0. */
#define Face_YMin 4
/* Face Y = 1. */
#define Face_YMax 5
/* Number of faces on a cube. */
#define Face_Count 6


/* Sound types for blocks. */
typedef UInt8 SoundType;
#define SoundType_None 0
#define SoundType_Wood 1
#define SoundType_Gravel 2
#define SoundType_Grass 3
#define SoundType_Stone 4
#define SoundType_Metal 5
#define SoundType_Glass 6
#define SoundType_Cloth 7
#define SoundType_Sand 8
#define SoundType_Snow 9


/* Describes how a block is rendered in the world. */
typedef UInt8 DrawType;
/* Completely covers blocks behind (e.g. dirt). */
#define DrawType_Opaque 0
/*  Blocks behind show (e.g. glass). Pixels are either fully visible or invisible.  */
#define DrawType_Transparent 1
/*  Same as Transparent, but all neighbour faces show. (e.g. leaves) */
#define DrawType_TransparentThick 2
/* Blocks behind show (e.g. water). Pixels blend with other blocks behind. */
#define DrawType_Translucent 3
/* Does not show (e.g. air). Can still be collided with. */
#define DrawType_Gas 4
/* Block renders as an X sprite (e.g. sapling). Pixels are either fully visible or invisible. */
#define DrawType_Sprite 5


/* Describes the interaction a block has with a player when they collide with it. */
typedef UInt8 CollideType;
/* No interaction when player collides. */
#define CollideType_Gas 0
/* 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_Liquid 1
/* Block completely stops the player when they are moving. */
#define CollideType_Solid 2
/* Block is solid and partially slidable on. */
#define CollideType_Ice 3
/* Block is solid and fully slidable on. */
#define CollideType_SlipperyIce 4
/* Water style 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_LiquidWater 5
/* Lava style 'swimming'/'bobbing' interaction when player collides. */
#define CollideType_LiquidLava 6

#endif