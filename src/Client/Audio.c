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

StringsBuffer files;
/*########################################################################################################################*
*------------------------------------------------------Soundboard---------------------------------------------------------*
*#########################################################################################################################*/
struct Sound {
	struct AudioFormat Format;
	UInt8* Data; UInt32 DataSize;
};

#define AUDIO_MAX_SOUNDS 10
struct SoundGroup {
	UChar NameBuffer[String_BufferSize(16)];
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
	UInt32 fourCC, size, pos, len;
	ReturnCode res;
	UInt8 tmp[WAV_FMT_SIZE];

	Stream_Read(stream, tmp, 3 * sizeof(UInt32));
	fourCC = Stream_GetU32_BE(&tmp[0]);
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;

	/* tmp[4] (4) file size */
	fourCC = Stream_GetU32_BE(&tmp[8]);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	while (!(res = stream->Position(stream, &pos)) && !(res = stream->Length(stream, &len)) && pos < len) {
		Stream_Read(stream, tmp, 2 * sizeof(UInt32));
		fourCC = Stream_GetU32_BE(&tmp[0]);
		size   = Stream_GetU32_LE(&tmp[4]);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			Stream_Read(stream, tmp, WAV_FMT_SIZE);
			if (Stream_GetU16_LE(&tmp[0]) != 1) return WAV_ERR_DATA_TYPE;

			snd->Format.Channels      = Stream_GetU16_LE(&tmp[2]);
			snd->Format.SampleRate    = Stream_GetU32_LE(&tmp[4]);
			/* tmp[8] (6) alignment data and stuff */
			snd->Format.BitsPerSample = Stream_GetU16_LE(&tmp[14]);
			size -= WAV_FMT_SIZE;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->Data = Mem_Alloc(size, sizeof(UInt8), "WAV sound data");
			snd->DataSize = size;
			Stream_Read(stream, snd->Data, size);
			return 0;
		}

		/* Skip over unhandled data */
		if (size) Stream_Skip(stream, size);
	}
	return res ? res : WAV_ERR_NO_DATA;
}

static ReturnCode Sound_ReadWave(STRING_PURE String* filename, struct Sound* snd) {
	UChar pathBuffer[String_BufferSize(FILENAME_SIZE)];
	String path = String_InitAndClearArray(pathBuffer);
	String_Format2(&path, "audio%r%s", &Directory_Separator, filename);

	ReturnCode fileResult = 0;
	void* file = NULL; ReturnCode result = File_Open(&file, &path);
	if (result) return result;

	struct Stream stream; Stream_FromFile(&stream, file, &path);
	{
		fileResult = Sound_ReadWaveData(&stream, snd);
	}

	result = stream.Close(&stream);
	return fileResult ? fileResult : result;
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
			if (board->Count == Array_Elems(board->Groups)) ErrorHandler_Fail("Soundboard - too many groups");

			group = &board->Groups[board->Count++];
			group->Name = String_InitAndClearArray(group->NameBuffer);
			String_Set(&group->Name, &name);
		}

		if (group->Count == Array_Elems(group->Sounds)) ErrorHandler_Fail("Soundboard - too many sounds");
		struct Sound* snd = &group->Sounds[group->Count];
		ReturnCode result = Sound_ReadWave(&file, snd);

		ErrorHandler_CheckOrFail(result, "Soundboard - reading WAV");
		group->Count++;
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
struct Soundboard digBoard, stepBoard;
#define AUDIO_MAX_HANDLES 6
AudioHandle monoOutputs[AUDIO_MAX_HANDLES]   = { -1, -1, -1, -1, -1, -1 };
AudioHandle stereoOutputs[AUDIO_MAX_HANDLES] = { -1, -1, -1, -1, -1, -1 };

static void Sounds_PlayRaw(AudioHandle output, struct Sound* snd, struct AudioFormat* fmt, Real32 volume) {
	Audio_SetVolume(output, volume);
	Audio_SetFormat(output, fmt);
	Audio_PlayData(output, 0, snd->Data, snd->DataSize);
	/* TODO: handle errors here */
}

static void Sounds_Play(UInt8 type, struct Soundboard* board) {
	if (type == SOUND_NONE || Game_SoundsVolume == 0) return;
	struct Sound* snd = Soundboard_PickRandom(board, type);

	if (snd == NULL) return;
	struct AudioFormat fmt = snd->Format;
	Real32 volume = Game_SoundsVolume / 100.0f;

	if (board == &digBoard) {
		if (type == SOUND_METAL) fmt.SampleRate = (fmt.SampleRate * 6) / 5;
		else fmt.SampleRate = (fmt.SampleRate * 4) / 5;
	} else {
		volume *= 0.50f;
		if (type == SOUND_METAL) fmt.SampleRate = (fmt.SampleRate * 7) / 5;
	}

	AudioHandle* outputs = NULL;
	if (fmt.Channels == 1) outputs = monoOutputs;
	if (fmt.Channels == 2) outputs = stereoOutputs;
	if (outputs == NULL) return; /* TODO: > 2 channel sound?? */
	Int32 i;

	/* Try to play on fresh device, or device with same data format */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		AudioHandle output = outputs[i];
		if (output == -1) {
			Audio_Init(&output, 1);
			outputs[i] = output;
		}
		if (!Audio_IsFinished(output)) continue;

		struct AudioFormat* l = Audio_GetFormat(output);
		if (l->Channels == 0 || AudioFormat_Eq(l, &fmt)) {
			Sounds_PlayRaw(output, snd, &fmt, volume); return;
		}
	}

	/* Try again with all devices, even if need to recreate one (expensive) */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		AudioHandle output = outputs[i];
		if (!Audio_IsFinished(output)) continue;

		Sounds_PlayRaw(output, snd, &fmt, volume); return;
	}
}

static void Audio_PlayBlockSound(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	if (block == BLOCK_AIR) {
		Audio_PlayDigSound(Block_DigSounds[oldBlock]);
	} else if (!Game_ClassicMode) {
		Audio_PlayDigSound(Block_StepSounds[block]);
	}
}

static void Sounds_FreeOutputs(AudioHandle* outputs) {
	bool anyPlaying = true;
	Int32 i;

	while (anyPlaying) {
		anyPlaying = false;
		for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
			if (outputs[i] == -1) continue;
			anyPlaying |= !Audio_IsFinished(outputs[i]);
		}
		if (anyPlaying) Thread_Sleep(1);
	}

	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		if (outputs[i] == -1) continue;
		Audio_Free(outputs[i]);
		outputs[i] = -1;
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

static ReturnCode Music_PlayOgg(struct Stream* source) {
	UInt8 buffer[OGG_BUFFER_SIZE];
	struct Stream stream;
	Ogg_MakeStream(&stream, buffer, source);

	struct VorbisState vorbis = { 0 };
	vorbis.Source = &stream;
	ReturnCode result = Vorbis_DecodeHeaders(&vorbis);
	if (result) return result;

	struct AudioFormat fmt;
	fmt.Channels      = vorbis.Channels;
	fmt.SampleRate    = vorbis.SampleRate;
	fmt.BitsPerSample = 16;
	Audio_SetFormat(music_out, &fmt);

	/* max possible result of a vorbis decode is blocksize1 */
	/* so we decode a bit over a second of audio each time */
	Int32 offset = 0, size = (Math_CeilDiv(fmt.SampleRate, vorbis.BlockSizes[1]) + 1) * vorbis.BlockSizes[1];
	Int16* data = Mem_Alloc((size * fmt.Channels) * AUDIO_MAX_CHUNKS, sizeof(Int16), "Ogg - final PCM output");

	Int32 i, next;
	for (;;) {
		next = -1;
		for (i = 0; i < AUDIO_MAX_CHUNKS; i++) {
			if (Audio_IsCompleted(music_out, i)) { next = i; break; }
		}

		if (next == -1) {
		} else if (music_pendingStop) {
			goto finished;
		} else {
			/* decode up to around a second */
			Int16* base = data + (size * fmt.Channels) * next;

			while (offset + vorbis.BlockSizes[1] < size) {
				result = Vorbis_DecodeFrame(&vorbis);
				if (result) goto finished;

				Int16* cur = &base[offset * fmt.Channels];
				offset += Vorbis_OutputFrame(&vorbis, cur);
			}

			Audio_PlayData(music_out, next, base, offset * fmt.Channels * sizeof(Int16));
			offset = 0;
		}
		Thread_Sleep(1);
	}

finished:
	/* Wait until the buffers finished playing */
	while (!Audio_IsFinished(music_out)) { Thread_Sleep(1); }

	Mem_Free(&data);
	Vorbis_Free(&vorbis);
	return result;
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
	UInt8 pathBuffer[String_BufferSize(FILENAME_SIZE)];

	while (!music_pendingStop) {
		Int32 idx = Random_Range(&rnd, 0, count);
		String filename = StringsBuffer_UNSAFE_Get(&files, musicFiles[idx]);
		String path = String_InitAndClearArray(pathBuffer);
		String_Format2(&path, "audio%r%s", &Directory_Separator, &filename);
		Platform_Log1("playing music file: %s", &filename);

		void* file; ReturnCode result = File_Open(&file, &path);
		ErrorHandler_CheckOrFail(result, "Music_RunLoop - opening file");
		struct Stream stream; Stream_FromFile(&stream, file, &path);
		{
			result = Music_PlayOgg(&stream);
			ErrorHandler_CheckOrFail(result, "Music_RunLoop - playing ogg");
		}
		result = stream.Close(&stream);
		ErrorHandler_CheckOrFail(result, "Music_RunLoop - closing file");

		if (music_pendingStop) break;
		Int32 delay = 1000 * 120 + Random_Range(&rnd, 0, 1000 * 300);
		Waitable_WaitFor(music_waitable, delay);
	}
}

static void Music_Init(void) {
	if (music_thread) { Audio_SetVolume(music_out, Game_MusicVolume / 100.0f); return; }

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
static Int32 AudioManager_GetVolume(const UChar* volKey, const UChar* boolKey) {
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
	StringsBuffer_Init(&files);
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
