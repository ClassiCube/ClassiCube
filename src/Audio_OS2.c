#include "Core.h"

#if CC_AUD_BACKEND == CC_AUD_BACKEND_OS2
#include <stdio.h>
#include <string.h>
#include <kai.h>

#include "_AudioBase.h"

struct AudioContext {
	int count;
	HKAI hkai;
	void *buffers[AUDIO_MAX_BUFFERS];
	cc_uint32 bufferSize[AUDIO_MAX_BUFFERS];
	int	fillBuffer, drainBuffer;
	int	indexIntoBuffer;
   cc_bool done;
};

CC_INLINE void getNextBuffer(int count, int *bufferIndex) {
	(*bufferIndex)++;
	if (*bufferIndex >= count) *bufferIndex = 0;
}

CC_INLINE void String_DecodeCP1252(cc_string* value, const void* data, int numBytes) {
	const cc_uint8* chars = (const cc_uint8*)data;
	int i; char c;

	for (i = 0; i < numBytes; i++) {
		if (Convert_TryCodepointToCP437(chars[i], &c)) String_Append(value, c);
	}
}

ULONG APIENTRY kaiCallback(PVOID data, PVOID buffer, ULONG size) {
	struct AudioContext *ctx = (struct AudioContext*)data;
	char* dst = buffer;
	void* src = ctx->buffers[ctx->drainBuffer];
	cc_uint32 read = 0;
	
	if (!src) return 0; // Reached end of playable queue

	while (size) {
  	cc_uint32 bufferLeft = ctx->bufferSize[ctx->drainBuffer] - ctx->indexIntoBuffer;
		cc_uint32 len = min(bufferLeft, size);
		
		Mem_Copy(dst, src + ctx->indexIntoBuffer, len);
		
		dst  += len;
		read += len;
		size -= len;
		ctx->indexIntoBuffer += len;
		
		// Finished current buffer
		if (ctx->indexIntoBuffer >= ctx->bufferSize[ctx->drainBuffer]) {
			// Reset the current buffer
			ctx->bufferSize[ctx->drainBuffer] = 0;
			ctx->buffers[ctx->drainBuffer]    = NULL;
			// Begin at the next buffer and go to it
			ctx->drainBuffer  = (ctx->drainBuffer + 1) % ctx->count;
			ctx->indexIntoBuffer = 0;
			src = ctx->buffers[ctx->drainBuffer];
			if (!src) break; // Reached end of playable queue
		}
	}

   if (read < size) ctx->done = true;
	return read;
}

cc_bool AudioBackend_Init(void) {
	KAISPEC ksWanted, ksObtained;
	
	if (kaiInit(KAIM_AUTO) != KAIE_NO_ERROR) {
		Logger_SimpleWarn(ERROR_BASE, "Kai: Init failed.");
		return false;
	}
	return true;
}

void AudioBackend_Free(void) {
printf("kaiDone()\n");
	kaiDone();
}

void AudioBackend_Tick(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	int i;
	
	ctx->count = buffers;
	for(i = 0; i < AUDIO_MAX_BUFFERS; i++) {
		ctx->buffers[i] = NULL;
		ctx->bufferSize[i] = 0;
	}
	ctx->fillBuffer = 0;
	ctx->drainBuffer = 0;
	ctx->indexIntoBuffer = 0;
  return 0;
}

void Audio_Close(struct AudioContext* ctx) {
   ctx->done = false;
	ctx->count = 0;
	if (ctx->hkai > 0) {
		kaiStop(ctx->hkai);
      kaiClearBuffer(ctx->hkai);
		kaiClose(ctx->hkai);
	}
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	KAISPEC ksWanted, ksObtained;
	APIRET rc;
	
	ksWanted.usDeviceIndex      = 0;
   ksWanted.ulType             = KAIT_PLAY;
   ksWanted.ulBitsPerSample    = 16;
   ksWanted.ulSamplingRate     = Audio_AdjustSampleRate(sampleRate, playbackRate);
   ksWanted.ulDataFormat       = 0;
   ksWanted.ulChannels         = channels;
   ksWanted.ulNumBuffers       = 2048;
   ksWanted.ulBufferSize       = 0;
   ksWanted.fShareable         = TRUE;
   ksWanted.pfnCallBack        = kaiCallback;
   ksWanted.pCallBackData      = (PVOID)ctx;

   rc = kaiOpen(&ksWanted, &ksObtained, &ctx->hkai);
   if (rc != KAIE_NO_ERROR) {
  	  Logger_SimpleWarn(ERROR_BASE, "Kai: Could not open playback");
  	  ctx->hkai = 0;
  	  return rc;
   }

	kaiSetSoundState(ctx->hkai, MCI_SET_AUDIO_ALL, true);
	return 0;
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	// Set volume for right and left channel
	if (ctx->hkai) kaiSetVolume(ctx->hkai, MCI_SET_AUDIO_ALL, volume);
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	if (ctx->buffers[ctx->fillBuffer])
		return ERR_INVALID_ARGUMENT; // tried to queue data while either playing or queued still
	
   ctx->buffers[ctx->fillBuffer]    = chunk->data;
	ctx->bufferSize[ctx->fillBuffer] = chunk->size;
	ctx->fillBuffer = (ctx->fillBuffer + 1) % ctx->count;
	return 0;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	APIRET rc;
	
	rc = kaiPlay(ctx->hkai);
	if (rc != KAIE_NO_ERROR) {
		Logger_SimpleWarn(ERROR_BASE, "Kai: Could start playback");
		return rc;
	}
	
	return 0;
}

cc_result Audio_Poll(struct AudioContext* ctx, int* inUse) {
	int i;
	
	*inUse = 0;
	for(i = 0; i < ctx->count; i++) {
		if(ctx->bufferSize[i] > 0) (*inUse)++;
	}
	return ctx->done;
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
	return false;
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
	CHAR buffer[128];
	if (mciGetErrorString(res, buffer, 128) == MCIERR_SUCCESS) {
		String_DecodeCP1252(dst, buffer, strlen(buffer));
	}
	else
		// TODO Query more error messages.
		*dst = String_FromReadonly("Unknown Error");
	return true;
}
#endif

