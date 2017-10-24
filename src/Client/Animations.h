#ifndef CC_ANIMS_H
#define CC_ANIMS_H
#include "Typedefs.h"
#include "String.h"
#include "Stream.h"
/* Water and lava liquid animations.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
   Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
   mirrored at https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-lava-animation-algorithm
   Water animation originally written by cybertoon, big thanks!
*/

#define LIQUID_ANIM_MAX 64

void LavaAnimation_Tick(UInt32* ptr, Int32 size);
void WaterAnimation_Tick(UInt32* ptr, Int32 size);
#endif