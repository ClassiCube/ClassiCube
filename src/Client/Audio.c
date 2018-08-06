#include "Audio.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Block.h"
#include "Game.h"

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
	String Name; UInt8 Count;
	struct Sound Sounds[AUDIO_MAX_SOUNDS];
};

struct Soundboard {
	Random Rnd; UInt8 Count;
	struct SoundGroup Groups[AUDIO_MAX_SOUNDS];
};

#define WAV_FourCC(a, b, c, d) (((UInt32)a << 24) | ((UInt32)b << 16) | ((UInt32)c << 8) | (UInt32)d)
enum WAV_ERR {
	WAV_ERR_STREAM_HDR = 329701, WAV_ERR_STREAM_TYPE, WAV_ERR_DATA_TYPE, WAV_ERR_NO_DATA
};

static ReturnCode Sound_ReadWaveData(struct Stream* stream, struct Sound* snd) {
	UInt32 fourCC, size, pos, len;
	ReturnCode res;

	fourCC = Stream_ReadU32_LE(stream);
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;
	Stream_ReadU32_LE(stream); /* file size, but we don't care */
	fourCC = Stream_ReadU32_LE(stream);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	while (!(res = stream->Position(stream, &pos)) && !(res = stream->Length(stream, &len)) && len < pos) {
		fourCC = Stream_ReadU32_LE(stream);
		size   = Stream_ReadU32_LE(stream);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			if (Stream_GetU16_LE(stream) != 1) return WAV_ERR_DATA_TYPE;

			snd->Format.Channels      = Stream_ReadU16_LE(stream);
			snd->Format.SampleRate    = Stream_ReadU32_LE(stream);
			Stream_Skip(&stream, 6);
			snd->Format.BitsPerSample = Stream_ReadU16_LE(stream);
			size -= 16;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->Data = Platform_MemAlloc(size, sizeof(UInt8), "WAV sound data");
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
	String_Format2(&path, "audio%r%s", &Platform_DirectorySeparator, filename);

	void* file = NULL;
	ReturnCode result = Platform_FileOpen(&file, &path);
	if (result) return result;
	ReturnCode fileResult = 0;

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
		String name = StringsBuffer_UNSAFE_Get(files, i);
		Utils_UNSAFE_GetFilename(&name);

		/* dig_grass1.wav -> dig_grass1 */
		Int32 dotIndex = String_LastIndexOf(&name, '.');
		if (dotIndex >= 0) { name = String_UNSAFE_Substring(0, &name, dotIndex); }
		if (!String_CaselessStarts(&name, boardName)) continue;

		/* Convert dig_grass1 to grass */
		name = String_UNSAFE_SubstringAt(&name, boardName->length);
		name = String_UNSAFE_Substring(&name, 0, name.length - 1);

		struct SoundGroup* group = Soundboard_Find(board, &name);
		if (group == NULL) {
			if (board->Count == AUDIO_MAX_SOUNDS) ErrorHandler_Fail("Soundboard_Init - too many sounds");
			group = &board->Groups[board->Count++]; 
			group.Name = name;
		}

		Sound snd = ReadWave(files[i]);
		group.Sounds.Add(snd);
	}
}

struct Sound* Soundboard_PickRandom(struct Soundboard* board, UInt8 type) {
	if (type == SOUND_NONE || type >= SOUND_COUNT) return NULL;
	if (type == SOUND_METAL) type = SOUND_STONE;
	string name = SoundType.Names[type];

	struct SoundGroup* group = Soundboard_Find(board, name);
	if (group == NULL) return NULL;
	return group.Sounds[rnd.Next(group.Sounds.Count)];
}


/*########################################################################################################################*
*--------------------------------------------------------Sounds-----------------------------------------------------------*
*#########################################################################################################################*/
struct Soundboard digBoard, stepBoard;
#define AUDIO_MAX_HANDLES 6
AudioHandle monoOutputs[AUDIO_MAX_HANDLES]   = { -1, -1, -1, -1, -1, -1 };
AudioHandle stereoOutputs[AUDIO_MAX_HANDLES] = { -1, -1, -1, -1, -1, -1 };

static void PlaySound(AudioHandle output, Real32 volume) {
	try {
		output.SetVolume(volume);
		output.PlayRawAsync(chunk);
	}
	catch (InvalidOperationException ex) {
		ErrorHandler.LogError("AudioPlayer.PlayCurrentSound()", ex);
		if (ex.Message == "No audio devices found")
			game.Chat.Add("&cNo audio devices found, disabling sounds.");
		else
			game.Chat.Add("&cAn error occured when trying to play sounds, disabling sounds.");

		SetSounds(0);
		game.SoundsVolume = 0;
	}
}

AudioChunk chunk = new AudioChunk();
static void PlaySound(UInt8 type, struct Soundboard* board) {
	if (type == SOUND_NONE || Game_SoundsVolume == 0) return;
	struct Sound* snd = Soundboard_PickRandom(board, type);
	if (snd == NULL) return;

	struct AudioFormat fmt = snd->Format;
	chunk.BytesUsed = snd.Data.Length;
	chunk.Data = snd.Data;

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
			Platform_AudioInit(&output, 1);
			outputs[i] = output;
		}
		if (!Platform_AudioIsFinished(output)) continue;

		struct AudioFormat* l = Platform_AudioGetFormat(output);
		if (l->Channels == 0 || AudioFormat_Eq(l, &fmt)) {
			PlaySound(output, volume); return;
		}
	}

	/* Try again with all devices, even if need to recreate one (expensive) */
	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		AudioHandle output = outputs[i];
		if (!Platform_AudioIsFinished(output)) continue;

		PlaySound(output, volume); return;
	}
}

static void Audio_PlayBlockSound(void* obj, Vector3I coords, BlockID oldBlock, BlockID block) {
	if (block == BLOCK_AIR) {
		PlayDigSound(Block_DigSounds[oldBlock]);
	} else if (!Game_ClassicMode) {
		PlayDigSound(Block_StepSounds[block]);
	}
}

static void Audio_FreeOutputs(AudioHandle* outputs) {
	bool anyPlaying = true;
	Int32 i;

	while (anyPlaying) {
		anyPlaying = false;
		for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
			if (outputs[i] == -1) continue;
			anyPlaying |= !Platform_AudioIsFinished(outputs[i]);
		}
		if (anyPlaying) Platform_ThreadSleep(1);
	}

	for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
		if (outputs[i] == -1) continue;
		Platform_AudioFree(outputs[i]);
		outputs[i] = -1;
	}
}

static void Audio_InitSounds(void) {
	if (digBoard.Count || stepBoard.Count) return;
	String dig  = String_FromConst("dig_");
	Soundboard_Init(&digBoard, &dig, &files);
	String step = String_FromConst("step_");
	Soundboard_Init(&stepBoard, &step, &files);
}

static void Audio_FreeSounds(void) {
	Audio_FreeOutputs(monoOutputs);
	Audio_FreeOutputs(stereoOutputs);
}

void Audio_SetSounds(Int32 volume) {
	if (volume) Audio_InitSounds();
	else        Audio_FreeSounds();
}

void Audio_PlayDigSound(UInt8 type)  { Audio_PlaySound(type, &digBoard); }
void Audio_PlayStepSound(UInt8 type) { Audio_PlaySound(type, &stepBoard); }


/*########################################################################################################################*
*--------------------------------------------------------Music------------------------------------------------------------*
*#########################################################################################################################*/
AudioHandle musicOut = -1;
string[] files, musicFiles;
Thread musicThread;

void SetMusic(int volume) {
	if (volume > 0) InitMusic();
	else DisposeMusic();
}

void InitMusic() {
	if (musicThread != null) { musicOut.SetVolume(game.MusicVolume / 100.0f); return; }

	int musicCount = 0;
	for (int i = 0; i < files.Length; i++) {
		if (Utils.CaselessEnds(files[i], ".ogg")) musicCount++;
	}

	musicFiles = new string[musicCount];
	for (int i = 0, j = 0; i < files.Length; i++) {
		if (!Utils.CaselessEnds(files[i], ".ogg")) continue;
		musicFiles[j] = files[i]; j++;
	}

	disposingMusic = false;
	musicOut = GetPlatformOut();
	musicOut.Create(10);
	musicThread = MakeThread(DoMusicThread, "ClassicalSharp.DoMusic");
}

EventWaitHandle musicHandle = new EventWaitHandle(false, EventResetMode.AutoReset);
void DoMusicThread() {
	if (musicFiles.Length == 0) return;
	Random rnd = new Random();
	while (!disposingMusic) {
		string file = musicFiles[rnd.Next(0, musicFiles.Length)];
		Utils.LogDebug("playing music file: " + file);

		string path = Path.Combine("audio", file);
		using (Stream fs = Platform.FileOpen(path)) {
			OggContainer container = new OggContainer(fs);
			try {
				musicOut.SetVolume(game.MusicVolume / 100.0f);
				musicOut.PlayStreaming(container);
			} catch (InvalidOperationException ex) {
				HandleMusicError(ex);
				return;
			} catch (Exception ex) {
				ErrorHandler.LogError("AudioPlayer.DoMusicThread()", ex);
				game.Chat.Add("&cError while trying to play music file " + file);
			}
		}
		if (disposingMusic) break;

		int delay = 2000 * 60 + rnd.Next(0, 5000 * 60);
		musicHandle.WaitOne(delay, false);
	}
}

void HandleMusicError(InvalidOperationException ex) {
	ErrorHandler.LogError("AudioPlayer.DoMusicThread()", ex);
	if (ex.Message == "No audio devices found")
		game.Chat.Add("&cNo audio devices found, disabling music.");
	else
		game.Chat.Add("&cAn error occured when trying to play music, disabling music.");

	SetMusic(0);
	game.MusicVolume = 0;
}
bool disposingMusic;

void DisposeMusic() {
	disposingMusic = true;
	musicHandle.Set();
	DisposeOf(ref musicOut, ref musicThread);
}

Thread MakeThread(ThreadStart func, string name) {
	Thread thread = new Thread(func);
	thread.Name = name;
	thread.IsBackground = true;
	thread.Start();
	return thread;
}

void DisposeOf(ref IAudioOutput output, ref Thread thread) {
	if (output == null) return;
	output.Stop();
	thread.Join();

	output.Dispose();
	output = null;
	thread = null;
}


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
static Int32 Audio_GetVolume(const UChar* volKey, const UChar* boolKey) {
	Int32 volume = Options_GetInt(volKey, 0, 100, 0);
	if (volume) return volume;

	volume = Options_GetBool(boolKey, false) ? 100 : 0;
	Options_Set(boolKey, NULL);
	return volume;
}

static void Audio_Init(void) {
	StringsBuffer_Init(&files);
	String path = String_FromConst("audio");
	if (Platform_DirectoryExists(&path)) {
		files = Platform.DirectoryFiles("audio");
	}

	Game_MusicVolume = GetVolume(OPT_MUSIC_VOLUME, OPT_USE_MUSIC);
	Audio_SetMusic(Game_MusicVolume);
	Game_SoundsVolume = GetVolume(OPT_SOUND_VOLUME, OPT_USE_SOUND);
	Audio_SetSounds(Game_SoundsVolume);
	game.UserEvents.BlockChanged += PlayBlockSound;
}

static void Audio_Free(void) {
	DisposeMusic();
	DisposeSound();
	musicHandle.Close();
	game.UserEvents.BlockChanged -= PlayBlockSound;
}
