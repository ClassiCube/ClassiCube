#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_OPENAL
/* Simpler to just include subset of OpenAL actually use here instead of including */

/* === BEGIN OPENAL HEADERS === */
#if defined CC_BUILD_WIN
	#define APIENTRY __cdecl
#else
	#define APIENTRY
#endif

#define AL_NONE              0
#define AL_GAIN              0x100A
#define AL_SOURCE_STATE      0x1010
#define AL_PLAYING           0x1012
#define AL_BUFFERS_QUEUED    0x1015
#define AL_BUFFERS_PROCESSED 0x1016
#define AL_FORMAT_MONO16     0x1101
#define AL_FORMAT_STEREO16   0x1103

#define AL_INVALID_NAME      0xA001
#define AL_INVALID_ENUM      0xA002
#define AL_INVALID_VALUE     0xA003
#define AL_INVALID_OPERATION 0xA004
#define AL_OUT_OF_MEMORY     0xA005

typedef char ALboolean;
typedef int ALint;
typedef unsigned int ALuint;
typedef int ALsizei;
typedef int ALenum;

/* Apologies for the ugly dynamic symbol definitions here */
static ALenum (APIENTRY *_alGetError)(void);
static void   (APIENTRY *_alGenSources)(ALsizei n, ALuint* sources);
static void   (APIENTRY *_alDeleteSources)(ALsizei n, const ALuint* sources);
static void   (APIENTRY *_alGetSourcei)(ALuint source, ALenum param, ALint* value);
static void   (APIENTRY *_alSourcef)(ALuint source, ALenum param, float value);
static void   (APIENTRY *_alSourcePlay) (ALuint source);
static void   (APIENTRY *_alSourcePause)(ALuint source);
static void   (APIENTRY *_alSourceStop) (ALuint source);
static void   (APIENTRY *_alSourceQueueBuffers)(ALuint source, ALsizei nb, const ALuint* buffers);
static void   (APIENTRY *_alSourceUnqueueBuffers)(ALuint source, ALsizei nb, ALuint* buffers);
static void   (APIENTRY *_alGenBuffers)(ALsizei n, ALuint* buffers);
static void   (APIENTRY *_alDeleteBuffers)(ALsizei n, const ALuint* buffers);
static void   (APIENTRY *_alBufferData)(ALuint buffer, ALenum format, const void* data, ALsizei size, ALsizei freq);

static void   (APIENTRY *_alDistanceModel)(ALenum distanceModel);
static void*     (APIENTRY *_alcCreateContext)(void* device, const ALint* attrlist);
static ALboolean (APIENTRY *_alcMakeContextCurrent)(void* context);
static void      (APIENTRY *_alcDestroyContext)(void* context);
static void*     (APIENTRY *_alcOpenDevice)(const char* devicename);
static ALboolean (APIENTRY *_alcCloseDevice)(void* device);
static ALenum    (APIENTRY *_alcGetError)(void* device);
/* === END OPENAL HEADERS === */

#include "Audio.h"
struct AudioContext {
	ALuint source;
	ALuint buffers[AUDIO_MAX_BUFFERS];
	ALuint freeIDs[AUDIO_MAX_BUFFERS];
	int count, free, sampleRate;
	ALenum format;
};

#define AUDIO_COMMON_ALLOC
#include "_AudioBase.h"
#include "Funcs.h"

static void* audio_device;
static void* audio_context;

#if defined CC_BUILD_WIN
static const cc_string alLib = String_FromConst("openal32.dll");
#elif defined CC_BUILD_MACOS
static const cc_string alLib = String_FromConst("/System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL");
#elif defined CC_BUILD_IOS
static const cc_string alLib = String_FromConst("/System/Library/Frameworks/OpenAL.framework/OpenAL");
#elif defined CC_BUILD_NETBSD
static const cc_string alLib = String_FromConst("/usr/pkg/lib/libopenal.so");
#elif defined CC_BUILD_BSD
static const cc_string alLib = String_FromConst("libopenal.so");
#else
static const cc_string alLib = String_FromConst("libopenal.so.1");
#endif

static cc_bool LoadALFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym(alcCreateContext),  DynamicLib_ReqSym(alcMakeContextCurrent),
		DynamicLib_ReqSym(alcDestroyContext), DynamicLib_ReqSym(alcOpenDevice),
		DynamicLib_ReqSym(alcCloseDevice),    DynamicLib_ReqSym(alcGetError),

		DynamicLib_ReqSym(alGetError),
		DynamicLib_ReqSym(alGenSources),      DynamicLib_ReqSym(alDeleteSources),
		DynamicLib_ReqSym(alGetSourcei),      DynamicLib_ReqSym(alSourcef),
		DynamicLib_ReqSym(alSourcePlay),      DynamicLib_ReqSym(alSourceStop),
		DynamicLib_ReqSym(alSourceQueueBuffers), DynamicLib_ReqSym(alSourceUnqueueBuffers),
		DynamicLib_ReqSym(alGenBuffers),      DynamicLib_ReqSym(alDeleteBuffers),
		DynamicLib_ReqSym(alBufferData),      DynamicLib_ReqSym(alDistanceModel),
		DynamicLib_OptSym(alSourcePlay)
	};
	void* lib;
	
	return DynamicLib_LoadAll(&alLib, funcs, Array_Elems(funcs), &lib);
}

static cc_result CreateALContext(void) {
	ALenum err;
	audio_device = _alcOpenDevice(NULL);
	if ((err = _alcGetError(audio_device))) return err;
	if (!audio_device)  return AL_ERR_INIT_DEVICE;

	audio_context = _alcCreateContext(audio_device, NULL);
	if ((err = _alcGetError(audio_device))) return err;
	if (!audio_context) return AL_ERR_INIT_CONTEXT;

	_alcMakeContextCurrent(audio_context);
	return _alcGetError(audio_device);
}

cc_bool AudioBackend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init OpenAL. No audio will play.");
	cc_result res;
	if (audio_device) return true;
	if (!LoadALFuncs()) { Logger_WarnFunc(&msg); return false; }

	res = CreateALContext();
	if (res) { Audio_Warn(res, "initing OpenAL"); return false; }
	return true;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) {
	if (!audio_device) return;
	_alcMakeContextCurrent(NULL);

	if (audio_context) _alcDestroyContext(audio_context);
	if (audio_device)  _alcCloseDevice(audio_device);

	audio_context = NULL;
	audio_device  = NULL;
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ALenum i, err;
	_alDistanceModel(AL_NONE);
	ctx->source = 0;
	ctx->count  = buffers;
	
	_alGetError(); /* Reset error state */
	_alGenSources(1, &ctx->source);
	if ((err = _alGetError())) return err;

	_alGenBuffers(buffers, ctx->buffers);
	if ((err = _alGetError())) return err;

	for (i = 0; i < buffers; i++) {
		ctx->freeIDs[i] = ctx->buffers[i];
	}
	ctx->free = buffers;
	return 0;
}

static void Audio_Stop(struct AudioContext* ctx) {
	_alSourceStop(ctx->source);
}

static void Audio_Reset(struct AudioContext* ctx) {
	_alDeleteSources(1,          &ctx->source);
	_alDeleteBuffers(ctx->count, ctx->buffers);
	ctx->source = 0;
}

static void ClearFree(struct AudioContext* ctx) {
	int i;
	for (i = 0; i < AUDIO_MAX_BUFFERS; i++) {
		ctx->freeIDs[i] = 0;
	}
	ctx->free = 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->source) {
		Audio_Stop(ctx);
		Audio_Reset(ctx);
		_alGetError(); /* Reset error state */
	}
	ClearFree(ctx);
	ctx->count = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	ctx->sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);

	if (channels == 1) {
		ctx->format = AL_FORMAT_MONO16;
	} else if (channels == 2) {
		ctx->format = AL_FORMAT_STEREO16;
	} else {
		return ERR_INVALID_ARGUMENT;
	}
	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	_alSourcef(ctx->source, AL_GAIN, volume / 100.0f);
	_alGetError(); /* Reset error state */
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	ALuint buffer;
	ALenum err;
	
	if (!ctx->free) return ERR_INVALID_ARGUMENT;
	buffer = ctx->freeIDs[--ctx->free];
	_alGetError(); /* Reset error state */

	_alBufferData(buffer, ctx->format, chunk->data, chunk->size, ctx->sampleRate);
	if ((err = _alGetError())) return err;
	_alSourceQueueBuffers(ctx->source, 1, &buffer);
	if ((err = _alGetError())) return err;
	return 0;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	ALint processed = 0;
	ALuint buffer;
	ALenum err;

	*inUse = 0;
	if (!ctx->source) return 0;

	_alGetError(); /* Reset error state */
	_alGetSourcei(ctx->source, AL_BUFFERS_PROCESSED, &processed);
	if ((err = _alGetError())) return err;

	if (processed > 0) {
		_alSourceUnqueueBuffers(ctx->source, 1, &buffer);
		if ((err = _alGetError())) return err;

		ctx->freeIDs[ctx->free++] = buffer;
	}
	*inUse = ctx->count - ctx->free; return 0;
}


/*########################################################################################################################*
*------------------------------------------------------Stream context-----------------------------------------------------*
*#########################################################################################################################*/
cc_result StreamContext_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	return Audio_SetFormat(ctx, channels, sampleRate, playbackRate);
}

cc_result StreamContext_Enqueue(struct AudioContext* ctx, struct AudioChunk* chunk) {
	return Audio_QueueChunk(ctx, chunk); 
}

cc_result StreamContext_Play(struct AudioContext* ctx) {
	_alSourcePlay(ctx->source);
	return _alGetError();
}

cc_result StreamContext_Pause(struct AudioContext* ctx) {
	if (!_alSourcePause) return ERR_NOT_SUPPORTED;

	_alSourcePause(ctx->source);
	return _alGetError();
}

cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
	return Audio_Poll(ctx, inUse);
}


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	/* Channels/Sample rate is per buffer, not a per source property */
	return true;
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    cc_result res;

	if ((res = Audio_SetFormat(ctx,  data->channels, data->sampleRate, data->rate))) return res;
	if ((res = Audio_QueueChunk(ctx, &data->chunk))) return res;
	_alSourcePlay(ctx->source);

	return _alGetError();
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
	int inUse = 1;
	cc_result res;
	if ((res = Audio_Poll(ctx, &inUse))) return res;

	*isBusy = inUse > 0;
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Audio misc-------------------------------------------------------*
*#########################################################################################################################*/
static const char* GetError(cc_result res) {
	switch (res) {
	case AL_ERR_INIT_CONTEXT:  return "Failed to init OpenAL context";
	case AL_ERR_INIT_DEVICE:   return "Failed to init OpenAL device";
	case AL_INVALID_NAME:      return "Invalid parameter name";
	case AL_INVALID_ENUM:      return "Invalid parameter";
	case AL_INVALID_VALUE:     return "Invalid parameter value";
	case AL_INVALID_OPERATION: return "Invalid operation";
	case AL_OUT_OF_MEMORY:     return "OpenAL out of memory";
	}
	return NULL;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	const char* err = GetError(res);
	if (err) String_AppendConst(dst, err);
	return err != NULL;
}
#endif

