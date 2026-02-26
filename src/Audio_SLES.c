#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_OPENSLES
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "ExtMath.h"
#include "Funcs.h"

static SLObjectItf slEngineObject;
static SLEngineItf slEngineEngine;
static SLObjectItf slOutputObject;

struct AudioContext {
	int count, volume;
	int channels, sampleRate;
	SLObjectItf       playerObject;
	SLPlayItf         playerPlayer;
	SLBufferQueueItf  playerQueue;
	SLPlaybackRateItf playerRate;
	SLVolumeItf       playerVolume;
};

#include "_AudioBase.h"

static SLresult (SLAPIENTRY *_slCreateEngine)(SLObjectItf* engine, SLuint32 numOptions, const SLEngineOption* engineOptions,
							SLuint32 numInterfaces, const SLInterfaceID* interfaceIds, const SLboolean* interfaceRequired);
static SLInterfaceID* _SL_IID_NULL;
static SLInterfaceID* _SL_IID_PLAY;
static SLInterfaceID* _SL_IID_ENGINE;
static SLInterfaceID* _SL_IID_BUFFERQUEUE;
static SLInterfaceID* _SL_IID_PLAYBACKRATE;
static SLInterfaceID* _SL_IID_VOLUME;

static const cc_string slLib = String_FromConst("libOpenSLES.so");

static cc_bool LoadSLFuncs(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_ReqSym(slCreateEngine),     DynamicLib_ReqSym(SL_IID_NULL),
		DynamicLib_ReqSym(SL_IID_PLAY),        DynamicLib_ReqSym(SL_IID_ENGINE),
		DynamicLib_ReqSym(SL_IID_BUFFERQUEUE), DynamicLib_ReqSym(SL_IID_PLAYBACKRATE),
		DynamicLib_ReqSym(SL_IID_VOLUME)
	};
	void* lib;
	
	return DynamicLib_LoadAll(&slLib, funcs, Array_Elems(funcs), &lib);
}

cc_bool AudioBackend_Init(void) {
	static const cc_string msg = String_FromConst("Failed to init OpenSLES. No audio will play.");
	SLInterfaceID ids[1];
	SLboolean req[1];
	SLresult res;

	if (slEngineObject) return true;
	if (!LoadSLFuncs()) { Logger_WarnFunc(&msg); return false; }
	
	/* mixer doesn't use any effects */
	ids[0] = *_SL_IID_NULL; req[0] = SL_BOOLEAN_FALSE;
	
	res = _slCreateEngine(&slEngineObject, 0, NULL, 0, NULL, NULL);
	if (res) { Audio_Warn(res, "creating OpenSL ES engine"); return false; }

	res = (*slEngineObject)->Realize(slEngineObject, SL_BOOLEAN_FALSE);
	if (res) { Audio_Warn(res, "realising OpenSL ES engine"); return false; }

	res = (*slEngineObject)->GetInterface(slEngineObject, *_SL_IID_ENGINE, &slEngineEngine);
	if (res) { Audio_Warn(res, "initing OpenSL ES engine"); return false; }

	res = (*slEngineEngine)->CreateOutputMix(slEngineEngine, &slOutputObject, 1, ids, req);
	if (res) { Audio_Warn(res, "creating OpenSL ES mixer"); return false; }

	res = (*slOutputObject)->Realize(slOutputObject, SL_BOOLEAN_FALSE);
	if (res) { Audio_Warn(res, "realising OpenSL ES mixer"); return false; }

	return true;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) {
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

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->count  = buffers;
	ctx->volume = 100;
	return 0;
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
	ctx->playerVolume = NULL;
}

void Audio_Close(struct AudioContext* ctx) {
	Audio_Stop(ctx);
	Audio_Reset(ctx);

	ctx->count      = 0;
	ctx->channels   = 0;
	ctx->sampleRate = 0;
}

static float Log10(float volume) { return Math_Log2(volume) / Math_Log2(10); }

static void UpdateVolume(struct AudioContext* ctx) {
	/* Object doesn't exist until Audio_SetFormat is called */
	if (!ctx->playerVolume) return;
	
	/* log of 0 is undefined */
	SLmillibel attenuation = ctx->volume == 0 ? SL_MILLIBEL_MIN : (2000 * Log10(ctx->volume / 100.0f));
	(*ctx->playerVolume)->SetVolumeLevel(ctx->playerVolume, attenuation);
}

static cc_result RecreatePlayer(struct AudioContext* ctx, int channels, int sampleRate) {
	SLDataLocator_AndroidSimpleBufferQueue input;
	SLDataLocator_OutputMix output;
	SLObjectItf playerObject;
	SLDataFormat_PCM fmt;
	SLInterfaceID ids[4];
	SLboolean req[4];
	SLDataSource src;
	SLDataSink dst;
	cc_result res;

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
	ids[3] = *_SL_IID_VOLUME;       req[3] = SL_BOOLEAN_TRUE;

	res = (*slEngineEngine)->CreateAudioPlayer(slEngineEngine, &playerObject, &src, &dst, 4, ids, req);
	ctx->playerObject = playerObject;
	if (res) return res;

	if ((res = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE)))                               return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_PLAY,         &ctx->playerPlayer))) return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_BUFFERQUEUE,  &ctx->playerQueue)))  return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_PLAYBACKRATE, &ctx->playerRate)))   return res;
	if ((res = (*playerObject)->GetInterface(playerObject, *_SL_IID_VOLUME,       &ctx->playerVolume))) return res;

	UpdateVolume(ctx);
	return 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	cc_result res;

	if (ctx->channels != channels || ctx->sampleRate != sampleRate) {
		if ((res = RecreatePlayer(ctx, channels, sampleRate))) return res;
	}

	/* rate is in milli, so 1000 = normal rate */
	return (*ctx->playerRate)->SetRate(ctx->playerRate, playbackRate * 10);
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	ctx->volume = volume;
	UpdateVolume(ctx);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	return (*ctx->playerQueue)->Enqueue(ctx->playerQueue, chunk->data, chunk->size);
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
	return (*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_PLAYING);
}

cc_result StreamContext_Pause(struct AudioContext* ctx) {
	return (*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_PAUSED);
}

cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
	return Audio_Poll(ctx, inUse);
}


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return !ctx->channels || (ctx->channels == data->channels && ctx->sampleRate == data->sampleRate);
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    cc_result res;

	if ((res = Audio_SetFormat(ctx,  data->channels, data->sampleRate, data->rate))) return res;
	if ((res = Audio_QueueChunk(ctx, &data->chunk))) return res;

	return (*ctx->playerPlayer)->SetPlayState(ctx->playerPlayer, SL_PLAYSTATE_PLAYING);
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
#endif

