#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "GameStructs.h"
/* Manages playing sound and music.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

IGameComponent Audio_MakeComponent(void);
void Audio_SetMusic(Int32 volume);
void Audio_SetSounds(Int32 volume);
void Audio_PlayDigSound(UInt8 type);
void Audio_PlayStepSound(UInt8 type);
#endif