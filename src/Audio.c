#include "Audio.h"
#include "String.h"
#include "Logger.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Errors.h"
#include "Vorbis.h"
#include "Chat.h"
#include "Stream.h"
#include "Utils.h"
#include "Options.h"
#ifdef CC_BUILD_ANDROID
/* TODO: Refactor maybe to not rely on checking WinInfo.Handle != NULL */
#include "Window.h"
#endif
int Audio_SoundsVolume, Audio_MusicVolume;
static const cc_string audio_dir = String_FromConst("audio");

struct Sound {
	int channels, sampleRate;
	void* data; cc_uint32 size;
};

static void ApplyVolume(cc_int16* samples, int count, int volume) {
	int i;

	for (i = 0; i < (count & ~0x07); i += 8, samples += 8) {
		samples[0] = (samples[0] * volume / 100);
		samples[1] = (samples[1] * volume / 100);
		samples[2] = (samples[2] * volume / 100);
		samples[3] = (samples[3] * volume / 100);

		samples[4] = (samples[4] * volume / 100);
		samples[5] = (samples[5] * volume / 100);
		samples[6] = (samples[6] * volume / 100);
		samples[7] = (samples[7] * volume / 100);
	}

	for (; i < count; i++, samples++) {
		samples[0] = (samples[0] * volume / 100);
	}
}

/* Common/Base methods */
static void AudioWarn(cc_result res, const char* action) {
	Logger_Warn(res, action, Audio_DescribeError);
}
static void AudioBase_Clear(struct AudioContext* ctx);
static cc_bool AudioBase_AdjustSound(struct AudioContext* ctx, struct AudioData* data);
/* achieve higher speed by playing samples at higher sample rate */
#define Audio_AdjustSampleRate(data) ((data->sampleRate * data->rate) / 100)

#if defined CC_BUILD_OPENAL
/*########################################################################################################################*
*------------------------------------------------------OpenAL backend-----------------------------------------------------*
*#########################################################################################################################*/
/* Simpler to just include subset of OpenAL actually use here instead of including */
/* === BEGIN OPENAL HEADERS === */
#if defined _WIN32
#define APIENTRY __cdecl
#else
#define APIENTRY
#endif
#define AL_NONE              0
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
static void   (APIENTRY *_alSourcePlay)(ALuint source); 
static void   (APIENTRY *_alSourceStop)(ALuint source);
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

struct AudioContext {
	ALuint source;
	ALuint buffers[AUDIO_MAX_BUFFERS];
	ALuint freeIDs[AUDIO_MAX_BUFFERS];
	int count, free, sampleRate;
	ALenum channels;
	cc_uint32 _tmpSize; void* _tmpData;
};
static void* audio_device;
static void* audio_context;
#define AUDIO_HAS_BACKEND

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
		DynamicLib_Sym(alcCreateContext),  DynamicLib_Sym(alcMakeContextCurrent),
		DynamicLib_Sym(alcDestroyContext), DynamicLib_Sym(alcOpenDevice),
		DynamicLib_Sym(alcCloseDevice),    DynamicLib_Sym(alcGetError),

		DynamicLib_Sym(alGetError),        DynamicLib_Sym(alGenSources),
		DynamicLib_Sym(alDeleteSources),   DynamicLib_Sym(alGetSourcei),
		DynamicLib_Sym(alSourcePlay),      DynamicLib_Sym(alSourceStop),
		DynamicLib_Sym(alSourceQueueBuffers), DynamicLib_Sym(alSourceUnqueueBuffers),
		DynamicLib_Sym(alGenBuffers),      DynamicLib_Sym(alDeleteBuffers),
		DynamicLib_Sym(alBufferData),      DynamicLib_Sym(alDistanceModel)
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

static cc_bool AudioBackend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init OpenAL. No audio will play.");
	cc_result res;
	if (audio_device) return true;
	if (!LoadALFuncs()) { Logger_WarnFunc(&msg); return false; }

	res = CreateALContext();
	if (res) { AudioWarn(res, "initing OpenAL"); return false; }
	return true;
}

static void AudioBackend_Free(void) {
	if (!audio_device) return;
	_alcMakeContextCurrent(NULL);

	if (audio_context) _alcDestroyContext(audio_context);
	if (audio_device)  _alcCloseDevice(audio_device);

	audio_context = NULL;
	audio_device  = NULL;
}

static void ClearFree(struct AudioContext* ctx) {
	int i;
	for (i = 0; i < AUDIO_MAX_BUFFERS; i++) {
		ctx->freeIDs[i] = 0;
	}
	ctx->free = 0;
}

void Audio_Init(struct AudioContext* ctx, int buffers) {
	_alDistanceModel(AL_NONE);
	ClearFree(ctx);

	ctx->source = 0;
	ctx->count  = buffers;
}

static void Audio_Stop(struct AudioContext* ctx) {
	_alSourceStop(ctx->source);
}

static void Audio_Reset(struct AudioContext* ctx) {
	_alDeleteSources(1,          &ctx->source);
	_alDeleteBuffers(ctx->count, ctx->buffers);
	ctx->source = 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->source) {
		Audio_Stop(ctx);
		Audio_Reset(ctx);
		_alGetError();
	}
	ClearFree(ctx);
	AudioBase_Clear(ctx);
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate) {
	ALenum i, err;
	if (!ctx->source) {
		_alGenSources(1, &ctx->source);
		if ((err = _alGetError())) return err;

		_alGenBuffers(ctx->count, ctx->buffers);
		if ((err = _alGetError())) return err;

		for (i = 0; i < ctx->count; i++) {
			ctx->freeIDs[i] = ctx->buffers[i];
		}
		ctx->free = ctx->count;
	}
	ctx->sampleRate = sampleRate;

	if (channels == 1) {
		ctx->channels = AL_FORMAT_MONO16;
	} else if (channels == 2) {
		ctx->channels = AL_FORMAT_STEREO16;
	} else {
		return ERR_INVALID_ARGUMENT;
	}
	return 0;
}

cc_result Audio_QueueData(struct AudioContext* ctx, void* data, cc_uint32 size) {
	ALuint buffer;
	ALenum err;
	
	if (!ctx->free) return ERR_INVALID_ARGUMENT;
	buffer = ctx->freeIDs[--ctx->free];

	_alBufferData(buffer, ctx->channels, data, size, ctx->sampleRate);
	if ((err = _alGetError())) return err;
	_alSourceQueueBuffers(ctx->source, 1, &buffer);
	if ((err = _alGetError())) return err;
	return 0;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	_alSourcePlay(ctx->source);
	return _alGetError();
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	ALint processed = 0;
	ALuint buffer;
	ALenum err;

	*inUse = 0;
	if (!ctx->source) return 0;

	_alGetSourcei(ctx->source, AL_BUFFERS_PROCESSED, &processed);
	if ((err = _alGetError())) return err;

	if (processed > 0) {
		_alSourceUnqueueBuffers(ctx->source, 1, &buffer);
		if ((err = _alGetError())) return err;

		ctx->freeIDs[ctx->free++] = buffer;
	}
	*inUse = ctx->count - ctx->free; return 0;
}

cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	/* Channels/Sample rate is per buffer, not a per source property */
	return true;
}

cc_result Audio_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	cc_bool ok = AudioBase_AdjustSound(ctx, data);
	cc_result res;
	if (!ok) return ERR_OUT_OF_MEMORY;	
	data->sampleRate = Audio_AdjustSampleRate(data);

	if ((res = Audio_SetFormat(ctx, data->channels, data->sampleRate))) return res;
	if ((res = Audio_QueueData(ctx, data->data,     data->size)))       return res;
	if ((res = Audio_Play(ctx))) return res;
	return 0;
}

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
#elif defined CC_BUILD_WINMM
/*########################################################################################################################*
*------------------------------------------------------WinMM backend------------------------------------------------------*
*#########################################################################################################################*/
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>

/* === BEGIN mmsyscom.h === */
#define CALLBACK_NULL  0x00000000l
typedef UINT           MMRESULT;
#define WINMMAPI       DECLSPEC_IMPORT
#define MMSYSERR_BADDEVICEID 2
/* === BEGIN mmeapi.h === */
typedef struct WAVEHDR_ {
	LPSTR       lpData;
	DWORD       dwBufferLength;
	DWORD       dwBytesRecorded;
	DWORD_PTR   dwUser;
	DWORD       dwFlags;
	DWORD       dwLoops;
	struct WAVEHDR_* lpNext;
	DWORD_PTR   reserved;
} WAVEHDR;

typedef struct WAVEFORMATEX_ {
	WORD  wFormatTag;
	WORD  nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD  nBlockAlign;
	WORD  wBitsPerSample;
	WORD  cbSize;
} WAVEFORMATEX;
typedef void* HWAVEOUT;

#define WAVE_MAPPER     ((UINT)-1)
#define WAVE_FORMAT_PCM 1
#define WHDR_DONE       0x00000001
#define WHDR_PREPARED   0x00000002

WINMMAPI MMRESULT WINAPI waveOutOpen(HWAVEOUT* phwo, UINT deviceID, const WAVEFORMATEX* fmt, DWORD_PTR callback, DWORD_PTR instance, DWORD flags);
WINMMAPI MMRESULT WINAPI waveOutClose(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, WAVEHDR* hdr, UINT hdrSize);
WINMMAPI MMRESULT WINAPI waveOutReset(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutGetErrorTextA(MMRESULT err, LPSTR text, UINT textLen);
WINMMAPI UINT     WINAPI waveOutGetNumDevs(void);
/* === END mmeapi.h === */

struct AudioContext {
	HWAVEOUT handle;
	WAVEHDR headers[AUDIO_MAX_BUFFERS];
	int count, channels, sampleRate;
	cc_uint32 _tmpSize; void* _tmpData;
};
static cc_bool AudioBackend_Init(void) { return true; }
static void AudioBackend_Free(void) { }
#define AUDIO_HAS_BACKEND

void Audio_Init(struct AudioContext* ctx, int buffers) {
	int i;
	for (i = 0; i < buffers; i++) {
		ctx->headers[i].dwFlags = WHDR_DONE;
	}
	ctx->count = buffers;
}

static void Audio_Stop(struct AudioContext* ctx) {
	waveOutReset(ctx->handle);
}

static cc_result Audio_Reset(struct AudioContext* ctx) {
	cc_result res;
	if (!ctx->handle) return 0;

	res = waveOutClose(ctx->handle);
	ctx->handle = NULL;
	return res;
}

void Audio_Close(struct AudioContext* ctx) {
	int inUse;
	if (ctx->handle) {
		Audio_Stop(ctx);
		Audio_Poll(ctx, &inUse); /* unprepare buffers */
		Audio_Reset(ctx);
	}
	AudioBase_Clear(ctx);
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate) {
	WAVEFORMATEX fmt;
	cc_result res;
	int sampleSize;

	if (ctx->channels == channels && ctx->sampleRate == sampleRate) return 0;
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;
	
	sampleSize = channels * 2; /* 16 bits per sample / 8 */
	if ((res = Audio_Reset(ctx))) return res;

	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = channels;
	fmt.nSamplesPerSec  = sampleRate;
	fmt.nAvgBytesPerSec = sampleRate * sampleSize;
	fmt.nBlockAlign     = sampleSize;
	fmt.wBitsPerSample  = 16;
	fmt.cbSize          = 0;
	
	res = waveOutOpen(&ctx->handle, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
	/* Show a better error message when no audio output devices connected than */
	/*  "A device ID has been used that is out of range for your system" */
	if (res == MMSYSERR_BADDEVICEID && waveOutGetNumDevs() == 0)
		return ERR_NO_AUDIO_OUTPUT;
	return res;
}

cc_result Audio_QueueData(struct AudioContext* ctx, void* data, cc_uint32 dataSize) {
	cc_result res = 0;
	WAVEHDR* hdr;
	int i;

	for (i = 0; i < ctx->count; i++) {
		hdr = &ctx->headers[i];
		if (!(hdr->dwFlags & WHDR_DONE)) continue;

		Mem_Set(hdr, 0, sizeof(WAVEHDR));
		hdr->lpData         = (LPSTR)data;
		hdr->dwBufferLength = dataSize;
		hdr->dwLoops        = 1;
		
		if ((res = waveOutPrepareHeader(ctx->handle, hdr, sizeof(WAVEHDR)))) return res;
		if ((res = waveOutWrite(ctx->handle, hdr, sizeof(WAVEHDR))))         return res;
		return 0;
	}
	/* tried to queue data without polling for free buffers first */
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Play(struct AudioContext* ctx) { return 0; }

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	cc_result res = 0;
	WAVEHDR* hdr;
	int i, count = 0;

	for (i = 0; i < ctx->count; i++) {
		hdr = &ctx->headers[i];
		if (!(hdr->dwFlags & WHDR_DONE)) { count++; continue; }
	
		if (!(hdr->dwFlags & WHDR_PREPARED)) continue;
		/* unprepare this WAVEHDR so it can be reused */
		res = waveOutUnprepareHeader(ctx->handle, hdr, sizeof(WAVEHDR));
		if (res) break;
	}

	*inUse = count; return res;
}


cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	int channels   = data->channels;
	int sampleRate = Audio_AdjustSampleRate(data);
	return !ctx->channels || (ctx->channels == channels && ctx->sampleRate == sampleRate);
}

cc_result Audio_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	cc_bool ok = AudioBase_AdjustSound(ctx, data);
	cc_result res;
	if (!ok) return ERR_OUT_OF_MEMORY; 
	data->sampleRate = Audio_AdjustSampleRate(data);

	if ((res = Audio_SetFormat(ctx, data->channels, data->sampleRate))) return res;
	if ((res = Audio_QueueData(ctx, data->data,    data->size)))        return res;
	return 0;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	char buffer[NATIVE_STR_LEN] = { 0 };
	waveOutGetErrorTextA(res, buffer, NATIVE_STR_LEN);

	if (!buffer[0]) return false;
	String_AppendConst(dst, buffer);
	return true;
}
#elif defined CC_BUILD_OPENSLES
/*########################################################################################################################*
*----------------------------------------------------OpenSL ES backend----------------------------------------------------*
*#########################################################################################################################*/
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
static SLObjectItf slEngineObject;
static SLEngineItf slEngineEngine;
static SLObjectItf slOutputObject;
#define AUDIO_HAS_BACKEND

struct AudioContext {
	int count, channels, sampleRate;
	SLObjectItf playerObject;
	SLPlayItf   playerPlayer;
	SLBufferQueueItf playerQueue;
	SLPlaybackRateItf playerRate;
	cc_uint32 _tmpSize; void* _tmpData;
};

static SLresult (SLAPIENTRY *_slCreateEngine)(SLObjectItf* engine, SLuint32 numOptions, const SLEngineOption* engineOptions,
							SLuint32 numInterfaces, const SLInterfaceID* interfaceIds, const SLboolean* interfaceRequired);
static SLInterfaceID* _SL_IID_NULL;
static SLInterfaceID* _SL_IID_PLAY;
static SLInterfaceID* _SL_IID_ENGINE;
static SLInterfaceID* _SL_IID_BUFFERQUEUE;
static SLInterfaceID* _SL_IID_PLAYBACKRATE;
static const cc_string slLib = String_FromConst("libOpenSLES.so");

static cc_bool LoadSLFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(slCreateEngine),     DynamicLib_Sym(SL_IID_NULL),
		DynamicLib_Sym(SL_IID_PLAY),        DynamicLib_Sym(SL_IID_ENGINE),
		DynamicLib_Sym(SL_IID_BUFFERQUEUE), DynamicLib_Sym(SL_IID_PLAYBACKRATE)
	};
	void* lib;
	
	return DynamicLib_LoadAll(&slLib, funcs, Array_Elems(funcs), &lib);
}

static cc_bool AudioBackend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init OpenSLES. No audio will play.");
	SLInterfaceID ids[1];
	SLboolean req[1];
	SLresult res;

	if (slEngineObject) return true;
	if (!LoadSLFuncs()) { Logger_WarnFunc(&msg); return false; }
	
	/* mixer doesn't use any effects */
	ids[0] = *_SL_IID_NULL; 
	req[0] = SL_BOOLEAN_FALSE;
	
	res = _slCreateEngine(&slEngineObject, 0, NULL, 0, NULL, NULL);
	if (res) { AudioWarn(res, "creating OpenSL ES engine"); return false; }

	res = (*slEngineObject)->Realize(slEngineObject, SL_BOOLEAN_FALSE);
	if (res) { AudioWarn(res, "realising OpenSL ES engine"); return false; }

	res = (*slEngineObject)->GetInterface(slEngineObject, *_SL_IID_ENGINE, &slEngineEngine);
	if (res) { AudioWarn(res, "initing OpenSL ES engine"); return false; }

	res = (*slEngineEngine)->CreateOutputMix(slEngineEngine, &slOutputObject, 1, ids, req);
	if (res) { AudioWarn(res, "creating OpenSL ES mixer"); return false; }

	res = (*slOutputObject)->Realize(slOutputObject, SL_BOOLEAN_FALSE);
	if (res) { AudioWarn(res, "realising OpenSL ES mixer"); return false; }

	return true;
}

static void AudioBackend_Free(void) {
	if (slOutputObject) {
		(*slOutputObject)->Destroy(slOutputObject);
		slOutputObject = NULL;
	}
	if (slEngineObject) {
		(*slEngineObject)->Destroy(slEngineObject);
		slEngineObject = NULL;
		slEngineEngine = NULL;
	}
}

void Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->count = buffers;
}

static void Audio_Stop(struct AudioContext* ctx) {
	if (!ctx->playerPlayer) return;

	(*ctx->playerQueue)->Clear(ctx->playerQueue);
	(*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_STOPPED);
}

static void Audio_Reset(struct AudioContext* ctx) {
	SLObjectItf playerObject = ctx->playerObject;
	if (!playerObject) return;

	(*playerObject)->Destroy(playerObject);
	ctx->playerObject = NULL;
	ctx->playerPlayer = NULL;
	ctx->playerQueue  = NULL;
	ctx->playerRate   = NULL;
}

void Audio_Close(struct AudioContext* ctx) {
	Audio_Stop(ctx);
	Audio_Reset(ctx);
	AudioBase_Clear(ctx);
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate) {
	SLDataLocator_AndroidSimpleBufferQueue input;
	SLDataLocator_OutputMix output;
	SLObjectItf playerObject;
	SLDataFormat_PCM fmt;
	SLInterfaceID ids[3];
	SLboolean req[3];
	SLDataSource src;
	SLDataSink dst;
	cc_result res;

	if (ctx->channels == channels && ctx->sampleRate == sampleRate) return 0;
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;
	Audio_Reset(ctx);

	fmt.formatType     = SL_DATAFORMAT_PCM;
	fmt.numChannels    = channels;
	fmt.samplesPerSec  = sampleRate * 1000;
	fmt.bitsPerSample  = SL_PCMSAMPLEFORMAT_FIXED_16;
	fmt.containerSize  = SL_PCMSAMPLEFORMAT_FIXED_16;
	fmt.channelMask    = 0;
	fmt.endianness     = SL_BYTEORDER_LITTLEENDIAN;

	input.locatorType  = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	input.numBuffers   = ctx->count;
	output.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	output.outputMix   = slOutputObject;

	src.pLocator = &input;
	src.pFormat  = &fmt;
	dst.pLocator = &output;
	dst.pFormat  = NULL;

	ids[0] = *_SL_IID_BUFFERQUEUE;  req[0] = SL_BOOLEAN_TRUE;
	ids[1] = *_SL_IID_PLAY;         req[1] = SL_BOOLEAN_TRUE;
	ids[2] = *_SL_IID_PLAYBACKRATE; req[2] = SL_BOOLEAN_TRUE;

	res = (*slEngineEngine)->CreateAudioPlayer(slEngineEngine, &playerObject, &src, &dst, 3, ids, req);
	ctx->playerObject = playerObject;
	if (res) return res;

	if ((res = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE)))                               return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_PLAY,         &ctx->playerPlayer))) return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_BUFFERQUEUE,  &ctx->playerQueue)))  return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_PLAYBACKRATE, &ctx->playerRate)))   return res;
	return 0;
}

cc_result Audio_QueueData(struct AudioContext* ctx, void* data, cc_uint32 size) {
	return (*ctx->playerQueue)->Enqueue(ctx->playerQueue, data, size);
}

static cc_result Audio_Pause(struct AudioContext* ctx) {
	return (*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_PAUSED);
}

cc_result Audio_Play(struct AudioContext* ctx) {
	return (*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_PLAYING);
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	SLBufferQueueState state = { 0 };
	cc_result res = 0;

	if (ctx->playerQueue) {
		res = (*ctx->playerQueue)->GetState(ctx->playerQueue, &state);	
	}
	*inUse  = state.count;
	return res;
}

cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return !ctx->channels || (ctx->channels == data->channels && ctx->sampleRate == data->sampleRate);
}

cc_result Audio_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	cc_bool ok = AudioBase_AdjustSound(ctx, data);
	cc_result res;
	if (!ok) return ERR_OUT_OF_MEMORY; 

	if ((res = Audio_SetFormat(ctx, data->channels, data->sampleRate))) return res;
	/* rate is in milli, so 1000 = normal rate */
	if ((res = (*ctx->playerRate)->SetRate(ctx->playerRate, data->rate * 10))) return res;

	if ((res = Audio_QueueData(ctx, data->data, data->size))) return res;
	if ((res = Audio_Play(ctx))) return res;
	return 0;
}

static const char* GetError(cc_result res) {
	switch (res) {
	case SL_RESULT_PRECONDITIONS_VIOLATED: return "Preconditions violated";
	case SL_RESULT_PARAMETER_INVALID:   return "Invalid parameter";
	case SL_RESULT_MEMORY_FAILURE:      return "Memory failure";
	case SL_RESULT_RESOURCE_ERROR:      return "Resource error";
	case SL_RESULT_RESOURCE_LOST:       return "Resource lost";
	case SL_RESULT_IO_ERROR:            return "I/O error";
	case SL_RESULT_BUFFER_INSUFFICIENT: return "Insufficient buffer";
	case SL_RESULT_CONTENT_CORRUPTED:   return "Content corrupted";
	case SL_RESULT_CONTENT_UNSUPPORTED: return "Content unsupported";
	case SL_RESULT_CONTENT_NOT_FOUND:   return "Content not found";
	case SL_RESULT_PERMISSION_DENIED:   return "Permission denied";
	case SL_RESULT_FEATURE_UNSUPPORTED: return "Feature unsupported";
	case SL_RESULT_INTERNAL_ERROR:      return "Internal error";
	case SL_RESULT_UNKNOWN_ERROR:       return "Unknown error";
	case SL_RESULT_OPERATION_ABORTED:   return "Operation aborted";
	case SL_RESULT_CONTROL_LOST:        return "Control lost";
	}
	return NULL;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	const char* err = GetError(res);
	if (err) String_AppendConst(dst, err);
	return err != NULL;
}
#elif defined CC_BUILD_WEBAUDIO
/*########################################################################################################################*
*-----------------------------------------------------WebAudio backend----------------------------------------------------*
*#########################################################################################################################*/
struct AudioContext { int contextID; };
extern int  interop_InitAudio(void);
extern int  interop_AudioCreate(void);
extern void interop_AudioClose(int contextID);
extern int  interop_AudioPlay(int contextID, const void* name, int volume, int rate);
extern int  interop_AudioPoll(int contetID, int* inUse);
extern int  interop_AudioDescribe(int res, char* buffer, int bufferLen);

static cc_bool AudioBackend_Init(void) {
	cc_result res = interop_InitAudio();
	if (res) { AudioWarn(res, "initing WebAudio context"); return false; }
	return true;
}

void Audio_Init(struct AudioContext* ctx, int buffers) { }
void Audio_Close(struct AudioContext* ctx) {
	if (ctx->contextID) interop_AudioClose(ctx->contextID);
	ctx->contextID = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate) {
	return ERR_NOT_SUPPORTED;
}
cc_result Audio_QueueData(struct AudioContext* ctx, void* data, cc_uint32 size) {
	return ERR_NOT_SUPPORTED;
}
cc_result Audio_Play(struct AudioContext* ctx) { return ERR_NOT_SUPPORTED; }

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	if (ctx->contextID)
		return interop_AudioPoll(ctx->contextID, inUse);

	*inUse = 0; return 0;
}

cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	/* Channels/Sample rate is per buffer, not a per source property */
	return true;
}

cc_result Audio_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	if (!ctx->contextID) 
		ctx->contextID = interop_AudioCreate();
	return interop_AudioPlay(ctx->contextID, data->data, data->volume, data->rate);
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	char buffer[NATIVE_STR_LEN];
	int len = interop_AudioDescribe(res, buffer, NATIVE_STR_LEN);

	String_AppendUtf8(dst, buffer, len);
	return len > 0;
}
#endif

/*########################################################################################################################*
*---------------------------------------------------Common backend code---------------------------------------------------*
*#########################################################################################################################*/
#ifndef AUDIO_HAS_BACKEND
static void AudioBackend_Free(void) { }
#else
static void AudioBase_Clear(struct AudioContext* ctx) {
	ctx->count      = 0;
	ctx->channels   = 0;
	ctx->sampleRate = 0;

	Mem_Free(ctx->_tmpData);
	ctx->_tmpData = NULL;
	ctx->_tmpSize = 0;
}

static cc_bool AudioBase_AdjustSound(struct AudioContext* ctx, struct AudioData* data) {
	void* audio;
	if (data->volume >= 100) return true;

	/* copy to temp buffer to apply volume */
	if (ctx->_tmpSize < data->size) {
		/* TODO: check if we can realloc NULL without a problem */
		if (ctx->_tmpData) {
			audio = Mem_TryRealloc(ctx->_tmpData, data->size, 1);
		} else {
			audio = Mem_TryAlloc(data->size, 1);
		}

		if (!data) return false;
		ctx->_tmpData = audio;
		ctx->_tmpSize = data->size;
	}

	audio = ctx->_tmpData;
	Mem_Copy(audio, data->data, data->size);
	ApplyVolume((cc_int16*)audio, data->size / 2, data->volume);
	data->data = audio;
	return true;
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------Sounds-----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NOSOUNDS
/* Can't use mojang's sound assets, so just stub everything out */
static void Sounds_Init(void) { }
static void Sounds_Free(void) { }
static void Sounds_Stop(void) { }
static void Sounds_Start(void) {
	Chat_AddRaw("&cSounds are not supported currently");
	Audio_SoundsVolume = 0;
}

void Audio_PlayDigSound(cc_uint8 type)  { }
void Audio_PlayStepSound(cc_uint8 type) { }
#else
#define AUDIO_MAX_SOUNDS 10
#define SOUND_MAX_CONTEXTS 8

struct SoundGroup {
	int count;
	struct Sound sounds[AUDIO_MAX_SOUNDS];
};
struct Soundboard { struct SoundGroup groups[SOUND_COUNT]; };

static struct Soundboard digBoard, stepBoard;
static struct AudioContext sound_contexts[SOUND_MAX_CONTEXTS];
static RNGState sounds_rnd;

#define WAV_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)
#define WAV_FMT_SIZE 16

static cc_result Sound_ReadWaveData(struct Stream* stream, struct Sound* snd) {
	cc_uint32 fourCC, size;
	cc_uint8 tmp[WAV_FMT_SIZE];
	cc_result res;
	int bitsPerSample;

	if ((res = Stream_Read(stream, tmp, 12)))  return res;
	fourCC = Stream_GetU32_BE(tmp + 0);
	if (fourCC == WAV_FourCC('I','D','3', 2))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.2 tags header */
	if (fourCC == WAV_FourCC('I','D','3', 3))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.3 tags header */
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;

	/* tmp[4] (4) file size */
	fourCC = Stream_GetU32_BE(tmp + 8);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	for (;;) {
		if ((res = Stream_Read(stream, tmp, 8))) return res;
		fourCC = Stream_GetU32_BE(tmp + 0);
		size   = Stream_GetU32_LE(tmp + 4);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			if ((res = Stream_Read(stream, tmp, sizeof(tmp)))) return res;
			if (Stream_GetU16_LE(tmp + 0) != 1) return WAV_ERR_DATA_TYPE;

			snd->channels   = Stream_GetU16_LE(tmp + 2);
			snd->sampleRate = Stream_GetU32_LE(tmp + 4);
			/* tmp[8] (6) alignment data and stuff */

			bitsPerSample = Stream_GetU16_LE(tmp + 14);
			if (bitsPerSample != 16) return WAV_ERR_SAMPLE_BITS;
			size -= WAV_FMT_SIZE;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->data = Mem_TryAlloc(size, 1);
			snd->size = size;

			if (!snd->data) return ERR_OUT_OF_MEMORY;
			return Stream_Read(stream, (cc_uint8*)snd->data, size);
		}

		/* Skip over unhandled data */
		if (size && (res = stream->Skip(stream, size))) return res;
	}
}

static cc_result Sound_ReadWave(const cc_string* path, struct Sound* snd) {
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res) return res;
	res = Sound_ReadWaveData(&stream, snd);

	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
	return res;
}

static struct SoundGroup* Soundboard_Find(struct Soundboard* board, const cc_string* name) {
	struct SoundGroup* groups = board->groups;
	int i;

	for (i = 0; i < SOUND_COUNT; i++) {
		if (String_CaselessEqualsConst(name, Sound_Names[i])) return &groups[i];
	}
	return NULL;
}

static void Soundboard_Load(struct Soundboard* board, const cc_string* boardName, const cc_string* file) {
	struct SoundGroup* group;
	struct Sound* snd;
	cc_string name = *file;
	cc_result res;
	int dotIndex;
	Utils_UNSAFE_TrimFirstDirectory(&name);

	/* dig_grass1.wav -> dig_grass1 */
	dotIndex = String_LastIndexOf(&name, '.');
	if (dotIndex >= 0) name.length = dotIndex;
	if (!String_CaselessStarts(&name, boardName)) return;

	/* Convert dig_grass1 to grass */
	name = String_UNSAFE_SubstringAt(&name, boardName->length);
	name = String_UNSAFE_Substring(&name, 0, name.length - 1);

	group = Soundboard_Find(board, &name);
	if (!group) {
		Chat_Add1("&cUnknown sound group '%s'", &name); return;
	}
	if (group->count == Array_Elems(group->sounds)) {
		Chat_AddRaw("&cCannot have more than 10 sounds in a group"); return;
	}

	snd = &group->sounds[group->count];
	res = Sound_ReadWave(file, snd);

	if (res) {
		Logger_SysWarn2(res, "decoding", file);
		Mem_Free(snd->data);
		snd->data = NULL;
		snd->size = 0;
	} else { group->count++; }
}

static const struct Sound* Soundboard_PickRandom(struct Soundboard* board, cc_uint8 type) {
	struct SoundGroup* group;
	int idx;

	if (type == SOUND_NONE || type >= SOUND_COUNT) return NULL;
	if (type == SOUND_METAL) type = SOUND_STONE;

	group = &board->groups[type];
	if (!group->count) return NULL;

	idx = Random_Next(&sounds_rnd, group->count);
	return &group->sounds[idx];
}


CC_NOINLINE static void Sounds_Fail(cc_result res) {
	AudioWarn(res, "playing sounds");
	Chat_AddRaw("&cDisabling sounds");
	Audio_SetSounds(0);
}

static void Sounds_Play(cc_uint8 type, struct Soundboard* board) {
	struct AudioData data;
	const struct Sound* snd;
	struct AudioContext* ctx;
	int inUse, i;
	cc_result res;

	if (type == SOUND_NONE || !Audio_SoundsVolume) return;
	snd = Soundboard_PickRandom(board, type);
	if (!snd) return;

	data.data       = snd->data;
	data.size       = snd->size;
	data.channels   = snd->channels;
	data.sampleRate = snd->sampleRate;
	data.rate       = 100;
	data.volume     = Audio_SoundsVolume;

	/* https://minecraft.fandom.com/wiki/Block_of_Gold#Sounds */
	/* https://minecraft.fandom.com/wiki/Grass#Sounds */
	if (board == &digBoard) {
		if (type == SOUND_METAL) data.rate = 120;
		else data.rate = 80;
	} else {
		data.volume /= 2;
		if (type == SOUND_METAL) data.rate = 140;
	}

	/* Try to play on a context that doesn't need to be recreated */
	for (i = 0; i < SOUND_MAX_CONTEXTS; i++) {
		ctx = &sound_contexts[i];
		res = Audio_Poll(ctx, &inUse);

		if (res) { Sounds_Fail(res); return; }
		if (inUse > 0) continue;
		if (!Audio_FastPlay(ctx, &data)) continue;

		res = Audio_PlayData(ctx, &data);
		if (res) Sounds_Fail(res);
		return;
	}

	/* Try again with all contexts, even if need to recreate one (expensive) */
	for (i = 0; i < SOUND_MAX_CONTEXTS; i++) {
		ctx = &sound_contexts[i];
		res = Audio_Poll(ctx, &inUse);

		if (res) { Sounds_Fail(res); return; }
		if (inUse > 0) continue;

		res = Audio_PlayData(ctx, &data);
		if (res) Sounds_Fail(res);
		return;
	}
}

static void Audio_PlayBlockSound(void* obj, IVec3 coords, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		Audio_PlayDigSound(Blocks.DigSounds[old]);
	} else if (!Game_ClassicMode) {
		/* use StepSounds instead when placing, as don't want */
		/*  to play glass break sound when placing glass */
		Audio_PlayDigSound(Blocks.StepSounds[now]);
	}
}

static void Sounds_LoadFile(const cc_string* path, void* obj) {
	static const cc_string dig  = String_FromConst("dig_");
	static const cc_string step = String_FromConst("step_");
	Soundboard_Load(&digBoard,  &dig,  path);
	Soundboard_Load(&stepBoard, &step, path);
}

/* TODO this is a pretty terrible solution */
#ifdef CC_BUILD_WEBAUDIO
static const struct SoundID { int group; const char* name; } sounds_list[] =
{
	{ SOUND_CLOTH,  "step_cloth1"  }, { SOUND_CLOTH,  "step_cloth2"  }, { SOUND_CLOTH,  "step_cloth3"  }, { SOUND_CLOTH,  "step_cloth4"  },
	{ SOUND_GRASS,  "step_grass1"  }, { SOUND_GRASS,  "step_grass2"  }, { SOUND_GRASS,  "step_grass3"  }, { SOUND_GRASS,  "step_grass4"  },
	{ SOUND_GRAVEL, "step_gravel1" }, { SOUND_GRAVEL, "step_gravel2" }, { SOUND_GRAVEL, "step_gravel3" }, { SOUND_GRAVEL, "step_gravel4" },
	{ SOUND_SAND,   "step_sand1"   }, { SOUND_SAND,   "step_sand2"   }, { SOUND_SAND,   "step_sand3"   }, { SOUND_SAND,   "step_sand4"   },
	{ SOUND_SNOW,   "step_snow1"   }, { SOUND_SNOW,   "step_snow2"   }, { SOUND_SNOW,   "step_snow3"   }, { SOUND_SNOW,   "step_snow4"   },
	{ SOUND_STONE,  "step_stone1"  }, { SOUND_STONE,  "step_stone2"  }, { SOUND_STONE,  "step_stone3"  }, { SOUND_STONE,  "step_stone4"  },
	{ SOUND_WOOD,   "step_wood1"   }, { SOUND_WOOD,   "step_wood2"   }, { SOUND_WOOD,   "step_wood3"   }, { SOUND_WOOD,   "step_wood4"   },
	{ SOUND_NONE, NULL },

	{ SOUND_CLOTH,  "dig_cloth1"   }, { SOUND_CLOTH,  "dig_cloth2"   }, { SOUND_CLOTH,  "dig_cloth3"   }, { SOUND_CLOTH,  "dig_cloth4"   },
	{ SOUND_GRASS,  "dig_grass1"   }, { SOUND_GRASS,  "dig_grass2"   }, { SOUND_GRASS,  "dig_grass3"   }, { SOUND_GRASS,  "dig_grass4"   },
	{ SOUND_GLASS,  "dig_glass1"   }, { SOUND_GLASS,  "dig_glass2"   }, { SOUND_GLASS,  "dig_glass3"   },
	{ SOUND_GRAVEL, "dig_gravel1"  }, { SOUND_GRAVEL, "dig_gravel2"  }, { SOUND_GRAVEL, "dig_gravel3"  }, { SOUND_GRAVEL, "dig_gravel4"  },
	{ SOUND_SAND,   "dig_sand1"    }, { SOUND_SAND,   "dig_sand2"    }, { SOUND_SAND,   "dig_sand3"    }, { SOUND_SAND,   "dig_sand4"    },
	{ SOUND_SNOW,   "dig_snow1"    }, { SOUND_SNOW,   "dig_snow2"    }, { SOUND_SNOW,   "dig_snow3"    }, { SOUND_SNOW,   "dig_snow4"    },
	{ SOUND_STONE,  "dig_stone1"   }, { SOUND_STONE,  "dig_stone2"   }, { SOUND_STONE,  "dig_stone3"   }, { SOUND_STONE,  "dig_stone4"   },
	{ SOUND_WOOD,   "dig_wood1"    }, { SOUND_WOOD,   "dig_wood2"    }, { SOUND_WOOD,   "dig_wood3"    }, { SOUND_WOOD,   "dig_wood4"    },
};

/* TODO this is a terrible solution */
static void InitWebSounds(void) {
	struct Soundboard* board = &stepBoard;
	struct SoundGroup* group;
	int i;

	for (i = 0; i < Array_Elems(sounds_list); i++) {
		if (sounds_list[i].group == SOUND_NONE) {
			board = &digBoard;
		} else {
			group = &board->groups[sounds_list[i].group];
			group->sounds[group->count++].data = sounds_list[i].name;
		}
	}
}
#endif

static cc_bool sounds_loaded;
static void Sounds_Start(void) {
	int i;
	if (!AudioBackend_Init()) { 
		AudioBackend_Free(); 
		Audio_SoundsVolume = 0; 
		return; 
	}

	for (i = 0; i < SOUND_MAX_CONTEXTS; i++) {
		Audio_Init(&sound_contexts[i], 1);
	}

	if (sounds_loaded) return;
	sounds_loaded = true;
#ifdef CC_BUILD_WEBAUDIO
	InitWebSounds();
#else
	Directory_Enum(&audio_dir, NULL, Sounds_LoadFile);
#endif
}

static void Sounds_Stop(void) {
	int i;
	for (i = 0; i < SOUND_MAX_CONTEXTS; i++) {
		Audio_Close(&sound_contexts[i]);
	}
}

static void Sounds_Init(void) {
	int volume = Options_GetInt(OPT_SOUND_VOLUME, 0, 100, DEFAULT_SOUNDS_VOLUME);
	Audio_SetSounds(volume);
	Event_Register_(&UserEvents.BlockChanged, NULL, Audio_PlayBlockSound);
}
static void Sounds_Free(void) { Sounds_Stop(); }

void Audio_PlayDigSound(cc_uint8 type)  { Sounds_Play(type, &digBoard); }
void Audio_PlayStepSound(cc_uint8 type) { Sounds_Play(type, &stepBoard); }
#endif


/*########################################################################################################################*
*--------------------------------------------------------Music------------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NOMUSIC
/* Can't use mojang's music assets, so just stub everything out */
static void Music_Init(void) { }
static void Music_Free(void) { }
static void Music_Stop(void) { }
static void Music_Start(void) {
	Chat_AddRaw("&cMusic is not supported currently");
	Audio_MusicVolume = 0;
}
#else
static struct AudioContext music_ctx;
static void* music_thread;
static void* music_waitable;
static volatile cc_bool music_stopping, music_joining;
static int music_minDelay, music_maxDelay;

static cc_result Music_Buffer(cc_int16* data, int maxSamples, struct VorbisState* ctx) {
	int samples = 0;
	cc_int16* cur;
	cc_result res = 0, res2;

	while (samples < maxSamples) {
		if ((res = Vorbis_DecodeFrame(ctx))) break;

		cur = &data[samples];
		samples += Vorbis_OutputFrame(ctx, cur);
	}
	if (Audio_MusicVolume < 100) { ApplyVolume(data, samples, Audio_MusicVolume); }

	res2 = Audio_QueueData(&music_ctx, data, samples * 2);
	if (res2) { music_stopping = true; return res2; }
	return res;
}

static cc_result Music_PlayOgg(struct Stream* source) {
	struct OggState ogg;
	struct VorbisState vorbis = { 0 };
	int channels, sampleRate;

	int chunkSize, samplesPerSecond;
	cc_int16* data = NULL;
	int inUse, i, cur;
	cc_result res;

	Ogg_Init(&ogg, source);
	vorbis.source = &ogg;
	if ((res = Vorbis_DecodeHeaders(&vorbis))) goto cleanup;
	
	channels   = vorbis.channels;
	sampleRate = vorbis.sampleRate;
	if ((res = Audio_SetFormat(&music_ctx, channels, sampleRate))) goto cleanup;

	/* largest possible vorbis frame decodes to blocksize1 * channels samples, */
	/*  so can end up decoding slightly over a second of audio */
	chunkSize        = channels * (sampleRate + vorbis.blockSizes[1]);
	samplesPerSecond = channels * sampleRate;

	cur  = 0;
	data = (cc_int16*)Mem_TryAlloc(chunkSize * AUDIO_MAX_BUFFERS, 2);
	if (!data) { res = ERR_OUT_OF_MEMORY; goto cleanup; }

	/* fill up with some samples before playing */
	for (i = 0; i < AUDIO_MAX_BUFFERS && !res; i++) {
		res = Music_Buffer(&data[chunkSize * cur], samplesPerSecond, &vorbis);
		cur = (cur + 1) % AUDIO_MAX_BUFFERS;
	}
	if (music_stopping) goto cleanup;

	res  = Audio_Play(&music_ctx);
	if (res) goto cleanup;

	while (!music_stopping) {
#ifdef CC_BUILD_ANDROID
		/* Don't play music while in the background on Android */
    	/* TODO: Not use such a terrible approach */
    	if (!WindowInfo.Handle) {
    		Audio_Pause(&music_ctx);
    		while (!WindowInfo.Handle && !music_stopping) {
    			Thread_Sleep(10); continue;
    		}
    		Audio_Play(&music_ctx);
    	}
#endif

		res = Audio_Poll(&music_ctx, &inUse);
		if (res) { music_stopping = true; break; }

		if (inUse >= AUDIO_MAX_BUFFERS) {
			Thread_Sleep(10); continue;
		}

		res = Music_Buffer(&data[chunkSize * cur], samplesPerSecond, &vorbis);
		cur = (cur + 1) % AUDIO_MAX_BUFFERS;

		/* need to specially handle last bit of audio */
		if (res) break;
	}

	if (music_stopping) {
		/* must close audio context, as otherwise some of the audio */
		/*  context's internal audio buffers may have a reference */
		/*  to the `data` buffer which will be freed after this */
		Audio_Close(&music_ctx);
	} else {
		/* Wait until the buffers finished playing */
		for (;;) {
			if (Audio_Poll(&music_ctx, &inUse) || inUse == 0) break;
			Thread_Sleep(10);
		}
	}

cleanup:
	Mem_Free(data);
	Vorbis_Free(&vorbis);
	return res == ERR_END_OF_STREAM ? 0 : res;
}

static void Music_AddFile(const cc_string* path, void* obj) {
	struct StringsBuffer* files = (struct StringsBuffer*)obj;
	static const cc_string ogg  = String_FromConst(".ogg");

	if (!String_CaselessEnds(path, &ogg)) return;
	StringsBuffer_Add(files, path);
}

static void Music_RunLoop(void) {
	struct StringsBuffer files;
	cc_string path;
	RNGState rnd;
	struct Stream stream;
	int idx, delay;
	cc_result res = 0;

	StringsBuffer_SetLengthBits(&files, STRINGSBUFFER_DEF_LEN_SHIFT);
	StringsBuffer_Init(&files);
	Directory_Enum(&audio_dir, &files, Music_AddFile);

	Random_SeedFromCurrentTime(&rnd);
	Audio_Init(&music_ctx, AUDIO_MAX_BUFFERS);

	while (!music_stopping && files.count) {
		idx  = Random_Next(&rnd, files.count);
		path = StringsBuffer_UNSAFE_Get(&files, idx);
		Platform_Log1("playing music file: %s", &path);

		res = Stream_OpenFile(&stream, &path);
		if (res) { Logger_SysWarn2(res, "opening", &path); break; }

		res = Music_PlayOgg(&stream);
		if (res) { Logger_SimpleWarn2(res, "playing", &path); }

		/* No point logging error for closing readonly file */
		(void)stream.Close(&stream);

		if (music_stopping) break;
		delay = Random_Range(&rnd, music_minDelay, music_maxDelay);
		Waitable_WaitFor(music_waitable, delay);
	}

	if (res) {
		Chat_AddRaw("&cDisabling music");
		Audio_MusicVolume = 0;
	}
	Audio_Close(&music_ctx);
	StringsBuffer_Clear(&files);

	if (music_joining) return;
	Thread_Detach(music_thread);
	music_thread = NULL;
}

static void Music_Start(void) {
	if (music_thread) return;
	if (!AudioBackend_Init()) {
		AudioBackend_Free(); 
		Audio_MusicVolume = 0;
		return; 
	}

	music_joining  = false;
	music_stopping = false;

	music_thread = Thread_Create(Music_RunLoop);
	Thread_Start2(music_thread, Music_RunLoop);
}

static void Music_Stop(void) {
	music_joining  = true;
	music_stopping = true;
	Waitable_Signal(music_waitable);
	
	if (music_thread) Thread_Join(music_thread);
	music_thread = NULL;
}

static void Music_Init(void) {
	int volume;
	/* music is delayed between 2 - 7 minutes by default */
	music_minDelay = Options_GetInt(OPT_MIN_MUSIC_DELAY, 0, 3600, 120) * MILLIS_PER_SEC;
	music_maxDelay = Options_GetInt(OPT_MAX_MUSIC_DELAY, 0, 3600, 420) * MILLIS_PER_SEC;
	music_waitable = Waitable_Create();

	volume = Options_GetInt(OPT_MUSIC_VOLUME, 0, 100, DEFAULT_MUSIC_VOLUME);
	Audio_SetMusic(volume);
}

static void Music_Free(void) {
	Music_Stop();
	Waitable_Free(music_waitable);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
void Audio_SetSounds(int volume) {
	Audio_SoundsVolume = volume;
	if (volume) Sounds_Start();
	else        Sounds_Stop();
}

void Audio_SetMusic(int volume) {
	Audio_MusicVolume = volume;
	if (volume) Music_Start();
	else        Music_Stop();
}

static void OnInit(void) {
	Sounds_Init();
	Music_Init();
}

static void OnFree(void) {
	Sounds_Free();
	Music_Free();
	AudioBackend_Free();
}

struct IGameComponent Audio_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
