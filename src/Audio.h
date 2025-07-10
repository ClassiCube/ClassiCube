#ifndef CC_AUDIO_H
#define CC_AUDIO_H
#include "Core.h"
/* Manages playing sound and music.
   Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/
CC_BEGIN_HEADER

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

union AudioChunkMeta { void* ptr; cc_uintptr val; };
struct AudioChunk {
	void* data; /* the raw 16 bit integer samples */
	cc_uint32 size;
	union AudioChunkMeta meta;
};

struct AudioData {
	struct AudioChunk chunk;
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
extern const cc_string Sounds_ZipPathMC;
extern const cc_string Sounds_ZipPathCC;

void Audio_SetMusic(int volume);
void Audio_SetSounds(int volume);
void Audio_PlayDigSound(cc_uint8 type);
void Audio_PlayStepSound(cc_uint8 type);
#define AUDIO_MAX_BUFFERS 4

cc_bool AudioBackend_Init(void);
void    AudioBackend_Tick(void);
void    AudioBackend_Free(void);
void    AudioBackend_LoadSounds(void);

#define AUDIO_USAGE_BUFFER 0x01
#define AUDIO_USAGE_STREAM 0x02

/* Initialises an audio context. */
cc_result Audio_Init(struct AudioContext* ctx, int buffers);
/* Stops any playing audio and then frees the audio context. */
void Audio_Close(struct AudioContext* ctx);
/* Sets the volume audio data is played at */
void Audio_SetVolume(struct AudioContext* ctx, int volume);

/* Outputs more detailed information about errors with audio. */
cc_bool Audio_DescribeError(cc_result res, cc_string* dst);
/* Allocates a group of chunks of data to store audio samples */
cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks);
/* Frees a previously allocated group of chunks of data */
void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks);

extern struct AudioContext music_ctx;
void Audio_Warn(cc_result res, const char* action);

cc_result AudioPool_Play(struct AudioData* data);
void AudioPool_Close(void);


/*########################################################################################################################*
*------------------------------------------------------Stream context-----------------------------------------------------*
*#########################################################################################################################*/
/* Sets the format of the audio data to be played. */
/* NOTE: Changing the format can be expensive, depending on the backend. */
cc_result StreamContext_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate);
/* Queues the given audio chunk for playing. */
/* NOTE: You MUST ensure StreamContext_Update indicates a buffer is free before calling this. */
/* NOTE: Some backends directly read from the chunk data - therefore you MUST NOT modify it */
cc_result StreamContext_Enqueue(struct AudioContext* ctx, struct AudioChunk* chunk);
/* Begins playing audio. StreamContext_Enqueue must have been used before this. */
cc_result StreamContext_Play(struct AudioContext* ctx);
/* Temporarily pauses playing audio. Can be resumed again using StreamContext_Play. */
cc_result StreamContext_Pause(struct AudioContext* ctx);
/* Updates the audio context and then potentially unqueues buffer */
/* Returns the number of buffers in use (i.e. being played or queued to play) */
/*   (e.g. if inUse is 0, no audio buffers are being played or queued) */
cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse);


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
/* Whether the given audio data can be played without recreating the underlying audio device */
cc_bool   SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data);
/* Plays the given audio data */
cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data);
/* Polls the audio context and then potentially unqueues internal buffers */
/* Returns whether the audio context is currently playing audio */
cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy);


/*########################################################################################################################*
*---------------------------------------------------------Sounds---------------------------------------------------------*
*#########################################################################################################################*/
enum SoundType {
	SOUND_NONE,  SOUND_WOOD,  SOUND_GRAVEL, SOUND_GRASS, 
	SOUND_STONE, SOUND_METAL, SOUND_GLASS,  SOUND_CLOTH, 
	SOUND_SAND,  SOUND_SNOW,  SOUND_COUNT
};
extern const char* const Sound_Names[SOUND_COUNT];

#define AUDIO_MAX_SOUNDS 10
struct Sound {
	int channels, sampleRate;
	struct AudioChunk chunk;
};
struct SoundGroup {
	int count;
	struct Sound sounds[AUDIO_MAX_SOUNDS];
};
struct Soundboard { struct SoundGroup groups[SOUND_COUNT]; };

extern struct Soundboard digBoard, stepBoard;
void Sounds_LoadDefault(void);

CC_END_HEADER
#endif
