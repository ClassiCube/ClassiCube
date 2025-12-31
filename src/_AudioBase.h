#include "Audio.h"
#include "String_.h"
#include "Logger.h"
#include "Errors.h"
#include "Utils.h"
#include "Platform.h"

void Audio_Warn(cc_result res, const char* action) {
	Logger_Warn(res, action, Audio_DescribeError);
}

/* achieve higher speed by playing samples at higher sample rate */
#define Audio_AdjustSampleRate(sampleRate, playbackRate) ((sampleRate * playbackRate) / 100)

static cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate);
static cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk);
static cc_result Audio_Play(struct AudioContext* ctx);
static cc_result Audio_Poll(struct AudioContext* ctx, int* inUse);


/*########################################################################################################################*
*---------------------------------------------------Common backend code---------------------------------------------------*
*#########################################################################################################################*/

#ifdef AUDIO_COMMON_VOLUME
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

static void AudioBase_Clear(struct AudioContext* ctx) {
	int i;
	ctx->count      = 0;
	ctx->channels   = 0;
	ctx->sampleRate = 0;
	
	for (i = 0; i < AUDIO_MAX_BUFFERS; i++)
	{
		Mem_Free(ctx->_tmpData[i]);
		ctx->_tmpData[i] = NULL;
		ctx->_tmpSize[i] = 0;
	}
}

static cc_bool AudioBase_AdjustSound(struct AudioContext* ctx, int i, struct AudioChunk* chunk) {
	void* audio;
	cc_uint32 src_size = chunk->size;
	if (ctx->volume >= 100) return true;

	/* copy to temp buffer to apply volume */
	if (ctx->_tmpSize[i] < src_size) {
		/* TODO: check if we can realloc NULL without a problem */
		if (ctx->_tmpData[i]) {
			audio = Mem_TryRealloc(ctx->_tmpData[i], src_size, 1);
		} else {
			audio = Mem_TryAlloc(src_size, 1);
		}

		if (!audio) return false;
		ctx->_tmpData[i] = audio;
		ctx->_tmpSize[i] = src_size;
	}

	audio = ctx->_tmpData[i];
	Mem_Copy(audio, chunk->data, src_size);
	ApplyVolume((cc_int16*)audio, src_size / 2, ctx->volume);

	chunk->data = audio;
	return true;
}
#endif

#ifndef AUDIO_OVERRIDE_ALLOC
cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	cc_uint8* dst = (cc_uint8*)Mem_TryAlloc(numChunks, size);
	int i;
	if (!dst) return ERR_OUT_OF_MEMORY;
	
	for (i = 0; i < numChunks; i++)
	{
		chunks[i].data = dst + size * i;
		chunks[i].size = size;
	}
	return 0;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) {
	Mem_Free(chunks[0].data);
}
#endif


#ifndef AUDIO_OVERRIDE_SOUNDS
void AudioBackend_LoadSounds(void) { Sounds_LoadDefault(); }
#endif


/*########################################################################################################################*
*---------------------------------------------------Audio context code----------------------------------------------------*
*#########################################################################################################################*/
struct AudioContext music_ctx;
#ifndef POOL_MAX_CONTEXTS
#define POOL_MAX_CONTEXTS 8
#endif
static struct AudioContext context_pool[POOL_MAX_CONTEXTS];

#ifndef CC_BUILD_NOSOUNDS
cc_result AudioPool_Play(struct AudioData* data) {
	struct AudioContext* ctx;
	cc_bool isBusy;
	cc_result res;
	int i;

	/* Try to play on a context that doesn't need to be recreated */
	for (i = 0; i < POOL_MAX_CONTEXTS; i++) 
	{
		ctx = &context_pool[i];
		if (!ctx->count && (res = Audio_Init(ctx, 1))) return res;

		if ((res = SoundContext_PollBusy(ctx, &isBusy))) return res;
		if (isBusy) continue;
		if (!SoundContext_FastPlay(ctx, data)) continue;

		Audio_SetVolume(ctx, data->volume);
		return SoundContext_PlayData(ctx, data);
	}

	/* Try again with all contexts, even if need to recreate one (expensive) */
	for (i = 0; i < POOL_MAX_CONTEXTS; i++) 
	{
		ctx = &context_pool[i];
		res = SoundContext_PollBusy(ctx, &isBusy);

		if (res) return res;
		if (isBusy) continue;

		Audio_SetVolume(ctx, data->volume);
		return SoundContext_PlayData(ctx, data);
	}
	return 0;
}

void AudioPool_Close(void) {
	int i;
	for (i = 0; i < POOL_MAX_CONTEXTS; i++)
	{
		Audio_Close(&context_pool[i]);
	}
}
#endif
