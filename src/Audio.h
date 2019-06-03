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
void Audio_PlayDigSound(uint8_t type);
void Audio_PlayStepSound(uint8_t type);

#define AUDIO_MAX_BUFFERS 4
/* Information about a source of audio. */
struct AudioFormat { uint16_t Channels, BitsPerSample; int SampleRate; };
#define AudioFormat_Eq(a, b) ((a)->Channels == (b)->Channels && (a)->BitsPerSample == (b)->BitsPerSample && (a)->SampleRate == (b)->SampleRate)
typedef int AudioHandle;

/* Acquires an audio context. */
void Audio_Open(AudioHandle* handle, int buffers);
/* Frees an allocated audio context. */
/* NOTE: Audio_StopAndClose should be used, because this method can fail if audio is playing. */
ReturnCode Audio_Close(AudioHandle handle);
/* Stops playing audio, unqueues buffers, then frees the audio context. */
ReturnCode Audio_StopAndClose(AudioHandle handle);
/* Returns the format audio is played in. */
struct AudioFormat* Audio_GetFormat(AudioHandle handle);
/* Sets the format audio to play is in. */
/* NOTE: Changing the format can be expensive, depending on the platform. */
ReturnCode Audio_SetFormat(AudioHandle handle, struct AudioFormat* format);
/* Sets the audio data in the given buffer. */
/* NOTE: You should ensure Audio_IsCompleted returns true before calling this. */
ReturnCode Audio_BufferData(AudioHandle handle, int idx, void* data, uint32_t dataSize);
/* Begins playing audio. Audio_BufferData must have been used before this. */
ReturnCode Audio_Play(AudioHandle handle);
/* Immediately stops the currently playing audio. */
ReturnCode Audio_Stop(AudioHandle handle);
/* Returns whether the given buffer has finished playing. */
ReturnCode Audio_IsCompleted(AudioHandle handle, int idx, bool* completed);
/* Returns whether all buffers have finished playing. */
ReturnCode Audio_IsFinished(AudioHandle handle, bool* finished);
#endif
