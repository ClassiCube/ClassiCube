#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Core.h"
/* Manages playing sound and music.
   Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/
struct IGameComponent;
extern struct IGameComponent Audio_Component; 
struct AudioContext;

#ifdef CC_BUILD_WEBAUDIO
#define DEFAULT_SOUNDS_VOLUME   0
#else
#define DEFAULT_SOUNDS_VOLUME 100
#endif

#ifdef CC_BUILD_NOMUSIC
#define DEFAULT_MUSIC_VOLUME   0
#else
#define DEFAULT_MUSIC_VOLUME 100
#endif

struct AudioData {
	void* data; cc_uint32 size; /* the raw 16 bit integer samples */
	int channels;
	int sampleRate; /* frequency / sample rate */
	int volume; /* volume data played at (100 = normal volume) */
	int rate;   /* speed/pitch played at (100 = normal speed) */
};

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

/* Initialises an audio context. */
void Audio_Init(struct AudioContext* ctx, int buffers);
/* Stops any playing audio and then frees the audio context. */
void Audio_Close(struct AudioContext* ctx);
/* Sets the format of the audio data to be played. */
/* NOTE: Changing the format can be expensive, depending on the backend. */
cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate);
/* Queues the given audio data for playing. */
/* NOTE: You MUST ensure Audio_Poll indicates a buffer is free before calling this. */
/* NOTE: Some backends directly read from the data - therefore you MUST NOT modify it */
cc_result Audio_QueueData(struct AudioContext* ctx, void* data, cc_uint32 size);
/* Begins playing audio. Audio_QueueData must have been used before this. */
cc_result Audio_Play(struct AudioContext* ctx);
/* Polls the audio context and then potentially unqueues buffer */
/* Returns the number of buffers being played or queued */
/* (e.g. if inUse is 0, no audio buffers are being played or queued) */
cc_result Audio_Poll(struct AudioContext* ctx, int* inUse);

/* Plays the given audio data */
cc_result Audio_PlayData(struct AudioContext* ctx, struct AudioData* data);
/* Whether the given audio data can be played without recreating the underlying audio device */
cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data);
/* Outputs more detailed information about errors with audio. */
cc_bool Audio_DescribeError(cc_result res, cc_string* dst);
#endif
