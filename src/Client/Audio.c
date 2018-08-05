#include "Audio.h"
#include "ErrorHandler.h"
#include "Platform.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Block.h"
#include "Game.h"

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

	fourCC = Stream_ReadU32_LE(&stream);
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;
	Stream_ReadU32_LE(&stream); /* file size, but we don't care */
	fourCC = Stream_ReadU32_LE(&stream);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	while (!(res = stream->Position(&stream, &pos)) && !(res = stream->Length(&stream, &len)) && len < pos) {
		fourCC = Stream_ReadU32_LE(&stream);
		size   = Stream_ReadU32_LE(&stream);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			if (Stream_GetU16_LE(&stream) != 1) return WAV_ERR_DATA_TYPE;

			snd->Format.Channels      = Stream_ReadU16_LE(&stream);
			snd->Format.Frequency     = Stream_ReadU32_LE(&stream);
			Stream_Skip(&stream, 6);
			snd->Format.BitsPerSample = Stream_ReadU16_LE(&stream);
			size -= 16;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			snd->Data = Platform_MemAlloc(size, sizeof(UInt8), "WAV sound data");
			Stream_Read(&stream, snd->Data, size);
			return 0;
		}

		/* Skip over unhandled data */
		if (size) Stream_Skip(&stream, size);
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


/*########################################################################################################################*
*-----------------------------------------------------Audio sounds--------------------------------------------------------*
*#########################################################################################################################*/
struct Soundboard digBoard, stepBoard;
#define AUDIO_MAX_HANDLES 6

void Audio_SetSounds(Int32 volume) {
	if (volume > 0) InitSound();
	else DisposeSound();
}

static void Audio_InitSound(void) {
	if (digBoard == null) InitSoundboards();
	monoOutputs = new IAudioOutput[maxSounds];
	stereoOutputs = new IAudioOutput[maxSounds];
}

static void Audio_InitSoundboards(void) {
	digBoard = new Soundboard();
	digBoard.Init("dig_", files);
	stepBoard = new Soundboard();
	stepBoard.Init("step_", files);
}

static void Audio_PlayBlockSound(object sender, BlockChangedEventArgs e) {
	if (e.Block == BLOCK_AIR) {
		PlayDigSound(Block_DigSounds[e.OldBlock]);
	} else if (!Game_ClassicMode) {
		PlayDigSound(Block_StepSounds[e.Block]);
	}
}

AudioChunk chunk = new AudioChunk();
static void PlaySound(byte type, Soundboard board) {
	if (type == SoundType.None || monoOutputs == null) return;
	Sound snd = board.PickRandomSound(type);
	if (snd == null) return;

	chunk.Channels = snd.Channels;
	chunk.BitsPerSample = snd.BitsPerSample;
	chunk.BytesOffset = 0;
	chunk.BytesUsed = snd.Data.Length;
	chunk.Data = snd.Data;

	float volume = game.SoundsVolume / 100.0f;
	if (board == digBoard) {
		if (type == SoundType.Metal) chunk.SampleRate = (snd.SampleRate * 6) / 5;
		else chunk.SampleRate = (snd.SampleRate * 4) / 5;
	} else {
		volume *= 0.50f;

		if (type == SoundType.Metal) chunk.SampleRate = (snd.SampleRate * 7) / 5;
		else chunk.SampleRate = snd.SampleRate;
	}

	if (snd.Channels == 1) {
		PlayCurrentSound(monoOutputs, volume);
	} else if (snd.Channels == 2) {
		PlayCurrentSound(stereoOutputs, volume);
	}
}

void Audio_PlayDigSound(UInt8 type)  { Audio_PlaySound(type, &digBoard); }
void Audio_PlayStepSound(UInt8 type) { Audio_PlaySound(type, &stepBoard); }


IAudioOutput firstSoundOut;
static void PlayCurrentSound(IAudioOutput[] outputs, float volume) {
	for (int i = 0; i < monoOutputs.Length; i++) {
		IAudioOutput output = outputs[i];
		if (output == null) output = MakeSoundOutput(outputs, i);
		if (!output.DoneRawAsync()) continue;

		LastChunk l = output.Last;
		if (l.Channels == 0 || (l.Channels == chunk.Channels && l.BitsPerSample == chunk.BitsPerSample
			&& l.SampleRate == chunk.SampleRate)) {
			PlaySound(output, volume); return;
		}
	}

	// This time we try to play the sound on all possible devices,
	// even if it requires the expensive case of recreating a device
	for (int i = 0; i < monoOutputs.Length; i++) {
		IAudioOutput output = outputs[i];
		if (!output.DoneRawAsync()) continue;

		PlaySound(output, volume); return;
	}
}


static IAudioOutput MakeSoundOutput(IAudioOutput[] outputs, int i) {
	IAudioOutput output = GetPlatformOut();
	output.Create(1, firstSoundOut);
	if (firstSoundOut == null)
		firstSoundOut = output;

	outputs[i] = output;
	return output;
}

static void PlaySound(IAudioOutput output, float volume) {
	try {
		output.SetVolume(volume);
		output.PlayRawAsync(chunk);
	} catch (InvalidOperationException ex) {
		ErrorHandler.LogError("AudioPlayer.PlayCurrentSound()", ex);
		if (ex.Message == "No audio devices found")
			game.Chat.Add("&cNo audio devices found, disabling sounds.");
		else
			game.Chat.Add("&cAn error occured when trying to play sounds, disabling sounds.");

		SetSounds(0);
		game.SoundsVolume = 0;
	}
}

static void DisposeSound() {
	DisposeOutputs(ref monoOutputs);
	DisposeOutputs(ref stereoOutputs);
	if (firstSoundOut != null) {
		firstSoundOut.Dispose();
		firstSoundOut = null;
	}
}

static void DisposeOutputs(ref IAudioOutput[] outputs) {
	if (outputs == null) return;
	bool soundPlaying = true;

	while (soundPlaying) {
		soundPlaying = false;
		for (int i = 0; i < outputs.Length; i++) {
			if (outputs[i] == null) continue;
			soundPlaying |= !outputs[i].DoneRawAsync();
		}
		if (soundPlaying)
			Thread.Sleep(1);
	}

	for (int i = 0; i < outputs.Length; i++) {
		if (outputs[i] == null || outputs[i] == firstSoundOut) continue;
		outputs[i].Dispose();
	}
	outputs = null;
}

// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.IO;
using System.Threading;
using SharpWave;
using SharpWave.Codecs.Vorbis;

namespace ClassicalSharp.Audio{

	public sealed partial class AudioPlayer : IGameComponent {

	IAudioOutput musicOut;
	IAudioOutput[] monoOutputs, stereoOutputs;
	string[] files, musicFiles;
	Thread musicThread;
	Game game;

	void IGameComponent.Init(Game game) {
		this.game = game;
		if (Platform.DirectoryExists("audio")) {
			files = Platform.DirectoryFiles("audio");
		} else {
			files = new string[0];
		}

		game.MusicVolume = GetVolume(OptionsKey.MusicVolume, OptionsKey.UseMusic);
		SetMusic(game.MusicVolume);
		game.SoundsVolume = GetVolume(OptionsKey.SoundsVolume, OptionsKey.UseSound);
		SetSounds(game.SoundsVolume);
		game.UserEvents.BlockChanged += PlayBlockSound;
	}

	static int GetVolume(string volKey, string boolKey) {
		int volume = Options.GetInt(volKey, 0, 100, 0);
		if (volume != 0) return volume;

		volume = Options.GetBool(boolKey, false) ? 100 : 0;
		Options.Set(boolKey, null);
		return volume;
	}

	public void SetMusic(int volume) {
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
	void IDisposable.Dispose() {
		DisposeMusic();
		DisposeSound();
		musicHandle.Close();
		game.UserEvents.BlockChanged -= PlayBlockSound;
	}

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

	IAudioOutput GetPlatformOut() {
		if (OpenTK.Configuration.RunningOnWindows && !Options.GetBool(OptionsKey.ForceOpenAL, false))
			return new WinMmOut();
		return new OpenALOut();
	}

	void DisposeOf(ref IAudioOutput output, ref Thread thread) {
		if (output == null) return;
		output.Stop();
		thread.Join();

		output.Dispose();
		output = null;
		thread = null;
	}
}
}
