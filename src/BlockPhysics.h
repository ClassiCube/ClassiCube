#ifndef CC_BLOCKPHYSICS_H
#define CC_BLOCKPHYSICS_H
#include "Core.h"
/* Implements simple block physics.
   Copyright 2014 - 2017 ClassicalSharp | Licensed under BSD-3
*/

extern bool Physics_Enabled;
void Physics_SetEnabled(bool enabled);
void Physics_Init(void);
void Physics_Free(void);
void Physics_Tick(void);
#endif
