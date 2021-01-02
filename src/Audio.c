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

#if defined CC_BUILD_NOAUDIO
/* Can't use mojang's sounds or music assets, so just stub everything out */
static void OnInit(void) { }
static void OnFree(void) { }

void Audio_SetMusic(int volume) {
	if (volume) Chat_AddRaw("&cMusic is not supported currently");
}
void Audio_SetSounds(int volume) {
	if (volume) Chat_AddRaw("&cSounds are not supported currently");
}

void Audio_PlayDigSound(cc_uint8 type) { }
void Audio_PlayStepSound(cc_uint8 type) { }
#else
static struct StringsBuffer files;

static void Volume_Mix16(cc_int16* samples, int count, int volume) {
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


/*########################################################################################################################*
*------------------------------------------------Native implementation----------------------------------------------------*
*#########################################################################################################################*/
static cc_result Audio_AllAvailable(struct AudioContext* ctx, cc_bool* finished);
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
	cc_bool available[AUDIO_MAX_BUFFERS];
	struct AudioFormat format;
	int count;
	ALenum dataFormat;
};
static void* audio_device;
static void* audio_context;

#if defined CC_BUILD_WIN
static const cc_string alLib = String_FromConst("openal32.dll");
#elif defined CC_BUILD_DARWIN
static const cc_string alLib = String_FromConst("/System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL");
#elif defined CC_BUILD_BSD
static const cc_string alLib = String_FromConst("libopenal.so");
#else
static const cc_string alLib = String_FromConst("libopenal.so.1");
#endif

#define QUOTE(x) #x
#define DefineALFunc(sym) { QUOTE(sym), (void**)&_ ## sym }
static cc_bool LoadALFuncs(void) {
	static const struct DynamicLibSym funcs[18] = {
		DefineALFunc(alcCreateContext),  DefineALFunc(alcMakeContextCurrent),
		DefineALFunc(alcDestroyContext), DefineALFunc(alcOpenDevice),
		DefineALFunc(alcCloseDevice),    DefineALFunc(alcGetError),

		DefineALFunc(alGetError),        DefineALFunc(alGenSources),
		DefineALFunc(alDeleteSources),   DefineALFunc(alGetSourcei),
		DefineALFunc(alSourcePlay),      DefineALFunc(alSourceStop),
		DefineALFunc(alSourceQueueBuffers), DefineALFunc(alSourceUnqueueBuffers),
		DefineALFunc(alGenBuffers),      DefineALFunc(alDeleteBuffers),
		DefineALFunc(alBufferData),      DefineALFunc(alDistanceModel)
	};

	void* lib = DynamicLib_Load2(&alLib);
	if (!lib) { Logger_DynamicLibWarn("loading", &alLib); return false; }
	return DynamicLib_GetAll(lib, funcs, Array_Elems(funcs));
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

static cc_bool Backend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init OpenAL. No audio will play.");
	cc_result res;
	if (audio_device) return true;
	if (!LoadALFuncs()) { Logger_WarnFunc(&msg); return false; }

	res = CreateALContext();
	if (res) { Logger_SimpleWarn(res, "initing OpenAL"); return false; }
	return true;
}

static void Backend_Free(void) {
	if (!audio_device) return;
	_alcMakeContextCurrent(NULL);

	if (audio_context) _alcDestroyContext(audio_context);
	if (audio_device)  _alcCloseDevice(audio_device);

	audio_context = NULL;
	audio_device  = NULL;
}

void Audio_Init(struct AudioContext* ctx, int buffers) {
	int i;
	_alDistanceModel(AL_NONE);
	ctx->source = -1;

	for (i = 0; i < buffers; i++) {
		ctx->available[i] = true;
	}
	ctx->count = buffers;
}

static cc_result Backend_Reset(struct AudioContext* ctx) {
	ALenum err;
	if (ctx->source == -1) return 0;

	_alDeleteSources(1, &ctx->source);
	ctx->source = -1;
	if ((err = _alGetError())) return err;

	_alDeleteBuffers(ctx->count, ctx->buffers);
	if ((err = _alGetError())) return err;
	return 0;
}

static ALenum GetALFormat(int channels) {
	if (channels == 1) return AL_FORMAT_MONO16;
	if (channels == 2) return AL_FORMAT_STEREO16;
	Logger_Abort("Unsupported audio format"); return 0;
}

static cc_result Backend_SetFormat(struct AudioContext* ctx, struct AudioFormat* format) {
	ALenum err;
	ctx->dataFormat = GetALFormat(format->channels);
	
	_alGenSources(1, &ctx->source);
	if ((err = _alGetError())) return err;

	_alGenBuffers(ctx->count, ctx->buffers);
	if ((err = _alGetError())) return err;
	return 0;
}

cc_result Audio_BufferData(struct AudioContext* ctx, int idx, void* data, cc_uint32 size) {
	ALuint buffer = ctx->buffers[idx];
	ALenum err;
	ctx->available[idx] = false;

	_alBufferData(buffer, ctx->dataFormat, data, size, ctx->format.sampleRate);
	if ((err = _alGetError())) return err;
	_alSourceQueueBuffers(ctx->source, 1, &buffer);
	if ((err = _alGetError())) return err;
	return 0;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	_alSourcePlay(ctx->source);
	return _alGetError();
}

cc_result Audio_Stop(struct AudioContext* ctx) {
	_alSourceStop(ctx->source);
	return _alGetError();
}

cc_result Audio_IsAvailable(struct AudioContext* ctx, int idx, cc_bool* available) {
	ALint i, processed = 0;
	ALuint buffer;
	ALenum err;

	_alGetSourcei(ctx->source, AL_BUFFERS_PROCESSED, &processed);
	if ((err = _alGetError())) return err;

	if (processed > 0) {
		_alSourceUnqueueBuffers(ctx->source, 1, &buffer);
		if ((err = _alGetError())) return err;

		for (i = 0; i < ctx->count; i++) {
			if (ctx->buffers[i] == buffer) ctx->available[i] = true;
		}
	}
	*available = ctx->available[idx]; return 0;
}

cc_result Audio_IsFinished(struct AudioContext* ctx, cc_bool* finished) {
	ALint state = 0;
	cc_result res;

	if (ctx->source == -1) { *finished = true; return 0; }
	res = Audio_AllAvailable(ctx, finished);
	if (res) return res;
	
	_alGetSourcei(ctx->source, AL_SOURCE_STATE, &state);
	*finished = state != AL_PLAYING; return 0;
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
#include <mmsystem.h>

struct AudioContext {
	HWAVEOUT handle;
	WAVEHDR headers[AUDIO_MAX_BUFFERS];
	struct AudioFormat format;
	int count;
};
static cc_bool Backend_Init(void) { return true; }
static void Backend_Free(void) { }

void Audio_Init(struct AudioContext* ctx, int buffers) {
	int i;
	for (i = 0; i < buffers; i++) {
		ctx->headers[i].dwFlags = WHDR_DONE;
	}
	ctx->count = buffers;
}

static cc_result Backend_Reset(struct AudioContext* ctx) {
	cc_result res;
	if (!ctx->handle) return 0;

	res = waveOutClose(ctx->handle);
	ctx->handle = NULL;
	return res;
}

static cc_result Backend_SetFormat(struct AudioContext* ctx, struct AudioFormat* format) {
	WAVEFORMATEX fmt;
	int sampleSize = format->channels * 2; /* 16 bits per sample / 8 */

	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = format->channels;
	fmt.nSamplesPerSec  = format->sampleRate;
	fmt.nAvgBytesPerSec = format->sampleRate * sampleSize;
	fmt.nBlockAlign     = sampleSize;
	fmt.wBitsPerSample  = 16;
	fmt.cbSize          = 0;
	return waveOutOpen(&ctx->handle, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
}

cc_result Audio_BufferData(struct AudioContext* ctx, int idx, void* data, cc_uint32 dataSize) {
	WAVEHDR* hdr = &ctx->headers[idx];
	cc_result res;

	Mem_Set(hdr, 0, sizeof(WAVEHDR));
	hdr->lpData         = (LPSTR)data;
	hdr->dwBufferLength = dataSize;
	hdr->dwLoops        = 1;
	
	if ((res = waveOutPrepareHeader(ctx->handle, hdr, sizeof(WAVEHDR)))) return res;
	if ((res = waveOutWrite(ctx->handle, hdr, sizeof(WAVEHDR))))         return res;
	return 0;
}

cc_result Audio_Play(struct AudioContext* ctx) { return 0; }

cc_result Audio_Stop(struct AudioContext* ctx) {
	if (!ctx->handle) return 0;
	return waveOutReset(ctx->handle);
}

cc_result Audio_IsAvailable(struct AudioContext* ctx, int idx, cc_bool* available) {
	WAVEHDR* hdr = &ctx->headers[idx];

	*available = false;
	if (!(hdr->dwFlags & WHDR_DONE)) return 0;
	cc_result res = 0;

	if (hdr->dwFlags & WHDR_PREPARED) {
		res = waveOutUnprepareHeader(ctx->handle, hdr, sizeof(WAVEHDR));
	}
	*available = true; return res;
}

cc_result Audio_IsFinished(struct AudioContext* ctx, cc_bool* finished) { return Audio_AllAvailable(ctx, finished); }
#elif defined CC_BUILD_OPENSLES
/*########################################################################################################################*
*----------------------------------------------------OpenSL ES backend----------------------------------------------------*
*#########################################################################################################################*/
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
static SLObjectItf slEngineObject;
static SLEngineItf slEngineEngine;
static SLObjectItf slOutputObject;

struct AudioContext {
	struct AudioFormat format;
	int count;
	cc_bool available[AUDIO_MAX_BUFFERS];
	SLObjectItf bqPlayerObject;
	SLPlayItf   bqPlayerPlayer;
	SLBufferQueueItf bqPlayerQueue;
};

static cc_bool Backend_Init(void) {
	SLInterfaceID ids[1];
	SLboolean req[1];
	SLresult res;

	if (slEngineObject) return true;
	/* mixer doesn't use any effects */
	ids[0] = SL_IID_NULL; req[0] = SL_BOOLEAN_FALSE;
	
	res = slCreateEngine(&slEngineObject, 0, NULL, 0, NULL, NULL);
	if (res) { Logger_SimpleWarn(res, "creating OpenSL ES engine"); return false; }

	res = (*slEngineObject)->Realize(slEngineObject, SL_BOOLEAN_FALSE);
	if (res) { Logger_SimpleWarn(res, "realising OpenSL ES engine"); return false; }

	res = (*slEngineObject)->GetInterface(slEngineObject, SL_IID_ENGINE, &slEngineEngine);
	if (res) { Logger_SimpleWarn(res, "initing OpenSL ES engine"); return false; }

	res = (*slEngineEngine)->CreateOutputMix(slEngineEngine, &slOutputObject, 1, ids, req);
	if (res) { Logger_SimpleWarn(res, "creating OpenSL ES mixer"); return false; }

	res = (*slOutputObject)->Realize(slOutputObject, SL_BOOLEAN_FALSE);
	if (res) { Logger_SimpleWarn(res, "realising OpenSL ES mixer"); return false; }

	return true;
}

static void Backend_Free(void) {
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
	int i;
	for (i = 0; i < buffers; i++) {
		ctx->available[i] = true;
	}
	ctx->count = buffers;
}

static cc_result Backend_Reset(struct AudioContext* ctx) {
	SLObjectItf bqPlayerObject = ctx->bqPlayerObject;
	if (bqPlayerObject) {
		(*bqPlayerObject)->Destroy(bqPlayerObject);
		ctx->bqPlayerObject = NULL;
		ctx->bqPlayerPlayer = NULL;
		ctx->bqPlayerQueue  = NULL;
	}
	return 0;
}

static void OnBufferFinished(SLBufferQueueItf bq, void* context) {
	struct AudioContext* ctx = (struct AudioContext*)context;
	SLBufferQueueState state = { 0 };
	unsigned int i;
	
	if ((*ctx->bqPlayerQueue)->GetState(ctx->bqPlayerQueue, &state)) return;
	/* At this point, playIndex is for index of NEXT buffer (about to be played) */
	/* So need to subtract 1 from it */
	i = (state.playIndex - 1) % (unsigned int)ctx->count;
	ctx->available[i] = true;
}

static cc_result Backend_SetFormat(struct AudioContext* ctx, struct AudioFormat* format) {
	SLDataLocator_AndroidSimpleBufferQueue input;
	SLDataLocator_OutputMix output;
	SLObjectItf bqPlayerObject;
	SLDataFormat_PCM fmt;
	SLInterfaceID ids[2];
	SLboolean req[2];
	SLDataSource src;
	SLDataSink dst;
	cc_result res;

	fmt.formatType     = SL_DATAFORMAT_PCM;
	fmt.numChannels    = format->channels;
	fmt.samplesPerSec  = format->sampleRate * 1000;
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

	ids[0] = SL_IID_BUFFERQUEUE; req[0] = SL_BOOLEAN_TRUE;
	ids[1] = SL_IID_PLAY;        req[1] = SL_BOOLEAN_TRUE;

	res = (*slEngineEngine)->CreateAudioPlayer(slEngineEngine, &bqPlayerObject, &src, &dst, 2, ids, req);
	ctx->bqPlayerObject = bqPlayerObject;
	if (res) return res;

	if ((res = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE)))                              return res;
	if ((res = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY,        &ctx->bqPlayerPlayer))) return res;
	if ((res = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &ctx->bqPlayerQueue)))  return res;

	return (*ctx->bqPlayerQueue)->RegisterCallback(ctx->bqPlayerQueue, OnBufferFinished, ctx);
}

cc_result Audio_BufferData(struct AudioContext* ctx, int idx, void* data, cc_uint32 size) {
	ctx->available[idx] = false;
	return (*ctx->bqPlayerQueue)->Enqueue(ctx->bqPlayerQueue, data, size);
}

cc_result Audio_Play(struct AudioContext* ctx) {
	return (*ctx->bqPlayerPlayer)->SetPlayState(ctx->bqPlayerPlayer, SL_PLAYSTATE_PLAYING);
}

cc_result Audio_Stop(struct AudioContext* ctx) {
	if (!ctx->bqPlayerPlayer) return 0;

	/* According to OpenSL ES spec, Clear can never fail anyways */
	(*ctx->bqPlayerQueue)->Clear(ctx->bqPlayerQueue);
	return (*ctx->bqPlayerPlayer)->SetPlayState(ctx->bqPlayerPlayer, SL_PLAYSTATE_STOPPED);
}

cc_result Audio_IsAvailable(struct AudioContext* ctx, int idx, cc_bool* available) {
	*available = ctx->available[idx];
	return 0;
}

cc_result Audio_IsFinished(struct AudioContext* ctx, cc_bool* finished) {
	SLBufferQueueState state = { 0 };
	cc_result res;

	if (ctx->bqPlayerQueue) {
		res       = (*ctx->bqPlayerQueue)->GetState(ctx->bqPlayerQueue, &state);
		*finished = state.count == 0;
	} else {
		res       = 0;
		*finished = true;
	}
	return res;
}
#endif

/*########################################################################################################################*
*---------------------------------------------------Common backend code---------------------------------------------------*
*#########################################################################################################################*/
static cc_result Audio_AllAvailable(struct AudioContext* ctx, cc_bool* finished) {
	cc_result res;
	int i;
	*finished = false;

	for (i = 0; i < ctx->count; i++) {
		res = Audio_IsAvailable(ctx, i, finished);
		if (res) return res;
		if (!(*finished)) return 0;
	}

	*finished = true;
	return 0;
}

struct AudioFormat* Audio_GetFormat(struct AudioContext* ctx) { return &ctx->format; }

cc_result Audio_SetFormat(struct AudioContext* ctx, struct AudioFormat* format) {
	struct AudioFormat* cur = &ctx->format;
	cc_result res;

	if (AudioFormat_Eq(cur, format)) return 0;
	if ((res = Backend_Reset(ctx)))  return res;

	ctx->format = *format;
	return Backend_SetFormat(ctx, format);
}

cc_result Audio_Close(struct AudioContext* ctx) {
	cc_bool finished;
	Audio_Stop(ctx);
	Audio_IsFinished(ctx, &finished); /* unqueue buffers */

	ctx->count             = 0;
	ctx->format.channels   = 0;
	ctx->format.sampleRate = 0;
	return Backend_Reset(ctx);
}


/*########################################################################################################################*
*------------------------------------------------------Soundboard---------------------------------------------------------*
*#########################################################################################################################*/
struct Sound {
	struct AudioFormat format;
	cc_uint8* data; cc_uint32 size;
};

#define AUDIO_MAX_SOUNDS 10
struct SoundGroup {
	int count;
	struct Sound sounds[AUDIO_MAX_SOUNDS];
};

struct Soundboard {
	RNGState rnd; cc_bool inited;
	struct SoundGroup groups[SOUND_COUNT];
};

#define WAV_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)
#define WAV_FMT_SIZE 16

static cc_result Sound_ReadWaveData(struct Stream* stream, struct Sound* snd) {
	cc_uint32 fourCC, size;
	cc_uint8 tmp[WAV_FMT_SIZE];
	cc_result res;
	int bitsPerSample;

	if ((res = Stream_Read(stream, tmp, 12)))  return res;
	fourCC = Stream_GetU32_BE(tmp + 0);
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

			snd->format.channels   = Stream_GetU16_LE(tmp + 2);
			snd->format.sampleRate = Stream_GetU32_LE(tmp + 4);
			/* tmp[8] (6) alignment data and stuff */

			bitsPerSample = Stream_GetU16_LE(tmp + 14);
			if (bitsPerSample != 16) return WAV_ERR_SAMPLE_BITS;
			size -= WAV_FMT_SIZE;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->data = (cc_uint8*)Mem_TryAlloc(size, 1);
			snd->size = size;

			if (!snd->data) return ERR_OUT_OF_MEMORY;
			return Stream_Read(stream, snd->data, size);
		}

		/* Skip over unhandled data */
		if (size && (res = stream->Skip(stream, size))) return res;
	}
}

static cc_result Sound_ReadWave(const cc_string* filename, struct Sound* snd) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	struct Stream stream;
	cc_result res;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "audio/%s", filename);

	res = Stream_OpenFile(&stream, &path);
	if (res) return res;

	res = Sound_ReadWaveData(&stream, snd);
	if (res) { stream.Close(&stream); return res; }

	return stream.Close(&stream);
}

static struct SoundGroup* Soundboard_Find(struct Soundboard* board, const cc_string* name) {
	struct SoundGroup* groups = board->groups;
	int i;

	for (i = 0; i < SOUND_COUNT; i++) {
		if (String_CaselessEqualsConst(name, Sound_Names[i])) return &groups[i];
	}
	return NULL;
}

static void Soundboard_Init(struct Soundboard* board, const cc_string* boardName) {
	cc_string file, name;
	struct SoundGroup* group;
	struct Sound* snd;
	cc_result res;
	int i, dotIndex;
	board->inited = true;

	for (i = 0; i < files.count; i++) {
		file = StringsBuffer_UNSAFE_Get(&files, i); 
		name = file;

		/* dig_grass1.wav -> dig_grass1 */
		dotIndex = String_LastIndexOf(&name, '.');
		if (dotIndex >= 0) name.length = dotIndex;
		if (!String_CaselessStarts(&name, boardName)) continue;

		/* Convert dig_grass1 to grass */
		name = String_UNSAFE_SubstringAt(&name, boardName->length);
		name = String_UNSAFE_Substring(&name, 0, name.length - 1);

		group = Soundboard_Find(board, &name);
		if (!group) {
			Chat_Add1("&cUnknown sound group '%s'", &name); continue;
		}
		if (group->count == Array_Elems(group->sounds)) {
			Chat_AddRaw("&cCannot have more than 10 sounds in a group"); continue;
		}

		snd = &group->sounds[group->count];
		res = Sound_ReadWave(&file, snd);

		if (res) {
			Logger_SysWarn2(res, "decoding", &file);
			Mem_Free(snd->data);
			snd->data = NULL;
			snd->size = 0;
		} else { group->count++; }
	}
}

static struct Sound* Soundboard_PickRandom(struct Soundboard* board, cc_uint8 type) {
	struct SoundGroup* group;
	int idx;

	if (type == SOUND_NONE || type >= SOUND_COUNT) return NULL;
	if (type == SOUND_METAL) type = SOUND_STONE;

	group = &board->groups[type];
	if (!group->count) return NULL;

	idx = Random_Next(&board->rnd, group->count);
	return &group->sounds[idx];
}


/*########################################################################################################################*
*--------------------------------------------------------Sounds-----------------------------------------------------------*
*#########################################################################################################################*/
struct SoundOutput { struct AudioContext* ctx; void* buffer; cc_uint32 capacity; };
static struct Soundboard digBoard, stepBoard;
#define AUDIO_MAX_HANDLES 6

static struct SoundOutput monoOutputs[AUDIO_MAX_HANDLES];
static struct SoundOutput stereoOutputs[AUDIO_MAX_HANDLES];
static struct AudioContext soundContexts[AUDIO_MAX_HANDLES * 2];

static struct AudioContext* Sounds_Open(void) {
	struct AudioContext* ctx;
	int i;

	for (i = 0; i < Array_Elems(soundContexts); i++) {
		ctx = &soundContexts[i];
		if (ctx->count) continue;

		Audio_Init(ctx, 1);
		return ctx;
	}

	/* Should never happen */
	Logger_Abort("No free audio contexts");
	return NULL;
}

CC_NOINLINE static void Sounds_Fail(cc_result res) {
	Logger_SimpleWarn(res, "playing sounds");
	Chat_AddRaw("&cDisabling sounds");
	Audio_SetSounds(0);
}

static void Sounds_PlayRaw(struct SoundOutput* output, struct Sound* snd, struct AudioFormat* fmt, int volume) {
	void* data = snd->data;
	void* tmp;
	cc_result res;
	if ((res = Audio_SetFormat(output->ctx, fmt))) { Sounds_Fail(res); return; }
	
	/* copy to temp buffer to apply volume */
	if (volume < 100) {
		/* TODO: Don't need a per sound temp buffer, just a global one */
		if (output->capacity < snd->size) {
			/* TODO: check if we can realloc NULL without a problem */
			if (output->buffer) {
				tmp = Mem_TryRealloc(output->buffer, snd->size, 1);
			} else {
				tmp = Mem_TryAlloc(snd->size, 1);
			}

			if (!tmp) { Sounds_Fail(ERR_OUT_OF_MEMORY); return; }
			output->buffer   = tmp;
			output->capacity = snd->size;
		}
		data = output->buffer;

		Mem_Copy(data, snd->data, snd->size);
		Volume_Mix16((cc_int16*)data, snd->size / 2, volume);
	}

	if ((res = Audio_BufferData(output->ctx, 0, data, snd->size))) { Sounds_Fail(res); return; }
	if ((res = Audio_Play(output->ctx)))                           { Sounds_Fail(res); return; }
}

static void Sounds_Play(cc_uint8 type, struct Soundboard* board) {
	struct Sound* snd;
	struct AudioFormat  fmt;
	struct SoundOutput* outputs;
	struct SoundOutput* output;
	struct AudioFormat* l;

	cc_bool finished;
	int i, volume;
	cc_result res;

	if (type == SOUND_NONE || !Audio_SoundsVolume) return;
	snd = Soundboard_PickRandom(board, type);

	if (!snd) return;
	if (!Backend_Init()) { Backend_Free(); Audio_SoundsVolume = 0; return; }

	fmt     = snd->format;
	volume  = Audio_SoundsVolume;
	outputs = fmt.channels == 1 ? monoOutputs : stereoOutputs;

	if (board == &digBoard) {
		if (type == SOUND_METAL) fmt.sampleRate = (fmt.sampleRate * 6) / 5;
		else fmt.sampleRate = (fmt.sampleRate * 4) / 5;
	} else {
		volume /= 2;
		if (type == SOUND_METAL) fmt.sampleRate = (fmt.sampleRate * 7) / 5;
	}

	/* Try to play on fresh device, or device with same data format */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		output = &outputs[i];
		if (!output->ctx) {
			output->ctx = Sounds_Open();
		} else {
			res = Audio_IsFinished(output->ctx, &finished);

			if (res) { Sounds_Fail(res); return; }
			if (!finished) continue;
		}

		l = Audio_GetFormat(output->ctx);
		if (!l->channels || AudioFormat_Eq(l, &fmt)) {
			Sounds_PlayRaw(output, snd, &fmt, volume); return;
		}
	}

	/* Try again with all devices, even if need to recreate one (expensive) */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		output = &outputs[i];
		res = Audio_IsFinished(output->ctx, &finished);

		if (res) { Sounds_Fail(res); return; }
		if (!finished) continue;

		Sounds_PlayRaw(output, snd, &fmt, volume); return;
	}
}

static void Audio_PlayBlockSound(void* obj, IVec3 coords, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		Audio_PlayDigSound(Blocks.DigSounds[old]);
	} else if (!Game_ClassicMode) {
		Audio_PlayDigSound(Blocks.StepSounds[now]);
	}
}

static void Sounds_FreeOutputs(struct SoundOutput* outputs) {
	int i;
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		if (!outputs[i].ctx) continue;

		Audio_Close(outputs[i].ctx);
		outputs[i].ctx = NULL;

		Mem_Free(outputs[i].buffer);
		outputs[i].buffer   = NULL;
		outputs[i].capacity = 0;
	}
}

static void Sounds_Init(void) {
	static const cc_string dig  = String_FromConst("dig_");
	static const cc_string step = String_FromConst("step_");

	if (digBoard.inited || stepBoard.inited) return;
	Soundboard_Init(&digBoard,  &dig);
	Soundboard_Init(&stepBoard, &step);
}

static void Sounds_Free(void) {
	Sounds_FreeOutputs(monoOutputs);
	Sounds_FreeOutputs(stereoOutputs);
}

void Audio_SetSounds(int volume) {
	if (volume) Sounds_Init();
	else        Sounds_Free();
	Audio_SoundsVolume = volume;
}

void Audio_PlayDigSound(cc_uint8 type)  { Sounds_Play(type, &digBoard); }
void Audio_PlayStepSound(cc_uint8 type) { Sounds_Play(type, &stepBoard); }


/*########################################################################################################################*
*--------------------------------------------------------Music------------------------------------------------------------*
*#########################################################################################################################*/
static struct AudioContext music_ctx;
static void* music_thread;
static void* music_waitable;
static volatile cc_bool music_pendingStop, music_joining;
static int music_minDelay, music_maxDelay;

static cc_result Music_Buffer(int i, cc_int16* data, int maxSamples, struct VorbisState* ctx) {
	int samples = 0;
	cc_int16* cur;
	cc_result res = 0, res2;

	while (samples < maxSamples) {
		if ((res = Vorbis_DecodeFrame(ctx))) break;

		cur = &data[samples];
		samples += Vorbis_OutputFrame(ctx, cur);
	}
	if (Audio_MusicVolume < 100) { Volume_Mix16(data, samples, Audio_MusicVolume); }
	#ifdef CC_BUILD_ANDROID
    /* Don't play music while in the background on Android */
    /* TODO: Not use such a terrible approach */
    if (!WindowInfo.Handle) { int i; for (i = 0; i < samples; i++) data[i] = 0; }
    #endif

	res2 = Audio_BufferData(&music_ctx, i, data, samples * 2);
	if (res2) { music_pendingStop = true; return res2; }
	return res;
}

static cc_result Music_PlayOgg(struct Stream* source) {
	struct OggState ogg;
	struct VorbisState vorbis = { 0 };
	struct AudioFormat fmt;

	int chunkSize, samplesPerSecond;
	cc_bool available, finished;
	cc_int16* data = NULL;
	int i, next;
	cc_result res;

	Ogg_Init(&ogg, source);
	vorbis.source = &ogg;
	if ((res = Vorbis_DecodeHeaders(&vorbis))) goto cleanup;
	
	fmt.channels   = vorbis.channels;
	fmt.sampleRate = vorbis.sampleRate;
	if ((res = Audio_SetFormat(&music_ctx, &fmt))) goto cleanup;

	/* largest possible vorbis frame decodes to blocksize1 * channels samples */
	/* so we may end up decoding slightly over a second of audio */
	chunkSize        = fmt.channels * (fmt.sampleRate + vorbis.blockSizes[1]);
	samplesPerSecond = fmt.channels * fmt.sampleRate;

	data = (cc_int16*)Mem_TryAlloc(chunkSize * AUDIO_MAX_BUFFERS, 2);
	if (!data) { res = ERR_OUT_OF_MEMORY; goto cleanup; }

	/* fill up with some samples before playing */
	for (i = 0; i < AUDIO_MAX_BUFFERS && !res; i++) {
		res = Music_Buffer(i, &data[chunkSize * i], samplesPerSecond, &vorbis);
	}
	if (music_pendingStop) goto cleanup;

	res = Audio_Play(&music_ctx);
	if (res) goto cleanup;

	for (;;) {
		next = -1;
		
		for (i = 0; i < AUDIO_MAX_BUFFERS; i++) {
			res = Audio_IsAvailable(&music_ctx, i, &available);
			if (res)       { music_pendingStop = true; break; }
			if (available) { next = i; break; }
		}

		if (music_pendingStop) break;
		if (next == -1) { Thread_Sleep(10); continue; }

		res = Music_Buffer(next, &data[chunkSize * next], samplesPerSecond, &vorbis);
		/* need to specially handle last bit of audio */
		if (res) break;
	}

	if (music_pendingStop) Audio_Stop(&music_ctx);
	/* Wait until the buffers finished playing */
	for (;;) {
		if (Audio_IsFinished(&music_ctx, &finished) || finished) break;
		Thread_Sleep(10);
	}

cleanup:
	Mem_Free(data);
	Vorbis_Free(&vorbis);
	return res == ERR_END_OF_STREAM ? 0 : res;
}

#define MUSIC_MAX_FILES 512
static void Music_RunLoop(void) {
	static const cc_string ogg = String_FromConst(".ogg");
	char pathBuffer[FILENAME_SIZE];
	cc_string path;

	unsigned short musicFiles[MUSIC_MAX_FILES];
	cc_string file;

	RNGState rnd;
	struct Stream stream;
	int i, count = 0, idx, delay;
	cc_result res = 0;

	for (i = 0; i < files.count && count < MUSIC_MAX_FILES; i++) {
		file = StringsBuffer_UNSAFE_Get(&files, i);
		if (!String_CaselessEnds(&file, &ogg)) continue;
		musicFiles[count++] = i;
	}

	Random_SeedFromCurrentTime(&rnd);
	Audio_Init(&music_ctx, AUDIO_MAX_BUFFERS);

	while (!music_pendingStop && count) {
		idx  = Random_Next(&rnd, count);
		file = StringsBuffer_UNSAFE_Get(&files, musicFiles[idx]);
		
		String_InitArray(path, pathBuffer);
		String_Format1(&path, "audio/%s", &file);
		Platform_Log1("playing music file: %s", &file);

		res = Stream_OpenFile(&stream, &path);
		if (res) { Logger_SysWarn2(res, "opening", &path); break; }

		res = Music_PlayOgg(&stream);
		if (res) { 
			Logger_SimpleWarn2(res, "playing", &path); 
			stream.Close(&stream); break;
		}

		res = stream.Close(&stream);
		if (res) { Logger_SysWarn2(res, "closing", &path); break; }

		if (music_pendingStop) break;
		delay = Random_Range(&rnd, music_minDelay, music_maxDelay);
		Waitable_WaitFor(music_waitable, delay);
	}

	if (res) {
		Chat_AddRaw("&cDisabling music");
		Audio_MusicVolume = 0;
	}
	Audio_Close(&music_ctx);

	if (music_joining) return;
	Thread_Detach(music_thread);
	music_thread = NULL;
}

static void Music_Init(void) {
	if (music_thread) return;
	if (!Backend_Init()) { Backend_Free(); Audio_MusicVolume = 0; return; }

	music_joining     = false;
	music_pendingStop = false;

	music_thread = Thread_Start(Music_RunLoop);
}

static void Music_Free(void) {
	music_joining     = true;
	music_pendingStop = true;
	Waitable_Signal(music_waitable);
	
	if (music_thread) Thread_Join(music_thread);
	music_thread = NULL;
}

void Audio_SetMusic(int volume) {
	Audio_MusicVolume = volume;
	if (volume) Music_Init();
	else        Music_Free();
}


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
static int Audio_LoadVolume(const char* volKey, const char* boolKey) {
	int volume = Options_GetInt(volKey, 0, 100, 0);
	if (volume) return volume;

	volume = Options_GetBool(boolKey, false) ? 100 : 0;
	Options_Set(boolKey, NULL);
	return volume;
}

static void Audio_FilesCallback(const cc_string* path, void* obj) {
	cc_string relPath = *path;
	Utils_UNSAFE_TrimFirstDirectory(&relPath);
	StringsBuffer_Add(&files, &relPath);
}

static void OnInit(void) {
	static const cc_string path = String_FromConst("audio");
	int volume;

	Directory_Enum(&path, NULL, Audio_FilesCallback);
	music_waitable = Waitable_Create();

	/* music is delayed between 2 - 7 minutes by default */
	music_minDelay = Options_GetInt(OPT_MIN_MUSIC_DELAY, 0, 3600, 120) * MILLIS_PER_SEC;
	music_maxDelay = Options_GetInt(OPT_MAX_MUSIC_DELAY, 0, 3600, 420) * MILLIS_PER_SEC;

	volume = Audio_LoadVolume(OPT_MUSIC_VOLUME, OPT_USE_MUSIC);
	Audio_SetMusic(volume);
	volume = Audio_LoadVolume(OPT_SOUND_VOLUME, OPT_USE_SOUND);
	Audio_SetSounds(volume);
	Event_Register_(&UserEvents.BlockChanged, NULL, Audio_PlayBlockSound);
}

static void OnFree(void) {
	Music_Free();
	Sounds_Free();
	Waitable_Free(music_waitable);
	Backend_Free();
}
#endif

struct IGameComponent Audio_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
