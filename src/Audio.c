#include "Audio.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Block.h"
#include "Game.h"
#include "GameStructs.h"
#include "Errors.h"
#include "Vorbis.h"
#include "Chat.h"
#include "Stream.h"

StringsBuffer files;
static void Volume_Mix16(Int16* samples, Int32 count, Int32 volume) {
	Int32 i;

	for (i = 0; i < (count & ~0x07); i += 8, samples += 8) {
		samples[0] = (samples[0] * volume / 100);
		samples[1] = (samples[1] * volume / 100);
		samples[2] = (samples[2] * volume / 100);
		samples[3] = (samples[3] * volume / 100);

		samples[4] = (samples[4] * volume / 100);
		samples[5] = (samples[5] * volume / 100);
		samples[6] = (samples[6] * volume / 100);
		samples[7] = (samples[7] * volume / 100);
	}

	for (; i < count; i++, samples++) {
		samples[0] = (samples[0] * volume / 100);
	}
}

static void Volume_Mix8(UInt8* samples, Int32 count, Int32 volume) {
	Int32 i;
	for (i = 0; i < count; i++, samples++) {
		samples[0] = (127 + (samples[0] - 127) * volume / 100);
	}
}

/*########################################################################################################################*
*------------------------------------------------------Soundboard---------------------------------------------------------*
*#########################################################################################################################*/
struct Sound {
	struct AudioFormat Format;
	UInt8* Data; UInt32 DataSize;
};

#define AUDIO_MAX_SOUNDS 10
struct SoundGroup {
	char NameBuffer[16];
	String Name; UInt8 Count;
	struct Sound Sounds[AUDIO_MAX_SOUNDS];
};

struct Soundboard {
	Random Rnd; UInt8 Count;
	struct SoundGroup Groups[AUDIO_MAX_SOUNDS];
};

#define WAV_FourCC(a, b, c, d) (((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d)
#define WAV_FMT_SIZE 16

static ReturnCode Sound_ReadWaveData(struct Stream* stream, struct Sound* snd) {
	UInt32 fourCC, size;
	UInt8 tmp[WAV_FMT_SIZE];
	ReturnCode res;

	if (res = Stream_Read(stream, tmp, 12)) return res;
	fourCC = Stream_GetU32_BE(&tmp[0]);
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;
	/* tmp[4] (4) file size */
	fourCC = Stream_GetU32_BE(&tmp[8]);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	for (;;) {
		if (res = Stream_Read(stream, tmp, 8)) return res;
		fourCC = Stream_GetU32_BE(&tmp[0]);
		size   = Stream_GetU32_LE(&tmp[4]);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			if (res = Stream_Read(stream, tmp, sizeof(tmp))) return res;
			if (Stream_GetU16_LE(&tmp[0]) != 1) return WAV_ERR_DATA_TYPE;

			snd->Format.Channels      = Stream_GetU16_LE(&tmp[2]);
			snd->Format.SampleRate    = Stream_GetU32_LE(&tmp[4]);
			/* tmp[8] (6) alignment data and stuff */
			snd->Format.BitsPerSample = Stream_GetU16_LE(&tmp[14]);
			size -= WAV_FMT_SIZE;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->Data = Mem_Alloc(size, sizeof(UInt8), "WAV sound data");
			snd->DataSize = size;
			return Stream_Read(stream, snd->Data, size);
		}

		/* Skip over unhandled data */
		if (size && (res = Stream_Skip(stream, size))) return res;
	}
}

static ReturnCode Sound_ReadWave(STRING_PURE String* filename, struct Sound* snd) {
	char pathBuffer[FILENAME_SIZE];
	String path = String_FromArray(pathBuffer);
	String_Format2(&path, "audio%r%s", &Directory_Separator, filename);

	ReturnCode res;
	void* file; res = File_Open(&file, &path);
	if (res) return res;

	struct Stream stream; Stream_FromFile(&stream, file);
	{
		res = Sound_ReadWaveData(&stream, snd);
		if (res) { stream.Close(&stream); return res; }
	}
	return stream.Close(&stream);
}

static struct SoundGroup* Soundboard_Find(struct Soundboard* board, STRING_PURE String* name) {
	Int32 i;
	struct SoundGroup* groups = board->Groups;

	for (i = 0; i < Array_Elems(board->Groups); i++) {
		if (String_CaselessEquals(&groups[i].Name, name)) return &groups[i];
	}
	return NULL;
}

static void Soundboard_Init(struct Soundboard* board, STRING_PURE String* boardName, StringsBuffer* files) {
	Int32 i;
	for (i = 0; i < files->Count; i++) {
		String file = StringsBuffer_UNSAFE_Get(files, i), name = file;
		/* dig_grass1.wav -> dig_grass1 */
		Int32 dotIndex = String_LastIndexOf(&name, '.');
		if (dotIndex >= 0) { name = String_UNSAFE_Substring(&name, 0, dotIndex); }
		if (!String_CaselessStarts(&name, boardName)) continue;

		/* Convert dig_grass1 to grass */
		name = String_UNSAFE_SubstringAt(&name, boardName->length);
		name = String_UNSAFE_Substring(&name, 0, name.length - 1);

		struct SoundGroup* group = Soundboard_Find(board, &name);
		if (group == NULL) {
			if (board->Count == Array_Elems(board->Groups)) {
				Chat_AddRaw("&cCannot have more than 10 sound groups"); return;
			}
			group = &board->Groups[board->Count++];

			String str = String_FromArray(group->NameBuffer);
			String_Copy(&str, &name);
			group->Name = str;
		}

		if (group->Count == Array_Elems(group->Sounds)) {
			Chat_AddRaw("&cCannot have more than 10 sounds in a group"); return;
		}

		struct Sound* snd = &group->Sounds[group->Count];
		ReturnCode res = Sound_ReadWave(&file, snd);

		if (res) {
			Chat_LogError(res, "decoding", &file);
			Mem_Free(snd->Data);
			snd->Data     = NULL;
			snd->DataSize = 0;
		} else { group->Count++; }
	}
}

struct Sound* Soundboard_PickRandom(struct Soundboard* board, UInt8 type) {
	if (type == SOUND_NONE || type >= SOUND_COUNT) return NULL;
	if (type == SOUND_METAL) type = SOUND_STONE;
	String name = String_FromReadonly(Sound_Names[type]);

	struct SoundGroup* group = Soundboard_Find(board, &name);
	if (group == NULL) return NULL;
	Int32 idx = Random_Range(&board->Rnd, 0, group->Count);
	return &group->Sounds[idx];
}


/*########################################################################################################################*
*--------------------------------------------------------Sounds-----------------------------------------------------------*
*#########################################################################################################################*/
struct SoundOutput { AudioHandle Handle; void* Buffer; UInt32 BufferSize; };
#define AUDIO_MAX_HANDLES 6
#define HANDLE_INV -1
#define SOUND_INV { HANDLE_INV, NULL, 0 }

struct Soundboard digBoard, stepBoard;
struct SoundOutput monoOutputs[AUDIO_MAX_HANDLES]   = { SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV };
struct SoundOutput stereoOutputs[AUDIO_MAX_HANDLES] = { SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV, SOUND_INV };

static void Sounds_PlayRaw(struct SoundOutput* output, struct Sound* snd, struct AudioFormat* fmt, Int32 volume) {
	Audio_SetFormat(output->Handle, fmt);
	void* buffer = snd->Data;
	/* copy to temp buffer to apply volume */

	if (volume < 100) {		
		if (output->BufferSize < snd->DataSize) {
			UInt32 expandBy = snd->DataSize - output->BufferSize;
			Utils_Resize(&output->Buffer, &output->BufferSize, 1, 0, expandBy);
		}
		buffer = output->Buffer;

		Mem_Copy(buffer, snd->Data, snd->DataSize);
		if (fmt->BitsPerSample == 8) {
			Volume_Mix8(buffer,  snd->DataSize,     volume);
		} else {
			Volume_Mix16(buffer, snd->DataSize / 2, volume);
		}
	}

	Audio_BufferData(output->Handle, 0, buffer, snd->DataSize);
	Audio_Play(output->Handle);
	/* TODO: handle errors here */
}

static void Sounds_Play(UInt8 type, struct Soundboard* board) {
	if (type == SOUND_NONE || Game_SoundsVolume == 0) return;
	struct Sound* snd = Soundboard_PickRandom(board, type);

	if (snd == NULL) return;
	struct AudioFormat fmt = snd->Format;
	Int32 volume = Game_SoundsVolume;

	if (board == &digBoard) {
		if (type == SOUND_METAL) fmt.SampleRate = (fmt.SampleRate * 6) / 5;
		else fmt.SampleRate = (fmt.SampleRate * 4) / 5;
	} else {
		volume /= 2;
		if (type == SOUND_METAL) fmt.SampleRate = (fmt.SampleRate * 7) / 5;
	}

	struct SoundOutput* outputs = fmt.Channels == 1 ? monoOutputs : stereoOutputs;
	Int32 i;

	/* Try to play on fresh device, or device with same data format */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		struct SoundOutput* output = &outputs[i];
		if (output->Handle == HANDLE_INV) {
			Audio_Init(&output->Handle, 1);
		}
		if (!Audio_IsFinished(output->Handle)) continue;

		struct AudioFormat* l = Audio_GetFormat(output->Handle);
		if (l->Channels == 0 || AudioFormat_Eq(l, &fmt)) {
			Sounds_PlayRaw(output, snd, &fmt, volume); return;
		}
	}

	/* Try again with all devices, even if need to recreate one (expensive) */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		struct SoundOutput* output = &outputs[i];
		if (!Audio_IsFinished(output->Handle)) continue;

		Sounds_PlayRaw(output, snd, &fmt, volume); return;
	}
}

static void Audio_PlayBlockSound(void* obj, Vector3I coords, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		Audio_PlayDigSound(Block_DigSounds[old]);
	} else if (!Game_ClassicMode) {
		Audio_PlayDigSound(Block_StepSounds[now]);
	}
}

static void Sounds_FreeOutputs(struct SoundOutput* outputs) {
	bool anyPlaying = true;
	Int32 i;

	while (anyPlaying) {
		anyPlaying = false;
		for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
			if (outputs[i].Handle == HANDLE_INV) continue;
			anyPlaying |= !Audio_IsFinished(outputs[i].Handle);
		}
		if (anyPlaying) Thread_Sleep(1);
	}

	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		if (outputs[i].Handle == HANDLE_INV) continue;
		Audio_Free(outputs[i].Handle);
		outputs[i].Handle = HANDLE_INV;

		Mem_Free(outputs[i].Buffer);
		outputs[i].Buffer     = NULL;
		outputs[i].BufferSize = 0;
	}
}

static void Sounds_Init(void) {
	if (digBoard.Count || stepBoard.Count) return;
	String dig  = String_FromConst("dig_");
	Soundboard_Init(&digBoard, &dig, &files);
	String step = String_FromConst("step_");
	Soundboard_Init(&stepBoard, &step, &files);
}

static void Sounds_Free(void) {
	Sounds_FreeOutputs(monoOutputs);
	Sounds_FreeOutputs(stereoOutputs);
}

void Audio_SetSounds(Int32 volume) {
	if (volume) Sounds_Init();
	else        Sounds_Free();
}

void Audio_PlayDigSound(UInt8 type)  { Sounds_Play(type, &digBoard); }
void Audio_PlayStepSound(UInt8 type) { Sounds_Play(type, &stepBoard); }


/*########################################################################################################################*
*--------------------------------------------------------Music------------------------------------------------------------*
*#########################################################################################################################*/
AudioHandle music_out = -1;
StringsBuffer music_files;
void* music_thread;
void* music_waitable;
volatile bool music_pendingStop;

static ReturnCode Music_BufferBlock(Int32 i, Int16* data, Int32 maxSamples, struct VorbisState* ctx) {
	Int32 samples = 0;
	ReturnCode res = 0;

	while (samples < maxSamples) {
		res = Vorbis_DecodeFrame(ctx);
		if (res) break;

		Int16* cur = &data[samples];
		samples += Vorbis_OutputFrame(ctx, cur);
	}

	if (Game_MusicVolume < 100) { Volume_Mix16(data, samples, Game_MusicVolume); }
	Audio_BufferData(music_out, i, data, samples * sizeof(Int16));
	return res;
}

static ReturnCode Music_PlayOgg(struct Stream* source) {
	UInt8 buffer[OGG_BUFFER_SIZE];
	struct Stream stream;
	Ogg_MakeStream(&stream, buffer, source);

	struct VorbisState vorbis = { 0 };
	vorbis.Source = &stream;
	ReturnCode res = Vorbis_DecodeHeaders(&vorbis);
	if (res) return res;

	struct AudioFormat fmt;
	fmt.Channels      = vorbis.Channels;
	fmt.SampleRate    = vorbis.SampleRate;
	fmt.BitsPerSample = 16;
	Audio_SetFormat(music_out, &fmt);

	/* largest possible vorbis frame decodes to blocksize1 * channels samples */
	/* so we may end up decoding slightly over a second of audio */
	Int32 i, chunkSize     = fmt.Channels * (fmt.SampleRate + vorbis.BlockSizes[1]);
	Int32 samplesPerSecond = fmt.Channels * fmt.SampleRate;
	Int16* data = Mem_Alloc(chunkSize * AUDIO_MAX_CHUNKS, sizeof(Int16), "Ogg - final PCM output");

	/* fill up with some samples before playing */
	for (i = 0; i < AUDIO_MAX_CHUNKS && !res; i++) {
		Int16* base = data + (chunkSize * i);
		res = Music_BufferBlock(i, base, samplesPerSecond, &vorbis);
	}
	Audio_Play(music_out);

	for (;;) {
		Int32 next = -1;
		for (i = 0; i < AUDIO_MAX_CHUNKS; i++) {
			if (Audio_IsCompleted(music_out, i)) { next = i; break; }
		}

		if (next == -1) { Thread_Sleep(10); continue; }
		if (music_pendingStop) break;

		Int16* base = data + (chunkSize * next);
		res = Music_BufferBlock(next, base, samplesPerSecond, &vorbis);
		/* need to specially handle last bit of audio */
		if (res) break;
	}

	/* Wait until the buffers finished playing */
	while (!Audio_IsFinished(music_out)) { Thread_Sleep(10); }

	Mem_Free(data);
	Vorbis_Free(&vorbis);

	if (res == ERR_END_OF_STREAM) res = 0;
	return res;
}

#define MUSIC_MAX_FILES 512
static void Music_RunLoop(void) {
	Int32 i, count = 0;
	UInt16 musicFiles[MUSIC_MAX_FILES];
	String ogg = String_FromConst(".ogg");

	for (i = 0; i < files.Count && count < MUSIC_MAX_FILES; i++) {
		String file = StringsBuffer_UNSAFE_Get(&files, i);
		if (!String_CaselessEnds(&file, &ogg)) continue;
		musicFiles[count++] = i;
	}

	if (!count) return;
	Random rnd; Random_InitFromCurrentTime(&rnd);
	char pathBuffer[FILENAME_SIZE];
	ReturnCode res;

	while (!music_pendingStop) {
		Int32 idx = Random_Range(&rnd, 0, count);
		String filename = StringsBuffer_UNSAFE_Get(&files, musicFiles[idx]);
		String path = String_FromArray(pathBuffer);
		String_Format2(&path, "audio%r%s", &Directory_Separator, &filename);
		Platform_Log1("playing music file: %s", &filename);

		void* file; res = File_Open(&file, &path);
		if (res) { Chat_LogError(res, "opening", &path); return; }
		struct Stream stream; Stream_FromFile(&stream, file);
		{
			res = Music_PlayOgg(&stream);
			if (res) { Chat_LogError(res, "playing", &path); }
		}
		res = stream.Close(&stream);
		if (res) { Chat_LogError(res, "closing", &path); }

		if (music_pendingStop) return;
		Int32 delay = 1000 * 120 + Random_Range(&rnd, 0, 1000 * 300);
		Waitable_WaitFor(music_waitable, delay);
	}
}

static void Music_Init(void) {
	if (music_thread) return;
	music_pendingStop = false;
	Audio_Init(&music_out, AUDIO_MAX_CHUNKS);
	music_thread = Thread_Start(Music_RunLoop);
}

static void Music_Free(void) {
	music_pendingStop = true;
	Waitable_Signal(music_waitable);
	if (music_out == -1) return;

	Thread_Join(music_thread);
	Thread_FreeHandle(music_thread);
	Audio_Free(music_out);
	music_out = -1;
	music_thread = NULL;
}

void Audio_SetMusic(Int32 volume) {
	if (volume) Music_Init();
	else        Music_Free();
}


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
static Int32 AudioManager_GetVolume(const char* volKey, const char* boolKey) {
	Int32 volume = Options_GetInt(volKey, 0, 100, 0);
	if (volume) return volume;

	volume = Options_GetBool(boolKey, false) ? 100 : 0;
	Options_Set(boolKey, NULL);
	return volume;
}

static void AudioManager_FilesCallback(STRING_PURE String* filename, void* obj) {
	StringsBuffer_Add(&files, filename);
}

static void AudioManager_Init(void) {
	music_waitable = Waitable_Create();
	String path = String_FromConst("audio");
	if (Directory_Exists(&path)) {
		Directory_Enum(&path, NULL, AudioManager_FilesCallback);
	}

	Game_MusicVolume  = AudioManager_GetVolume(OPT_MUSIC_VOLUME, OPT_USE_MUSIC);
	Audio_SetMusic(Game_MusicVolume);
	Game_SoundsVolume = AudioManager_GetVolume(OPT_SOUND_VOLUME, OPT_USE_SOUND);
	Audio_SetSounds(Game_SoundsVolume);
	Event_RegisterBlock(&UserEvents_BlockChanged, NULL, Audio_PlayBlockSound);
}

static void AudioManager_Free(void) {
	Music_Free();
	Sounds_Free();
	Waitable_Free(music_waitable);
	Event_UnregisterBlock(&UserEvents_BlockChanged, NULL, Audio_PlayBlockSound);
}

void Audio_MakeComponent(struct IGameComponent* comp) {
	comp->Init = AudioManager_Init;
	comp->Free = AudioManager_Free;
}
