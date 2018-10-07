#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Core.h"
/* Manages playing sound and music.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/
struct IGameComponent;

void Audio_MakeComponent(struct IGameComponent* comp);
void Audio_SetMusic(int volume);
void Audio_SetSounds(int volume);
void Audio_PlayDigSound(uint8_t type);
void Audio_PlayStepSound(uint8_t type);
#endif
