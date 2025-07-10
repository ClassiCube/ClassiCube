#include "../Audio.h"
#include <3ds.h>

struct AudioContext {
	int chanID, count;
	ndspWaveBuf bufs[AUDIO_MAX_BUFFERS];
	int sampleRate;
	cc_bool stereo;
};
#define AUDIO_OVERRIDE_ALLOC
#include "../_AudioBase.h"

static int channelIDs;
#define MAX_AUDIO_VOICES 24

#define AudioBuf_InUse(buf) ((buf)->status == NDSP_WBUF_QUEUED || (buf)->status == NDSP_WBUF_PLAYING)

// See https://github.com/devkitPro/3ds-examples/blob/master/audio/README.md
// To get audio to work in Citra, just create a 0 byte file in sdmc/3ds named dspfirm.cdca
cc_bool AudioBackend_Init(void) {
	int result = ndspInit();
	Platform_Log2("NDSP_INIT: %i, %h", &result, &result);

	if (result == MAKERESULT(RL_PERMANENT, RS_NOTFOUND, RM_DSP, RD_NOT_FOUND)) {
		static const cc_string msg = String_FromConst("/3ds/dspfirm.cdc not found on SD card, therefore no audio will play");
		Logger_WarnFunc(&msg);
	} else if (result) {
		Audio_Warn(result, "initing DSP for playing audio");
	}

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	return result == 0;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	int chanID = -1;
	
	for (int i = 0; i < MAX_AUDIO_VOICES; i++)
	{
		// channel in use
		if (channelIDs & (1 << i)) continue;
		
		chanID = i; break;
	}
	if (chanID == -1) return ERR_INVALID_ARGUMENT;
	
	channelIDs |= (1 << chanID);
	ctx->count  = buffers;
	ctx->chanID = chanID;

	ndspChnSetInterp(ctx->chanID, NDSP_INTERP_LINEAR);
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->count) {
		ndspChnWaveBufClear(ctx->chanID);
		channelIDs &= ~(1 << ctx->chanID);
	}
	ctx->count = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	ctx->stereo = (channels == 2);
	int fmt = ctx->stereo ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16;

	sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);
	ndspChnSetFormat(ctx->chanID, fmt);
	ndspChnSetRate(ctx->chanID, sampleRate);
	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	float mix[12] = { 0 };
 	mix[0] = volume / 100.0f;
 	mix[1] = volume / 100.0f;
 	
 	ndspChnSetMix(ctx->chanID, mix);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	ndspWaveBuf* buf;

	// DSP audio buffers must be aligned to a multiple of 0x80, according to the example code I could find.
	if (((uintptr_t)chunk->data & 0x7F) != 0) {
		Platform_Log1("Audio_QueueData: tried to queue buffer with non-aligned audio buffer 0x%x\n", &chunk->data);
	}
	if ((chunk->size & 0x7F) != 0) {
		Platform_Log1("Audio_QueueData: unaligned audio data size 0x%x\n", &chunk->size);
	}

	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		if (AudioBuf_InUse(buf))
			continue;

		buf->data_pcm16 = chunk->data;
		buf->nsamples   = chunk->size / (sizeof(cc_int16) * (ctx->stereo ? 2 : 1));
		DSP_FlushDataCache(buf->data_pcm16, chunk->size);
		ndspChnWaveBufAdd(ctx->chanID, buf);
		return 0;
	}
	// tried to queue data without polling for free buffers first
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	ndspWaveBuf* buf;
	int count = 0;

	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		if (AudioBuf_InUse(buf)) {
			count++; continue;
		}
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
	return 0;
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

	return 0;
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
	ndspWaveBuf* buf = &ctx->bufs[0];
	
	*isBusy = AudioBuf_InUse(buf);
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Audio misc-------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	size = (size + 0x7F) & ~0x7F;  // round up to nearest multiple of 0x80
	cc_uint8* dst = linearAlloc(size * numChunks);
	if (!dst) return ERR_OUT_OF_MEMORY;

	for (int i = 0; i < numChunks; i++)
	{
		chunks[i].data = dst + size * i;
		chunks[i].size = size;
	}
	return 0;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) {
	linearFree(chunks[0].data);
}

