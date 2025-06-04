#include "Core.h"

#if defined CC_BUILD_WEBAUDIO
struct AudioContext { int contextID, count, rate; void* data; };

#define AUDIO_OVERRIDE_ALLOC
#include "_AudioBase.h"

extern int  interop_InitAudio(void);
extern int  interop_AudioCreate(void);
extern void interop_AudioClose(int contextID);
extern int  interop_AudioPlay(int contextID, const void* name, int rate);
extern int  interop_AudioPoll(int contextID, int* inUse);
extern int  interop_AudioVolume(int contextID, int volume);
extern int  interop_AudioDescribe(int res, char* buffer, int bufferLen);

cc_bool AudioBackend_Init(void) {
	cc_result res = interop_InitAudio();
	if (res) { Audio_Warn(res, "initing WebAudio context"); return false; }
	return true;
}

void AudioBackend_Tick(void) { }
void AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->count     = buffers;
	ctx->contextID = interop_AudioCreate();
	ctx->data      = NULL;
	ctx->rate      = 100;
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->contextID) interop_AudioClose(ctx->contextID);
	ctx->contextID = 0;
	ctx->count     = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	ctx->rate = playbackRate; return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	interop_AudioVolume(ctx->contextID, volume);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	ctx->data = chunk->data; return 0;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	return interop_AudioPlay(ctx->contextID, ctx->data, ctx->rate);
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	return interop_AudioPoll(ctx->contextID, inUse);
}

static cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	/* Channels/Sample rate is per buffer, not a per source property */
	return true;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	char buffer[NATIVE_STR_LEN];
	int len = interop_AudioDescribe(res, buffer, NATIVE_STR_LEN);

	String_AppendUtf8(dst, buffer, len);
	return len > 0;
}
#endif

