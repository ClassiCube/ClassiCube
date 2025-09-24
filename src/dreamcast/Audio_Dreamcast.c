#include <kos.h>
#include "../Audio.h"

/* TODO needs way more testing, especially with sounds */
#define HANDLE_STATE_UNUSED    0
#define HANDLE_STATE_ALLOCATED 1
#define HANDLE_STATE_PLAYABLE  2
// Need to manually track playable state, to avoid triggering divide by zero when 0 channels in poll
// https://github.com/KallistiOS/KallistiOS/pull/1099 - but manually check this to support older toolchains
static cc_uint8 valid_handles[SND_STREAM_MAX];

struct AudioBuffer {
	int available;
	int bytesLeft;
	void* samples;
};

struct AudioContext {
	int bufHead, channels;
	snd_stream_hnd_t hnd;
	struct AudioBuffer bufs[AUDIO_MAX_BUFFERS];
	int count, sampleRate;
};
#define AUDIO_OVERRIDE_ALLOC
#include "../_AudioBase.h"
#include "../Funcs.h"

cc_bool AudioBackend_Init(void) {
	return snd_stream_init() == 0;
}

void AudioBackend_Tick(void) {
	// TODO is this really threadsafe with music? should this be done in Audio_Poll instead?
	for (int i = 0; i < SND_STREAM_MAX; i++)
	{
		if (valid_handles[i] == HANDLE_STATE_PLAYABLE) snd_stream_poll(i);
	}
}

void AudioBackend_Free(void) {
	snd_stream_shutdown();
}

static void* AudioCallback(snd_stream_hnd_t hnd, int smp_req, int *smp_recv) {
	struct AudioContext* ctx = snd_stream_get_userdata(hnd);
	struct AudioBuffer* buf  = &ctx->bufs[ctx->bufHead];
	
	int samples = min(buf->bytesLeft, smp_req);
	*smp_recv   = samples;
	void* ptr   = buf->samples;
	
	buf->samples   += samples;
	buf->bytesLeft -= samples;
	
	if (buf->bytesLeft == 0) {
		ctx->bufHead   = (ctx->bufHead + 1) % ctx->count;
		buf->samples   = NULL;
		buf->available = true;

		// special case to fix sounds looping
		if (samples == 0 && ptr == NULL) *smp_recv = smp_req;
	}
	return ptr;
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->hnd = snd_stream_alloc(AudioCallback, SND_STREAM_BUFFER_MAX);
	if (ctx->hnd == SND_STREAM_INVALID) return ERR_NOT_SUPPORTED;
	snd_stream_set_userdata(ctx->hnd, ctx);
	
	Mem_Set(ctx->bufs, 0, sizeof(ctx->bufs));
	for (int i = 0; i < buffers; i++) {
		ctx->bufs[i].available = true;
	}
	
	ctx->count   = buffers;
	ctx->bufHead = 0;
	valid_handles[ctx->hnd] = HANDLE_STATE_ALLOCATED;
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->count) {
		snd_stream_stop(ctx->hnd);
		snd_stream_destroy(ctx->hnd);
		valid_handles[ctx->hnd] = HANDLE_STATE_UNUSED;
	}
	
	ctx->hnd   = SND_STREAM_INVALID;
	ctx->count = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;
	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	snd_stream_volume(ctx->hnd, volume);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	struct AudioBuffer* buf;

	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		if (!buf->available) continue;

		buf->samples   = chunk->data;
		buf->bytesLeft = chunk->size;
		buf->available = false;
		return 0;
	}
	// tried to queue data without polling for free buffers first
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	snd_stream_start(ctx->hnd, ctx->sampleRate, ctx->channels == 2);
	valid_handles[ctx->hnd] = HANDLE_STATE_PLAYABLE;
	return 0;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	struct AudioBuffer* buf;
	int count = 0;

	for (int i = 0; i < ctx->count; i++)
	{
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

static int totalSize;
cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	size = (size + 0x1F) & ~0x1F;  // round up to nearest multiple of 32
	void* dst = memalign(32, size * numChunks);
	if (!dst) return ERR_OUT_OF_MEMORY;
	totalSize += size * numChunks;
	//Platform_Log3("ALLOC: %i X %i (%i)", &size, &numChunks, &totalSize);

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

