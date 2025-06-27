#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_NULL
struct AudioContext { int count; };

#define AUDIO_OVERRIDE_SOUNDS
#define AUDIO_OVERRIDE_ALLOC
#include "_AudioBase.h"

cc_bool AudioBackend_Init(void) { return false; }
void    AudioBackend_Tick(void) { }
void    AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	return ERR_NOT_SUPPORTED;
}

void Audio_Close(struct AudioContext* ctx) { }

void Audio_SetVolume(struct AudioContext* ctx, int volume) { }


/*########################################################################################################################*
*------------------------------------------------------Stream context-----------------------------------------------------*
*#########################################################################################################################*/
cc_result StreamContext_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	return ERR_NOT_SUPPORTED;
}

cc_result StreamContext_Enqueue(struct AudioContext* ctx, struct AudioChunk* chunk) {
	return ERR_NOT_SUPPORTED;
}

cc_result StreamContext_Play(struct AudioContext* ctx) {
	return ERR_NOT_SUPPORTED;
}

cc_result StreamContext_Pause(struct AudioContext* ctx) {
	return ERR_NOT_SUPPORTED;
}

cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) { return false; }

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	return ERR_NOT_SUPPORTED;
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
	return ERR_NOT_SUPPORTED;
}


/*########################################################################################################################*
*--------------------------------------------------------Audio misc-------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Audio_DescribeError(cc_result res, cc_string* dst) { return false; }

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	return ERR_NOT_SUPPORTED;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) { }


/*########################################################################################################################*
*----------------------------------------------------------Sounds---------------------------------------------------------*
*#########################################################################################################################*/
void AudioBackend_LoadSounds(void) { }
#endif

