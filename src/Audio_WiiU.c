#include "Core.h"

#if defined CC_BUILD_WIIU
#include <sndcore2/core.h>
static cc_bool ax_inited;

struct AudioContext { int count; };

#include "_AudioBase.h"

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

static cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	/* Channels/Sample rate is per buffer, not a per source property */
	return true;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	return false;
}
#endif

