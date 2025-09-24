#include <sndcore2/core.h>

struct AudioContext { int count; };

#include "../_AudioBase.h"
static cc_bool ax_inited;

cc_bool AudioBackend_Init(void) {
	if (ax_inited) return true;
	ax_inited = true;

	AXInit();
	return true;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) {
	AXQuit();
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->count = buffers;
	return ERR_NOT_SUPPORTED;
}

void Audio_Close(struct AudioContext* ctx) {
	ctx->count = 0; // TODO
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	// TODO
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	// TODO
	return ERR_NOT_SUPPORTED;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	// TODO
	return ERR_NOT_SUPPORTED;
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
	/* Channels/Sample rate is per buffer, not a per source property */
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

