#include "Core.h"

#if defined CC_BUILD_SWITCH
#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include "Audio.h"

struct AudioContext {
	int chanID, count;
	AudioDriverWaveBuf bufs[AUDIO_MAX_BUFFERS];
	int channels, sampleRate;
};
#define AUDIO_OVERRIDE_ALLOC
#include "_AudioBase.h"

static int channelIDs;
AudioDriver drv;
bool switchAudio = false;
void* audrv_mutex;

cc_bool AudioBackend_Init(void) {
	if (switchAudio) return true;
	switchAudio = true;

	if (!audrv_mutex) audrv_mutex = Mutex_Create("Audio sync");

	static const AudioRendererConfig arConfig =
    {
        .output_rate     = AudioRendererOutputRate_48kHz,
        .num_voices      = 24,
        .num_effects     = 0,
        .num_sinks       = 1,
        .num_mix_objs    = 1,
        .num_mix_buffers = 2,
    };

	audrenInitialize(&arConfig);
	audrvCreate(&drv, &arConfig, 2);

	static const u8 sink_channels[] = { 0, 1 };
	/*int sink =*/ audrvDeviceSinkAdd(&drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

	audrvUpdate(&drv);

	Result res = audrenStartAudioRenderer();
	
	return R_SUCCEEDED(res);
}

void AudioBackend_Tick(void) {
	Mutex_Lock(audrv_mutex);
	if (switchAudio) audrvUpdate(&drv);
	Mutex_Unlock(audrv_mutex);
}

void AudioBackend_Free(void) {
	for (int i = 0; i < 24; i++) {
		audrvVoiceStop(&drv, i);
	}
	audrvUpdate(&drv);
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	int chanID = -1;
	
	for (int i = 0; i < 24; i++)
	{
		// channel in use
		if (channelIDs & (1 << i)) continue;
		
		chanID = i; break;
	}
	if (chanID == -1) return ERR_INVALID_ARGUMENT;
	
	channelIDs |= (1 << chanID);
	ctx->count  = buffers;
	ctx->chanID = chanID;
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->count) {
		audrvVoiceStop(&drv, ctx->chanID);
		channelIDs &= ~(1 << ctx->chanID);
	}
	ctx->count = 0;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;

	audrvVoiceStop(&drv, ctx->chanID);
	audrvVoiceInit(&drv, ctx->chanID, ctx->channels, PcmFormat_Int16, ctx->sampleRate);
	audrvVoiceSetDestinationMix(&drv, ctx->chanID, AUDREN_FINAL_MIX_ID);

	if (channels == 1) {
		// mono
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 1.0f, 0, 0);
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 1.0f, 0, 1);
	} else {
		// stereo
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 1.0f, 0, 0);
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 0.0f, 0, 1);
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 0.0f, 1, 0);
		audrvVoiceSetMixFactor(&drv, ctx->chanID, 1.0f, 1, 1);
	}

	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	audrvVoiceSetVolume(&drv, ctx->chanID, volume / 100.0f);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	AudioDriverWaveBuf* buf;

	// Audio buffers must be aligned to a multiple of 0x1000, according to libnx example code
	if (((uintptr_t)chunk->data & 0xFFF) != 0) {
		Platform_Log1("Audio_QueueData: tried to queue buffer with non-aligned audio buffer 0x%x\n", &chunk->data);
	}
	if ((chunk->size & 0xFFF) != 0) {
		Platform_Log1("Audio_QueueData: unaligned audio data size 0x%x\n", &chunk->size);
	}


	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		int state = buf->state;
		cc_uint32 endOffset = chunk->size / (sizeof(cc_int16) * ((ctx->channels == 2) ? 2 : 1));

		if (state == AudioDriverWaveBufState_Queued || state == AudioDriverWaveBufState_Playing || state == AudioDriverWaveBufState_Waiting)
			continue;

		buf->data_pcm16 = chunk->data;
		buf->size       = chunk->size;
		buf->start_sample_offset = 0;
		buf->end_sample_offset   = endOffset;

		Mutex_Lock(audrv_mutex);
		audrvVoiceAddWaveBuf(&drv, ctx->chanID, buf);
		Mutex_Unlock(audrv_mutex);

		return 0;
	}

	// tried to queue data without polling for free buffers first
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	audrvVoiceStart(&drv, ctx->chanID);
	return 0;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	AudioDriverWaveBuf* buf;
	int count = 0;

	for (int i = 0; i < ctx->count; i++)
	{
		buf = &ctx->bufs[i];
		if (buf->state == AudioDriverWaveBufState_Queued || buf->state == AudioDriverWaveBufState_Playing || buf->state == AudioDriverWaveBufState_Waiting) {
			count++; continue;
		}
	}

	*inUse = count;
	return 0;
}

static cc_bool Audio_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return true;
}

cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	return false;
}

cc_result Audio_AllocChunks(cc_uint32 size, struct AudioChunk* chunks, int numChunks) {
	size = (size + 0xFFF) & ~0xFFF;  // round up to nearest multiple of 0x1000
	void* dst = aligned_alloc(0x1000, size * numChunks);
	if (!dst) return ERR_OUT_OF_MEMORY;

	for (int i = 0; i < numChunks; i++)
	{
		chunks[i].data = dst + size * i;
		chunks[i].size = size;

		int mpid = audrvMemPoolAdd(&drv, chunks[i].data, size);
		audrvMemPoolAttach(&drv, mpid);
		chunks[i].meta.val = mpid;
	}
	return 0;
}

void Audio_FreeChunks(struct AudioChunk* chunks, int numChunks) {
	for (int i = 0; i < numChunks; i++)
	{
		if (!chunks[i].data) continue;
		int mpid = chunks[i].meta.val;

		audrvMemPoolDetach(&drv, mpid);
		audrvMemPoolRemove(&drv, mpid);
	}
	free(chunks[0].data);
}
#endif

