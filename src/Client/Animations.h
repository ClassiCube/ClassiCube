#ifndef CC_ANIMS_H
#define CC_ANIMS_H
#include "GameStructs.h"
/* Texture animations, and water and lava liquid animations.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
   Based off the incredible work from https://dl.dropboxusercontent.com/u/12694594/lava.txt
   mirrored at https://github.com/UnknownShadow200/ClassicalSharp/wiki/Minecraft-Classic-lava-animation-algorithm
   Water animation originally written by cybertoon, big thanks!
*/

IGameComponent Animations_MakeComponent(void);
void Animations_Tick(ScheduledTask* task);
#endif