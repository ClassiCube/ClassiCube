#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Core.h"
/* Manages playing sound and music.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Audio_Component; 

/* Volume sounds are played at, from 0-100. */
/* NOTE: Use Audio_SetSounds, don't change this directly. */
extern int Audio_SoundsVolume;
/* Volume music is played at, from 0-100. */
/* NOTE: Use Audio_SetMusic, don't change this directly. */
extern int Audio_MusicVolume;

void Audio_SetMusic(int volume);
void Audio_SetSounds(int volume);
void Audio_PlayDigSound(cc_uint8 type);
void Audio_PlayStepSound(cc_uint8 type);

#define AUDIO_MAX_BUFFERS 4
/* Information about a source of audio. */
struct AudioFormat { cc_uint16 channels; int sampleRate; };
#define AudioFormat_Eq(a, b) ((a)->channels == (b)->channels && (a)->sampleRate == (b)->sampleRate)
typedef int AudioHandle;

/* Acquires an audio context. */
void Audio_Open(AudioHandle* handle, int buffers);
/* Stops playing audio, unqueues buffers, then frees the audio context. */
cc_result Audio_Close(AudioHandle handle);
/* Returns the format audio is played in. */
struct AudioFormat* Audio_GetFormat(AudioHandle handle);
/* Sets the format audio to play is in. */
/* NOTE: Changing the format can be expensive, depending on the platform. */
cc_result Audio_SetFormat(AudioHandle handle, struct AudioFormat* format);
/* Sets the audio data in the given buffer. */
/* NOTE: You should ensure Audio_IsCompleted returns true before calling this. */
cc_result Audio_BufferData(AudioHandle handle, int idx, void* data, cc_uint32 dataSize);
/* Begins playing audio. Audio_BufferData must have been used before this. */
cc_result Audio_Play(AudioHandle handle);
/* Immediately stops the currently playing audio. */
cc_result Audio_Stop(AudioHandle handle);
/* Returns whether the given buffer has finished playing. */
cc_result Audio_IsCompleted(AudioHandle handle, int idx, cc_bool* completed);
/* Returns whether all buffers have finished playing. */
cc_result Audio_IsFinished(AudioHandle handle, cc_bool* finished);
#endif
