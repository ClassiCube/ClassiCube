#include "Core.h"

#if defined CC_BUILD_DREAMCAST
#include <kos.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/aica_comm.h>
#include "Audio.h"

struct AudioBuffer {
	int available;
	int bytesLeft;
	void* samples;
};

struct AudioContext {
	int volume, count;
	int chan_ids[2];
};
#define AUDIO_OVERRIDE_ALLOC
#include "_AudioBase.h"
#include "Funcs.h"

cc_bool AudioBackend_Init(void) {
	return snd_init() == 0;
}

void AudioBackend_Tick(void) {
}

void AudioBackend_Free(void) {
	
}

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	ctx->chan_ids[0] = snd_sfx_chn_alloc();
	ctx->chan_ids[1] = snd_sfx_chn_alloc();
	
	ctx->count = buffers;
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	if (ctx->count) {
		snd_sfx_stop(ctx->chan_ids[0]);
		snd_sfx_stop(ctx->chan_ids[1]);

		snd_sfx_chn_free(ctx->chan_ids[0]);
		snd_sfx_chn_free(ctx->chan_ids[1]);
	}
	ctx->count = 0;
}

static void SetChannelVolume(int ch, int volume) {
	AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

	cmd->cmd    = AICA_CMD_CHAN;
	cmd->timestamp = 0;
	cmd->size   = AICA_CMDSTR_CHANNEL_SIZE;
	cmd->cmd_id = ch;

	chan->cmd   = AICA_CH_CMD_UPDATE | AICA_CH_UPDATE_SET_VOL;
	chan->vol   = volume;

	snd_sh4_to_aica(tmp, cmd->size);
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	ctx->volume = volume;
	SetChannelVolume(ctx->chan_ids[0], volume);
	SetChannelVolume(ctx->chan_ids[1], volume);
}


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
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return true;
}

static void PlaySound(struct AudioContext* ctx, int ch, int freq, struct AudioChunk* chunk) {
    AICA_CMDSTR_CHANNEL(tmp, cmd, chan);

    int samples = chunk->size; // altered in SoundContext_Prepare
    if (samples >= 65535) samples = 65534;

	cmd->cmd = AICA_CMD_CHAN;
    cmd->timestamp  = 0;
    cmd->size       = AICA_CMDSTR_CHANNEL_SIZE;
    cmd->cmd_id     = ctx->chan_ids[ch];

    chan->cmd       = AICA_CH_CMD_START;
    chan->base      = chunk->meta.val + (ch * (samples * 2));
    chan->type      = AICA_SM_16BIT;
    chan->length    = samples;
    chan->loop      = 0;
    chan->loopstart = 0;
    chan->loopend   = samples;
    chan->freq      = freq;
    chan->vol       = ctx->volume;

	// TODO panning
	Platform_Log2("Playing %i samples to %i", &samples, &ctx->chan_ids[ch]);
	snd_sh4_to_aica(tmp, cmd->size);
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
	Platform_Log3("???? %h.%h.%h", &data->chunk.data, &data->chunk.size, &data->chunk.meta);
	if (!data->chunk.meta.val) return ERR_NOT_SUPPORTED;
	int freq = Audio_AdjustSampleRate(data->sampleRate, data->rate);
    
	snd_sh4_to_aica_stop();

	if (data->channels > 0) PlaySound(ctx, 0, freq, &data->chunk);
	if (data->channels > 1) PlaySound(ctx, 1, freq, &data->chunk);

	snd_sh4_to_aica_start();

	return 0;
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
	*isBusy = false; // TODO
	return 0;
}

void SoundContext_Prepare(struct Sound* snd) {
	int size = snd->chunk.size;
	uint32_t aram = snd_mem_malloc(size);

	snd->chunk.meta.val = aram;
	if (!aram) { Platform_LogConst("out of memory?"); return; }

	if (snd->channels == 2) {
		Platform_Log2("stereo %i to %h", &size, &aram);
		snd_pcm16_split_sq(snd->chunk.data, aram, aram + size / 2, size);
	} else {
		Platform_Log2("mono %i to %h", &size, &aram);
		spu_memload_sq(aram, snd->chunk.data, size);
	}

	free(snd->chunk.data);
	snd->chunk.data = NULL;
	snd->chunk.size = size / (2 * snd->channels); // store number of samples instead 
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
#endif

