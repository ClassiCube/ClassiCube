#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_WINMM
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>
/*
#include <winmm.h>
*/
/* Compatibility versions so compiling works on older Windows SDKs */
#include "../misc/windows/min-winmm.h"

#include "Audio.h"
struct AudioContext {
	HWAVEOUT handle;
	WAVEHDR headers[AUDIO_MAX_BUFFERS];
	int count, channels, sampleRate, volume;
	cc_uint32 _tmpSize[AUDIO_MAX_BUFFERS];
	void* _tmpData[AUDIO_MAX_BUFFERS];
};

#define AUDIO_COMMON_VOLUME
#include "_AudioBase.h"

cc_bool AudioBackend_Init(void) { return true; }
void AudioBackend_Tick(void) { }
void AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	int i;
	for (i = 0; i < buffers; i++) {
		ctx->headers[i].dwFlags = WHDR_DONE;
	}
	ctx->count  = buffers;
	ctx->volume = 100;
	return 0;
}

static void Audio_Stop(struct AudioContext* ctx) {
	waveOutReset(ctx->handle);
}

static cc_result Audio_Reset(struct AudioContext* ctx) {
	cc_result res;
	if (!ctx->handle) return 0;

	res = waveOutClose(ctx->handle);
	ctx->handle = NULL;
	return res;
}

void Audio_Close(struct AudioContext* ctx) {
	int inUse;
	if (ctx->handle) {
		Audio_Stop(ctx);
		Audio_Poll(ctx, &inUse); /* unprepare buffers */
		Audio_Reset(ctx);
	}
	AudioBase_Clear(ctx);
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	WAVEFORMATEX fmt;
	cc_result res;
	int sampleSize;

	sampleRate = Audio_AdjustSampleRate(sampleRate, playbackRate);
	if (ctx->channels == channels && ctx->sampleRate == sampleRate) return 0;
	ctx->channels   = channels;
	ctx->sampleRate = sampleRate;
	
	sampleSize = channels * 2; /* 16 bits per sample / 8 */
	if ((res = Audio_Reset(ctx))) return res;

	fmt.wFormatTag      = WAVE_FORMAT_PCM;
	fmt.nChannels       = channels;
	fmt.nSamplesPerSec  = sampleRate;
	fmt.nAvgBytesPerSec = sampleRate * sampleSize;
	fmt.nBlockAlign     = sampleSize;
	fmt.wBitsPerSample  = 16;
	fmt.cbSize          = 0;
	
	res = waveOutOpen(&ctx->handle, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL);
	/* Show a better error message when no audio output devices connected than */
	/*  "A device ID has been used that is out of range for your system" */
	if (res == MMSYSERR_BADDEVICEID && waveOutGetNumDevs() == 0)
		return ERR_NO_AUDIO_OUTPUT;
	return res;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) { ctx->volume = volume; }

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	cc_result res;
	WAVEHDR* hdr;
	cc_bool ok;
	int i;
	struct AudioChunk tmp = *chunk;

	for (i = 0; i < ctx->count; i++) {
		hdr = &ctx->headers[i];
		if (!(hdr->dwFlags & WHDR_DONE)) continue;
		
		ok = AudioBase_AdjustSound(ctx, i, &tmp);
		if (!ok) return ERR_OUT_OF_MEMORY;

		Mem_Set(hdr, 0, sizeof(WAVEHDR));
		hdr->lpData         = (LPSTR)tmp.data;
		hdr->dwBufferLength = tmp.size;
		hdr->dwLoops        = 1;
		
		if ((res = waveOutPrepareHeader(ctx->handle, hdr, sizeof(WAVEHDR)))) return res;
		if ((res = waveOutWrite(ctx->handle, hdr, sizeof(WAVEHDR))))         return res;
		return 0;
	}
	/* tried to queue data without polling for free buffers first */
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	cc_result res = 0;
	WAVEHDR* hdr;
	int i, count = 0;

	for (i = 0; i < ctx->count; i++) {
		hdr = &ctx->headers[i];
		if (!(hdr->dwFlags & WHDR_DONE)) { count++; continue; }
	
		if (!(hdr->dwFlags & WHDR_PREPARED)) continue;
		/* unprepare this WAVEHDR so it can be reused */
		res = waveOutUnprepareHeader(ctx->handle, hdr, sizeof(WAVEHDR));
		if (res) break;
	}

	*inUse = count; return res;
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
	int channels   = data->channels;
	int sampleRate = Audio_AdjustSampleRate(data->sampleRate, data->rate);
	return !ctx->channels || (ctx->channels == channels && ctx->sampleRate == sampleRate);
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    cc_result res;

	if ((res = Audio_SetFormat(ctx,  data->channels, data->sampleRate, data->rate))) return res;
	if ((res = Audio_QueueChunk(ctx, &data->chunk))) return res;

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
	char buffer[NATIVE_STR_LEN] = { 0 };
	waveOutGetErrorTextA(res, buffer, NATIVE_STR_LEN);

	if (!buffer[0]) return false;
	String_AppendConst(dst, buffer);
	return true;
}
#endif

