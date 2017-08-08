#if 0
#ifndef CS_LIQUIDANIMS_H
#define CS_LIQUIDANIMS_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
/* Water and lava liquid animations.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
   Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
   mirrored at https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-lava-animation-algorithm

   Water animation originally written by cybertoon, big thanks!
*/


#define LiquidAnim_Max 64

/* Animates lava */
void LavaAnimation_Tick(UInt32* ptr, Int32 size);

/* Animates water */
void WaterAnimation_Tick(UInt32* ptr, Int32 size);
#endif
#endif