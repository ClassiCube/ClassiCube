#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Core.h"
/* Manages playing sound and music.
   Copyright 2014-2021 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Audio_Component; 
struct AudioContext;

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

/* Initialises an audio context. */
void Audio_Init(struct AudioContext* ctx, int buffers);
/* Stops playing audio, unqueues buffers, then frees the audio context. */
cc_result Audio_Close(struct AudioContext* ctx);
/* Returns the format audio is played in. */
struct AudioFormat* Audio_GetFormat(struct AudioContext* ctx);
/* Sets the format of the audio data to be played. */
/* NOTE: Changing the format can be expensive, depending on the backend. */
cc_result Audio_SetFormat(struct AudioContext* ctx, struct AudioFormat* format);
/* Sets the audio data in the given buffer. */
/* NOTE: You MUST ensure Audio_IsCompleted returns true before calling this. */
cc_result Audio_BufferData(struct AudioContext* ctx, int idx, void* data, cc_uint32 size);
/* Begins playing audio. Audio_BufferData must have been used before this. */
cc_result Audio_Play(struct AudioContext* ctx);
/* Immediately stops the currently playing audio. */
cc_result Audio_Stop(struct AudioContext* ctx);
/* Returns whether the given buffer is available for playing data. */
cc_result Audio_IsAvailable(struct AudioContext* ctx, int idx, cc_bool* available);
/* Returns whether all buffers have finished playing. */
cc_result Audio_IsFinished(struct AudioContext* ctx, cc_bool* finished);
#endif
