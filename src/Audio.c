#include "Audio.h"
#include "String.h"
#include "Logger.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Game.h"
#include "Errors.h"
#include "Vorbis.h"
#include "Chat.h"
#include "Stream.h"
#include "Utils.h"
#include "Options.h"
#include "Deflate.h"
#ifdef CC_BUILD_ANDROID
/* TODO: Refactor maybe to not rely on checking WinInfo.Handle != NULL */
#include "Window.h"
#endif

int Audio_SoundsVolume, Audio_MusicVolume;
const cc_string Sounds_ZipPathMC = String_FromConst("audio/default.zip");
const cc_string Sounds_ZipPathCC = String_FromConst("audio/classicube.zip");
static const cc_string audio_dir = String_FromConst("audio");

struct Sound {
	int channels, sampleRate;
	struct AudioChunk chunk;
};


/*########################################################################################################################*
*--------------------------------------------------------Sounds-----------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NOSOUNDS
/* Can't use mojang's sound assets, so just stub everything out */
static void Sounds_Init(void) { }
static void Sounds_Free(void) { }
static void Sounds_Stop(void) { }
static void Sounds_Start(void) {
	Chat_AddRaw("&cSounds are not supported currently");
	Audio_SoundsVolume = 0;
}

void Audio_PlayDigSound(cc_uint8 type)  { }
void Audio_PlayStepSound(cc_uint8 type) { }
#else
#define AUDIO_MAX_SOUNDS 10

struct SoundGroup {
	int count;
	struct Sound sounds[AUDIO_MAX_SOUNDS];
};
struct Soundboard { struct SoundGroup groups[SOUND_COUNT]; };

static struct Soundboard digBoard, stepBoard;
static RNGState sounds_rnd;

#define WAV_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)
#define WAV_FMT_SIZE 16

static cc_result Sound_ReadWaveData(struct Stream* stream, struct Sound* snd) {
	cc_uint32 fourCC, size;
	cc_uint8 tmp[WAV_FMT_SIZE];
	cc_result res;
	int bitsPerSample;

	if ((res = Stream_Read(stream, tmp, 12)))  return res;
	fourCC = Stream_GetU32_BE(tmp + 0);
	if (fourCC == WAV_FourCC('I','D','3', 2))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.2 tags header */
	if (fourCC == WAV_FourCC('I','D','3', 3))  return AUDIO_ERR_MP3_SIG; /* ID3 v2.3 tags header */
	if (fourCC != WAV_FourCC('R','I','F','F')) return WAV_ERR_STREAM_HDR;

	/* tmp[4] (4) file size */
	fourCC = Stream_GetU32_BE(tmp + 8);
	if (fourCC != WAV_FourCC('W','A','V','E')) return WAV_ERR_STREAM_TYPE;

	for (;;) {
		if ((res = Stream_Read(stream, tmp, 8))) return res;
		fourCC = Stream_GetU32_BE(tmp + 0);
		size   = Stream_GetU32_LE(tmp + 4);

		if (fourCC == WAV_FourCC('f','m','t',' ')) {
			if ((res = Stream_Read(stream, tmp, sizeof(tmp)))) return res;
			if (Stream_GetU16_LE(tmp + 0) != 1) return WAV_ERR_DATA_TYPE;

			snd->channels   = Stream_GetU16_LE(tmp + 2);
			snd->sampleRate = Stream_GetU32_LE(tmp + 4);
			/* tmp[8] (6) alignment data and stuff */

			bitsPerSample = Stream_GetU16_LE(tmp + 14);
			if (bitsPerSample != 16) return WAV_ERR_SAMPLE_BITS;
			size -= WAV_FMT_SIZE;
		} else if (fourCC == WAV_FourCC('d','a','t','a')) {
			if ((res = Audio_AllocChunks(size, &snd->chunk, 1))) return res;
			res = Stream_Read(stream, (cc_uint8*)snd->chunk.data, size);

			#ifdef CC_BUILD_BIGENDIAN
			Utils_SwapEndian16((cc_int16*)snd->chunk.data, size / 2);
			#endif
			return res;
		}

		/* Skip over unhandled data */
		if (size && (res = stream->Skip(stream, size))) return res;
	}
}

static struct SoundGroup* Soundboard_FindGroup(struct Soundboard* board, const cc_string* name) {
	struct SoundGroup* groups = board->groups;
	int i;

	for (i = 0; i < SOUND_COUNT; i++) 
	{
		if (String_CaselessEqualsConst(name, Sound_Names[i])) return &groups[i];
	}
	return NULL;
}

static void Soundboard_Load(struct Soundboard* board, const cc_string* boardName, const cc_string* file, struct Stream* stream) {
	struct SoundGroup* group;
	struct Sound* snd;
	cc_string name = *file;
	cc_result res;
	int dotIndex;
	Utils_UNSAFE_TrimFirstDirectory(&name);

	/* dig_grass1.wav -> dig_grass1 */
	dotIndex = String_LastIndexOf(&name, '.');
	if (dotIndex >= 0) name.length = dotIndex;
	if (!String_CaselessStarts(&name, boardName)) return;

	/* Convert dig_grass1 to grass */
	name = String_UNSAFE_SubstringAt(&name, boardName->length);
	name = String_UNSAFE_Substring(&name, 0, name.length - 1);

	group = Soundboard_FindGroup(board, &name);
	if (!group) {
		Chat_Add1("&cUnknown sound group '%s'", &name); return;
	}
	if (group->count == Array_Elems(group->sounds)) {
		Chat_AddRaw("&cCannot have more than 10 sounds in a group"); return;
	}

	snd = &group->sounds[group->count];
	res = Sound_ReadWaveData(stream, snd);

	if (res) {
		Logger_SysWarn2(res, "decoding", file);
		Audio_FreeChunks(&snd->chunk, 1);
		snd->chunk.data = NULL;
		snd->chunk.size = 0;
	} else { group->count++; }
}

static const struct Sound* Soundboard_PickRandom(struct Soundboard* board, cc_uint8 type) {
	struct SoundGroup* group;
	int idx;

	if (type == SOUND_NONE || type >= SOUND_COUNT) return NULL;
	if (type == SOUND_METAL) type = SOUND_STONE;

	group = &board->groups[type];
	if (!group->count) return NULL;

	idx = Random_Next(&sounds_rnd, group->count);
	return &group->sounds[idx];
}


CC_NOINLINE static void Sounds_Fail(cc_result res) {
	Audio_Warn(res, "playing sounds");
	Chat_AddRaw("&cDisabling sounds");
	Audio_SetSounds(0);
}

static void Sounds_Play(cc_uint8 type, struct Soundboard* board) {
	const struct Sound* snd;
	struct AudioData data;
	cc_result res;

	if (type == SOUND_NONE || !Audio_SoundsVolume) return;
	snd = Soundboard_PickRandom(board, type);
	if (!snd) return;

	data.chunk      = snd->chunk;
	data.channels   = snd->channels;
	data.sampleRate = snd->sampleRate;
	data.rate       = 100;
	data.volume     = Audio_SoundsVolume;

	/* https://minecraft.wiki/w/Block_of_Gold#Sounds */
	/* https://minecraft.wiki/w/Grass#Sounds */
	if (board == &digBoard) {
		if (type == SOUND_METAL) data.rate = 120;
		else data.rate = 80;
	} else {
		data.volume /= 2;
		if (type == SOUND_METAL) data.rate = 140;
	}
	
	res = AudioPool_Play(&data);
	if (res) Sounds_Fail(res);
}

static void Audio_PlayBlockSound(void* obj, IVec3 coords, BlockID old, BlockID now) {
	if (now == BLOCK_AIR) {
		Audio_PlayDigSound(Blocks.DigSounds[old]);
	} else if (!Game_ClassicMode) {
		/* use StepSounds instead when placing, as don't want */
		/*  to play glass break sound when placing glass */
		Audio_PlayDigSound(Blocks.StepSounds[now]);
	}
}

static cc_bool SelectZipEntry(const cc_string* path) { return true; }
static cc_result ProcessZipEntry(const cc_string* path, struct Stream* stream, struct ZipEntry* source) {
	static const cc_string dig  = String_FromConst("dig_");
	static const cc_string step = String_FromConst("step_");
	
	Soundboard_Load(&digBoard,  &dig,  path, stream);
	Soundboard_Load(&stepBoard, &step, path, stream);
	return 0;
}

static cc_result Sounds_ExtractZip(const cc_string* path) {
	struct ZipEntry entries[128];
	struct Stream stream;
	cc_result res;

	res = Stream_OpenFile(&stream, path);
	if (res) { Logger_SysWarn2(res, "opening", path); return res; }

	res = Zip_Extract(&stream, SelectZipEntry, ProcessZipEntry,
						entries, Array_Elems(entries));
	if (res) Logger_SysWarn2(res, "extracting", path);

	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
	return res;
}

/* TODO this is a pretty terrible solution */
#ifdef CC_BUILD_WEBAUDIO
static const struct SoundID { int group; const char* name; } sounds_list[] =
{
	{ SOUND_CLOTH,  "step_cloth1"  }, { SOUND_CLOTH,  "step_cloth2"  }, { SOUND_CLOTH,  "step_cloth3"  }, { SOUND_CLOTH,  "step_cloth4"  },
	{ SOUND_GRASS,  "step_grass1"  }, { SOUND_GRASS,  "step_grass2"  }, { SOUND_GRASS,  "step_grass3"  }, { SOUND_GRASS,  "step_grass4"  },
	{ SOUND_GRAVEL, "step_gravel1" }, { SOUND_GRAVEL, "step_gravel2" }, { SOUND_GRAVEL, "step_gravel3" }, { SOUND_GRAVEL, "step_gravel4" },
	{ SOUND_SAND,   "step_sand1"   }, { SOUND_SAND,   "step_sand2"   }, { SOUND_SAND,   "step_sand3"   }, { SOUND_SAND,   "step_sand4"   },
	{ SOUND_SNOW,   "step_snow1"   }, { SOUND_SNOW,   "step_snow2"   }, { SOUND_SNOW,   "step_snow3"   }, { SOUND_SNOW,   "step_snow4"   },
	{ SOUND_STONE,  "step_stone1"  }, { SOUND_STONE,  "step_stone2"  }, { SOUND_STONE,  "step_stone3"  }, { SOUND_STONE,  "step_stone4"  },
	{ SOUND_WOOD,   "step_wood1"   }, { SOUND_WOOD,   "step_wood2"   }, { SOUND_WOOD,   "step_wood3"   }, { SOUND_WOOD,   "step_wood4"   },
	{ SOUND_NONE, NULL },

	{ SOUND_CLOTH,  "dig_cloth1"   }, { SOUND_CLOTH,  "dig_cloth2"   }, { SOUND_CLOTH,  "dig_cloth3"   }, { SOUND_CLOTH,  "dig_cloth4"   },
	{ SOUND_GRASS,  "dig_grass1"   }, { SOUND_GRASS,  "dig_grass2"   }, { SOUND_GRASS,  "dig_grass3"   }, { SOUND_GRASS,  "dig_grass4"   },
	{ SOUND_GLASS,  "dig_glass1"   }, { SOUND_GLASS,  "dig_glass2"   }, { SOUND_GLASS,  "dig_glass3"   },
	{ SOUND_GRAVEL, "dig_gravel1"  }, { SOUND_GRAVEL, "dig_gravel2"  }, { SOUND_GRAVEL, "dig_gravel3"  }, { SOUND_GRAVEL, "dig_gravel4"  },
	{ SOUND_SAND,   "dig_sand1"    }, { SOUND_SAND,   "dig_sand2"    }, { SOUND_SAND,   "dig_sand3"    }, { SOUND_SAND,   "dig_sand4"    },
	{ SOUND_SNOW,   "dig_snow1"    }, { SOUND_SNOW,   "dig_snow2"    }, { SOUND_SNOW,   "dig_snow3"    }, { SOUND_SNOW,   "dig_snow4"    },
	{ SOUND_STONE,  "dig_stone1"   }, { SOUND_STONE,  "dig_stone2"   }, { SOUND_STONE,  "dig_stone3"   }, { SOUND_STONE,  "dig_stone4"   },
	{ SOUND_WOOD,   "dig_wood1"    }, { SOUND_WOOD,   "dig_wood2"    }, { SOUND_WOOD,   "dig_wood3"    }, { SOUND_WOOD,   "dig_wood4"    },
};

/* TODO this is a terrible solution */
static void InitWebSounds(void) {
	struct Soundboard* board = &stepBoard;
	struct SoundGroup* group;
	int i;

	for (i = 0; i < Array_Elems(sounds_list); i++) {
		if (sounds_list[i].group == SOUND_NONE) {
			board = &digBoard;
		} else {
			group = &board->groups[sounds_list[i].group];
			group->sounds[group->count++].chunk.data = sounds_list[i].name;
		}
	}
}
#endif

static cc_bool sounds_loaded;
static void Sounds_Start(void) {
	cc_result res;
	if (!AudioBackend_Init()) { 
		AudioBackend_Free(); 
		Audio_SoundsVolume = 0; 
		return; 
	}

	if (sounds_loaded) return;
	sounds_loaded = true;
#ifdef CC_BUILD_WEBAUDIO
	InitWebSounds();
#else
	res = Sounds_ExtractZip(&Sounds_ZipPathMC);
	if (res == ReturnCode_FileNotFound)
		Sounds_ExtractZip(&Sounds_ZipPathCC);
#endif
}

static void Sounds_Stop(void) { AudioPool_Close(); }

static void Sounds_Init(void) {
	int volume = Options_GetInt(OPT_SOUND_VOLUME, 0, 100, DEFAULT_SOUNDS_VOLUME);
	Audio_SetSounds(volume);
	Event_Register_(&UserEvents.BlockChanged, NULL, Audio_PlayBlockSound);
}
static void Sounds_Free(void) { Sounds_Stop(); }

void Audio_PlayDigSound(cc_uint8 type)  { Sounds_Play(type, &digBoard); }
void Audio_PlayStepSound(cc_uint8 type) { Sounds_Play(type, &stepBoard); }
#endif


/*########################################################################################################################*
*--------------------------------------------------------Music------------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_NOMUSIC
/* Can't use mojang's music assets, so just stub everything out */
static void Music_Init(void) { }
static void Music_Free(void) { }
static void Music_Stop(void) { }
static void Music_Start(void) {
	Chat_AddRaw("&cMusic is not supported currently");
	Audio_MusicVolume = 0;
}
#else
static void* music_thread;
static void* music_waitable;
static volatile cc_bool music_stopping, music_joining;
static int music_minDelay, music_maxDelay;

static cc_result Music_Buffer(struct AudioChunk* chunk, int maxSamples, struct VorbisState* ctx) {
	int samples = 0;
	cc_int16* cur;
	cc_result res = 0, res2;
	cc_int16* data = (cc_int16*)chunk->data;

	while (samples < maxSamples) {
		if ((res = Vorbis_DecodeFrame(ctx))) break;

		cur = &data[samples];
		samples += Vorbis_OutputFrame(ctx, cur);
	}

	chunk->size = samples * 2;
	res2 = Audio_QueueChunk(&music_ctx, chunk);
	if (res2) { music_stopping = true; return res2; }
	return res;
}

static cc_result Music_PlayOgg(struct Stream* source) {
	struct OggState ogg;
	struct VorbisState vorbis;
	int channels, sampleRate, volume;

	int chunkSize, samplesPerSecond;
	struct AudioChunk chunks[AUDIO_MAX_BUFFERS] = { 0 };
	int inUse, i, cur;
	cc_result res;

	Ogg_Init(&ogg, source);
	Vorbis_Init(&vorbis);
	vorbis.source = &ogg;
	if ((res = Vorbis_DecodeHeaders(&vorbis))) goto cleanup;
	
	channels   = vorbis.channels;
	sampleRate = vorbis.sampleRate;
	if ((res = Audio_SetFormat(&music_ctx, channels, sampleRate, 100))) goto cleanup;

	/* largest possible vorbis frame decodes to blocksize1 * channels samples, */
	/*  so can end up decoding slightly over a second of audio */
	chunkSize        = channels * (sampleRate + vorbis.blockSizes[1]);
	samplesPerSecond = channels * sampleRate;

	if ((res = Audio_AllocChunks(chunkSize * 2, chunks, AUDIO_MAX_BUFFERS))) goto cleanup;
    volume = Audio_MusicVolume;
    Audio_SetVolume(&music_ctx, volume);	

	/* fill up with some samples before playing */
	for (i = 0; i < AUDIO_MAX_BUFFERS && !res; i++) 
	{
		res = Music_Buffer(&chunks[i], samplesPerSecond, &vorbis);
	}
	if (music_stopping) goto cleanup;

	res  = Audio_Play(&music_ctx);
	if (res) goto cleanup;
	cur  = 0;

	while (!music_stopping) {
#ifdef CC_BUILD_ANDROID
		/* Don't play music while in the background on Android */
    	/* TODO: Not use such a terrible approach */
    	if (!Window_Main.Handle.ptr) {
    		Audio_Pause(&music_ctx);
    		while (!Window_Main.Handle.ptr && !music_stopping) {
    			Thread_Sleep(10); continue;
    		}
    		Audio_Play(&music_ctx);
    	}
#endif
        if (volume != Audio_MusicVolume) {
            volume = Audio_MusicVolume;
            Audio_SetVolume(&music_ctx, volume);
        }

		res = Audio_Poll(&music_ctx, &inUse);
		if (res) { music_stopping = true; break; }

		if (inUse >= AUDIO_MAX_BUFFERS) {
			Thread_Sleep(10); continue;
		}

		res = Music_Buffer(&chunks[cur], samplesPerSecond, &vorbis);
		cur = (cur + 1) % AUDIO_MAX_BUFFERS;

		/* need to specially handle last bit of audio */
		if (res) break;
	}

	if (music_stopping) {
		/* must close audio context, as otherwise some of the audio */
		/*  context's internal audio buffers may have a reference */
		/*  to the `data` buffer which will be freed after this */
		Audio_Close(&music_ctx);
	} else {
		/* Wait until the buffers finished playing */
		for (;;) {
			if (Audio_Poll(&music_ctx, &inUse) || inUse == 0) break;
			Thread_Sleep(10);
		}
	}

cleanup:
	Audio_FreeChunks(chunks, AUDIO_MAX_BUFFERS);
	Vorbis_Free(&vorbis);
	return res == ERR_END_OF_STREAM ? 0 : res;
}

static void Music_AddFile(const cc_string* path, void* obj, int isDirectory) {
	struct StringsBuffer* files = (struct StringsBuffer*)obj;
	static const cc_string ogg  = String_FromConst(".ogg");

	if (isDirectory) {
		Directory_Enum(path, obj, Music_AddFile);
	} else if (String_CaselessEnds(path, &ogg)) {
		StringsBuffer_Add(files, path);
	}
}

static void Music_RunLoop(void) {
	struct StringsBuffer files;
	cc_string path;
	RNGState rnd;
	struct Stream stream;
	int idx, delay;
	cc_result res = 0;

	StringsBuffer_SetLengthBits(&files, STRINGSBUFFER_DEF_LEN_SHIFT);
	StringsBuffer_Init(&files);
	Directory_Enum(&audio_dir, &files, Music_AddFile);

	Random_SeedFromCurrentTime(&rnd);
	res = Audio_Init(&music_ctx, AUDIO_MAX_BUFFERS);
	if (res) music_stopping = true;

	while (!music_stopping && files.count) {
		idx  = Random_Next(&rnd, files.count);
		path = StringsBuffer_UNSAFE_Get(&files, idx);
		Platform_Log1("playing music file: %s", &path);

		res = Stream_OpenFile(&stream, &path);
		if (res) { Logger_SysWarn2(res, "opening", &path); break; }

		res = Music_PlayOgg(&stream);
		if (res) { Logger_SimpleWarn2(res, "playing", &path); }

		/* No point logging error for closing readonly file */
		(void)stream.Close(&stream);

		if (music_stopping) break;
		delay = Random_Range(&rnd, music_minDelay, music_maxDelay);
		Waitable_WaitFor(music_waitable, delay);
	}

	if (res) {
		Chat_AddRaw("&cDisabling music");
		Audio_MusicVolume = 0;
	}
	Audio_Close(&music_ctx);
	StringsBuffer_Clear(&files);

	if (music_joining) return;
	Thread_Detach(music_thread);
	music_thread = NULL;
}

static void Music_Start(void) {
	if (music_thread) return;
	if (!AudioBackend_Init()) {
		AudioBackend_Free(); 
		Audio_MusicVolume = 0;
		return; 
	}

	music_joining  = false;
	music_stopping = false;

	Thread_Run(&music_thread, Music_RunLoop, 256 * 1024, "Music");
}

static void Music_Stop(void) {
	music_joining  = true;
	music_stopping = true;
	Waitable_Signal(music_waitable);
	
	if (music_thread) Thread_Join(music_thread);
	music_thread = NULL;
}

static void Music_Init(void) {
	int volume;
	/* music is delayed between 2 - 7 minutes by default */
	music_minDelay = Options_GetInt(OPT_MIN_MUSIC_DELAY, 0, 3600, 120) * MILLIS_PER_SEC;
	music_maxDelay = Options_GetInt(OPT_MAX_MUSIC_DELAY, 0, 3600, 420) * MILLIS_PER_SEC;
	music_waitable = Waitable_Create("Music sleep");

	volume = Options_GetInt(OPT_MUSIC_VOLUME, 0, 100, DEFAULT_MUSIC_VOLUME);
	Audio_SetMusic(volume);
}

static void Music_Free(void) {
	Music_Stop();
	Waitable_Free(music_waitable);
}
#endif


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
void Audio_SetSounds(int volume) {
	Audio_SoundsVolume = volume;
	if (volume) Sounds_Start();
	else        Sounds_Stop();
}

void Audio_SetMusic(int volume) {
	Audio_MusicVolume = volume;
	if (volume) Music_Start();
	else        Music_Stop();
}

static void OnInit(void) {
	Sounds_Init();
	Music_Init();
}

static void OnFree(void) {
	Sounds_Free();
	Music_Free();
	AudioBackend_Free();
}

struct IGameComponent Audio_Component = {
	OnInit, /* Init  */
	OnFree  /* Free  */
};
