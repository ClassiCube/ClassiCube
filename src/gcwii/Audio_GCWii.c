#include "../Audio.h"

#include <asndlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

struct AudioBuffer {
	int available;
	int size;
	void* samples;
};

struct AudioContext {
	int chanID, count, bufHead;
	struct AudioBuffer bufs[AUDIO_MAX_BUFFERS];
	int channels, sampleRate, volume;
	cc_bool makeAvailable;
};
#define AUDIO_OVERRIDE_ALLOC
#include "../_AudioBase.h"

cc_bool AudioBackend_Init(void) {
	ASND_Init();
	ASND_Pause(0);
	return true;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) {
	ASND_Pause(1);
	ASND_End();
}

void MusicCallback(s32 voice) {
	struct AudioContext* ctx = &music_ctx;
	struct AudioBuffer* nextBuf  = &ctx->bufs[(ctx->bufHead + 1) % ctx->count];

	if (ASND_StatusVoice(voice) != SND_WORKING) return;

	if (ASND_AddVoice(voice, nextBuf->samples, nextBuf->size) == SND_OK) {
		ctx->bufHead   = (ctx->bufHead + 1) % ctx->count;
		if (ctx->bufHead == 2) ctx->makeAvailable = true;
		if (ctx->makeAvailable) {
			int prev = ctx->bufHead - 2;
			if (prev < 0) prev += 4;
			ctx->bufs[prev].available = true;
		}
	}

	int inUse;
	Audio_Poll(ctx, &inUse);
	if (!inUse) {
		// music has finished, stop the voice so this function isn't called anymore
		ASND_StopVoice(ctx->chanID);
	}
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->chanID        = -1;
	ctx->count         = buffers;
	ctx->volume        = MAX_VOLUME;
	ctx->bufHead       = 0;
	ctx->makeAvailable = false;

	Mem_Set(ctx->bufs, 0, sizeof(ctx->bufs));
	for (int i = 0; i < buffers; i++) {
		ctx->bufs[i].available = true;
	}

	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->chanID != -1) ASND_StopVoice(ctx->chanID);
	ctx->chanID = -1;
	ctx->count  = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;
	ctx->chanID     = ASND_GetFirstUnusedVoice();

	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	ctx->volume = (volume / 100.0f) * MAX_VOLUME;
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	// Audio buffers must be aligned and padded to a multiple of 32 bytes
	if (((uintptr_t)chunk->data & 0x1F) != 0) {
		Platform_Log1("Audio_QueueData: tried to queue buffer with non-aligned audio buffer 0x%x\n", &chunk->data);
	}

	struct AudioBuffer* buf;

	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		if (!buf->available) continue;

		buf->samples   = chunk->data;
		buf->size      = chunk->size;
		buf->available = false;

		return 0;
	}

	// tried to queue data without polling for free buffers first
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	int format = (ctx->channels == 2) ? VOICE_STEREO_16BIT : VOICE_MONO_16BIT;
	cc_bool music = ctx->count > 1;

	ASND_SetVoice(ctx->chanID, format, ctx->sampleRate, 0, ctx->bufs[0].samples, ctx->bufs[0].size, 
					ctx->volume, ctx->volume, music ? MusicCallback : NULL);
	if (!music) ctx->bufs[0].available = true;

	return 0;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	struct AudioBuffer* buf;
	int count = 0;

	for (int i = 0; i < ctx->count; i++) {
		buf = &ctx->bufs[i];
		if (!buf->available) count++;
	}

	*inUse = count;
	return 0;
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
	return Audio_Play(ctx);
}

cc_result StreamContext_Pause(struct AudioContext* ctx) {
	return ERR_NOT_SUPPORTED;
}

cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
	return Audio_Poll(ctx, inUse);
}


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return true;
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    cc_result res;

	if ((res = Audio_SetFormat(ctx,  data->channels, data->sampleRate, data->rate))) return res;
	if ((res = Audio_QueueChunk(ctx, &data->chunk))) return res;
	if ((res = Audio_Play(ctx))) return res;

	return 0;
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
cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	size = (size + 0x1F) & ~0x1F; // round up to nearest multiple of 0x20
	void* dst = memalign(0x20, size * numChunks);
	if (!dst) return ERR_OUT_OF_MEMORY;

	for (int i = 0; i < numChunks; i++)
	{
		chunks[i].data = dst + size * i;
		chunks[i].size = size;
	}
	return 0;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) {
	free(chunks[0].data);
}

