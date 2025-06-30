#include "Core.h"
#if defined CC_BUILD_SYMBIAN && CC_AUD_BACKEND != CC_AUD_BACKEND_NULL

#include <e32base.h>
#include <MdaAudioOutputStream.h>
#include <mda/common/audio.h>
extern "C" {
#include "Audio.h"
#include "Platform.h"
#include "Errors.h"
#include "Funcs.h"
}

#define AUDIO_COMMON_ALLOC
#if defined CC_BUILD_SYMBIAN_3
#define POOL_MAX_CONTEXTS 8
#else
#define POOL_MAX_CONTEXTS 2
#endif

struct AudioBuffer {
	int available;
	int size;
	void* samples;
};

class CAudioStream;

struct AudioContext {
	int count, bufHead, channels, sampleRate, volume;
	CAudioStream* stream;
	struct AudioBuffer bufs[AUDIO_MAX_BUFFERS];
};

extern "C" {
#include "_AudioBase.h"
}

enum AudioState {
	STATE_INITIALIZED = 0x1,
	STATE_STARTED = 0x2,
	STATE_STOPPED = 0x4,
	STATE_CLOSED = 0x8,
	STATE_CHANGE_VOLUME = 0x10,
	STATE_CHANGE_FORMAT = 0x20,
};

class CAudioStream : public CBase, public MMdaAudioOutputStreamCallback {
public:
	static CAudioStream* NewL(struct AudioContext* ctx);
	virtual ~CAudioStream();
	void MaoscOpenComplete(TInt aError);
	void MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer);
	void MaoscPlayComplete(TInt aError);
	void Open();
	void Stop();
	void Start();
	void Request();
	

protected:
	CAudioStream(struct AudioContext* ctx);
	void ConstructL();
public:
	int iState;
private:
	CMdaAudioOutputStream* iOutputStream;
	TMdaAudioDataSettings iAudioSettings;
	struct AudioContext* iContext;
	TPtrC8 iPtr;
};

CAudioStream::CAudioStream(struct AudioContext* ctx) :
	iState(STATE_STOPPED),
	iContext(ctx) { }

CAudioStream::~CAudioStream() {
	if (iState & STATE_INITIALIZED) {
		Stop();
	}
	User::After(100000);
	delete iOutputStream;
}

CAudioStream* CAudioStream::NewL(struct AudioContext* ctx) {
	CAudioStream* self = new (ELeave) CAudioStream(ctx);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
}

void CAudioStream::ConstructL() {
	iOutputStream = CMdaAudioOutputStream::NewL(*this);
}

void CAudioStream::Open() {
	iAudioSettings.iCaps = TMdaAudioDataSettings::ERealTime | TMdaAudioDataSettings::ESampleRateFixed;
	iState &= ~STATE_STOPPED;
	iOutputStream->Open(&iAudioSettings);
}

void CAudioStream::Stop() {
	if (iState & (STATE_STARTED | STATE_INITIALIZED)) {
		iState &= ~STATE_INITIALIZED;
		iOutputStream->Stop();
		iState &= ~STATE_STARTED;
		iState |= STATE_STOPPED;
	}
	
	for (int i = 0; i < iContext->count; i++) {
		iContext->bufs[i].available = true;
	}
}

void CAudioStream::Start() {
	if (iPtr.Length() == 0) {
		iState |= STATE_STARTED;
		if (iState & STATE_INITIALIZED) {
			Request();
		} else if (iState & STATE_STOPPED) {
			Open();
		}
	}
}

static int GetSampleRate(int sampleRate) {
	switch (sampleRate) {
	case 8000:
		return TMdaAudioDataSettings::ESampleRate8000Hz;
	case 11025:
		return TMdaAudioDataSettings::ESampleRate11025Hz;
	case 12000:
		return TMdaAudioDataSettings::ESampleRate12000Hz;
	case 16000:
		return TMdaAudioDataSettings::ESampleRate16000Hz;
	case 22050:
		return TMdaAudioDataSettings::ESampleRate22050Hz;
	case 24000:
		return TMdaAudioDataSettings::ESampleRate24000Hz;
	case 32000:
		return TMdaAudioDataSettings::ESampleRate32000Hz;
	case 44100:
		return TMdaAudioDataSettings::ESampleRate44100Hz;
	case 48000:
		return TMdaAudioDataSettings::ESampleRate48000Hz;
	case 96000:
		return TMdaAudioDataSettings::ESampleRate96000Hz;
	case 64000:
		return TMdaAudioDataSettings::ESampleRate64000Hz;
	default:
		return 0;
	}
}

static int GetNearestSampleRate(int sampleRate) {
	if (sampleRate >= 96000) return 96000;
	if (sampleRate < 8000) return 8000;
	
	while (GetSampleRate(sampleRate) == 0) {
		++sampleRate;
	}
	return sampleRate;
}

void CAudioStream::MaoscOpenComplete(TInt aError) {
	if (aError == KErrNone) {
		iOutputStream->SetPriority(EMdaPriorityNormal, EMdaPriorityPreferenceTime);
		
		iState |= STATE_INITIALIZED | STATE_CHANGE_VOLUME | STATE_CHANGE_FORMAT;
		if (iState & STATE_STARTED) {
			Request();
		}
	} else {
		Audio_Warn(aError, "opening audio stream");
	}
}

void CAudioStream::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer) {
	iPtr.Set(KNullDesC8);

	// MaoscPlayComplete() is not called in older versions
#if defined CC_BUILD_SYMBIAN_3
	if (iContext->count > 1) {
#else
	if (iContext->count) {
#endif
		struct AudioBuffer* buf = &iContext->bufs[iContext->bufHead];
		iContext->bufHead = (iContext->bufHead + 1) % iContext->count;
		buf->available = true;
	}
	
	if (aError == KErrNone) {
		if (iState & STATE_INITIALIZED) {
			Request();
		} else {
			iOutputStream->Stop();
		}
	}
}

void CAudioStream::MaoscPlayComplete(TInt aError) {
	iPtr.Set(KNullDesC8);
	iState &= ~STATE_STARTED;
	
	for (int i = 0; i < iContext->count; i++) {
		iContext->bufs[i].available = true;
	}
}

void CAudioStream::Request() {
	if (iState & STATE_INITIALIZED) {
		iPtr.Set(KNullDesC8);
		TInt err = 0;
		
		if (iState & STATE_CHANGE_FORMAT) {
			iState &= ~STATE_CHANGE_FORMAT;
			int sampleRate = GetSampleRate(GetNearestSampleRate(iContext->sampleRate));
			int channels;
			switch (iContext->channels) {
			case 1:
				channels = TMdaAudioDataSettings::EChannelsMono;
				break;
			case 2:
			default:
				channels = TMdaAudioDataSettings::EChannelsStereo;
				break;
			}
			TRAP(err, iOutputStream->SetAudioPropertiesL(sampleRate, channels));
			if (err != 0) {
				Audio_Warn(err, "setting audio properties");
				Stop();
				return;
			}
		}
		
		if (iState & STATE_CHANGE_VOLUME) {
			iState &= ~STATE_CHANGE_VOLUME;
			iOutputStream->SetVolume((iContext->volume * iOutputStream->MaxVolume()) / 100);
		}
		
		if (iState & STATE_STARTED) {
			struct AudioBuffer* buf = &iContext->bufs[iContext->bufHead];
			if (buf->size == 0 && buf->samples == NULL) {
				return;
			}
			iPtr.Set((const TUint8*)buf->samples, (TInt)buf->size);
			buf->size = 0;
			buf->samples = NULL;
		}
		
		if (iPtr.Length() == 0) {
			return;
		}
		
		TRAP(err, iOutputStream->WriteL(iPtr));
		if (err != KErrNone) {
			Audio_Warn(err, "writing to audio stream");
		}
	}
}

cc_bool AudioBackend_Init(void) {
	return true;
}

void AudioBackend_Tick(void) { }

void AudioBackend_Free(void) { }

cc_result Audio_Init(struct AudioContext* ctx, int buffers) {
	TRAPD(err, ctx->stream = CAudioStream::NewL(ctx));
	if (err != KErrNone) {
		return err;
	}
	
	ctx->count = buffers;
	ctx->bufHead = 0;
	ctx->volume = 100;
	
	Mem_Set(ctx->bufs, 0, sizeof(ctx->bufs));
	for (int i = 0; i < buffers; i++) {
		ctx->bufs[i].available = true;
	}
	
	return 0;
}

void Audio_Close(struct AudioContext* ctx) {
	ctx->count = 0;
	if (ctx->stream) {
		delete ctx->stream;
		ctx->stream = NULL;
	}
}

void Audio_SetVolume(struct AudioContext* ctx, int volume) {
	ctx->volume = volume;
	ctx->stream->iState |= STATE_CHANGE_VOLUME;
}

cc_result Audio_QueueChunk(struct AudioContext* ctx, struct AudioChunk* chunk) {
	struct AudioBuffer* buf;

	for (int i = 0; i < ctx->count; i++) {
		buf = &ctx->bufs[i];
		if (!buf->available) continue;

		buf->samples   = chunk->data;
		buf->size      = chunk->size;
		buf->available = false;
		return 0;
	}
		
	return ERR_INVALID_ARGUMENT;
}

cc_result Audio_Play(struct AudioContext* ctx) {
	TRAPD(err, ctx->stream->Start());
	return err;
}

cc_result Audio_SetFormat(struct AudioContext* ctx, int channels, int sampleRate, int playbackRate) {
	// playbackRate not supported
	int sampleRateNew = /* Audio_AdjustSampleRate(sampleRate, playbackRate) */ sampleRate;
	
	if (ctx->channels != channels || ctx->sampleRate != sampleRateNew) {
		ctx->channels = channels;
		ctx->sampleRate = sampleRateNew;
		ctx->stream->iState |= STATE_CHANGE_FORMAT;
	}
	
	if (ctx->stream->iState & STATE_STOPPED) {
		ctx->stream->Open();
	}
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
	return Audio_Play(ctx);
}

cc_result StreamContext_Pause(struct AudioContext* ctx) {
	ctx->stream->Stop();
	return 0;
}

cc_result StreamContext_Update(struct AudioContext* ctx, int* inUse) {
	struct AudioBuffer* buf;
	int i, count = 0;
	
	// Process background tasks in music thread
	RThread thread;
	TInt error = KErrNone;
	while (thread.RequestCount()) {
		CActiveScheduler::RunIfReady(error, CActive::EPriorityIdle);
		User::WaitForAnyRequest();
	}

	for (i = 0; i < ctx->count; i++) 
	{
		buf = &ctx->bufs[i];
		if (!buf->available) count++;
	}

	*inUse = count;
	return 0;
}


/*########################################################################################################################*
*------------------------------------------------------Sound context------------------------------------------------------*
*#########################################################################################################################*/
cc_bool SoundContext_FastPlay(struct AudioContext* ctx, struct AudioData* data) {
	return !ctx->channels || (ctx->channels == data->channels && ctx->sampleRate == data->sampleRate);
}

cc_result SoundContext_PlayData(struct AudioContext* ctx, struct AudioData* data) {
    cc_result res;

	if ((res = Audio_SetFormat(ctx,  data->channels, data->sampleRate, data->rate))) return res;
	if ((res = Audio_QueueChunk(ctx, &data->chunk))) return res;
	if ((res = Audio_Play(ctx))) return res;

	return 0;
}

cc_result SoundContext_PollBusy(struct AudioContext* ctx, cc_bool* isBusy) {
	*isBusy = !ctx->bufs[0].available;
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------------Audio misc-------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Audio_DescribeError(cc_result res, cc_string* dst) {
	return false;
}
#endif

