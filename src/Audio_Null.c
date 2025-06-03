#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_NULL
struct AudioContext { int count; };
#include "_AudioBase.h"

cc_bool AudioBackend_Init(void) { return false; }
void    AudioBackend_Tick(void) { }
void    AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	return ERR_NOT_SUPPORTED;
}

void Audio_Close(struct AudioContext* ctx) { }

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	return ERR_NOT_SUPPORTED;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) { }

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	return ERR_NOT_SUPPORTED;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	return ERR_NOT_SUPPORTED;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	return ERR_NOT_SUPPORTED;
}

static cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) { return false; }

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) { return false; }

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	return ERR_NOT_SUPPORTED;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) { }
#endif

