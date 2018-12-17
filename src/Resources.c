#include "Resources.h"
#include "Funcs.h"
#include "String.h"
#include "Constants.h"
#include "Deflate.h"
#include "Stream.h"
#include "Platform.h"
#include "Launcher.h"
#include "Utils.h"
#include "Vorbis.h"
#include "Errors.h"

/*########################################################################################################################*
*---------------------------------------------------------List/Checker----------------------------------------------------*
*#########################################################################################################################*/
bool SoundsExist;
int Resources_Count, Resources_Size;

static void Resources_CheckFiles(void) {
	int i, flags;
	
	flags = Resources_GetFetchFlags();
	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (!(flags & Resources_Files[i].Flag)) continue;

		Resources_Size += Resources_Files[i].Size;
		Resources_Count++;
	}
}

static void Resources_CheckMusic(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i;
	String_InitArray(path, pathBuffer);

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		path.length = 0;
		String_Format1(&path, "audio/%c", Resources_Music[i].Name);

		Resources_Music[i].Exists = File_Exists(&path);
		if (Resources_Music[i].Exists) continue;

		Resources_Size += Resources_Music[i].Size;
		Resources_Count++;
	}
}

static void Resources_CheckSounds(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i;
	String_InitArray(path, pathBuffer);

	for (i = 0; i < Array_Elems(Resources_Sounds); i++) {
		path.length = 0;
		String_Format1(&path, "audio/%c.wav", Resources_Sounds[i].Name);

		if (File_Exists(&path)) continue;
		SoundsExist = false;

		Resources_Count += Array_Elems(Resources_Sounds);
		Resources_Size  += 417;
		return;
	}
	SoundsExist = true;
}

static bool Resources_SelectZipEntry(const String* path) {
	String name;
	int i;

	name = *path;
	Utils_UNSAFE_GetFilename(&name);
	for (i = 0; i < Array_Elems(Resources_Textures); i++) {
		if (Resources_Textures[i].Exists) continue;

		if (!String_CaselessEqualsConst(&name, Resources_Textures[i].Filename)) continue;
		Resources_Textures[i].Exists = true;
		break;
	}
	return false;
}

static void Resources_CheckDefaultZip(void) {
	const static String path = String_FromConst("texpacks/default.zip");
	struct Stream stream;
	struct ZipState state;
	ReturnCode res;

	if (!File_Exists(&path)) return;
	res = Stream_OpenFile(&stream, &path);

	if (res) { Launcher_ShowError(res, "checking default.zip"); return; }
	Zip_Init(&state, &stream);
	state.SelectEntry = Resources_SelectZipEntry;

	res = Zip_Extract(&state);
	stream.Close(&stream);
	if (res) { Launcher_ShowError(res, "inspecting default.zip"); }
}

int Resources_GetFetchFlags(void) {
	int flags = 0, i;
	for (i = 0; i < Array_Elems(Resources_Textures); i++) {
		if (Resources_Textures[i].Exists) continue;

		flags |= Resources_Textures[i].Flags;
	}
	return flags;
}

void Resources_CheckExistence(void) {
	Resources_CheckDefaultZip();
	Resources_CheckFiles();
	Resources_CheckMusic();
	Resources_CheckSounds();
}

struct ResourceFile Resources_Files[4] = {
	{ "classic jar", "http://launcher.mojang.com/mc/game/c0.30_01c/client/54622801f5ef1bcc1549a842c5b04cb5d5583005/client.jar",  291, FLAG_CLASSIC },
	{ "1.6.2 jar",   "http://launcher.mojang.com/mc/game/1.6.2/client/b6cb68afde1d9cf4a20cbf27fa90d0828bf440a4/client.jar",     4621, FLAG_MODERN },
	{ "gui.png patch",     "http://static.classicube.net/terrain-patch2.png",  7, FLAG_GUI },
	{ "terrain.png patch", "http://static.classicube.net/gui.png",            21, FLAG_TERRAIN }
};

struct ResourceTexture Resources_Textures[19] = {
	/* classic jar files */
	{ "char.png",       FLAG_CLASSIC }, { "clouds.png",      FLAG_CLASSIC },
	{ "default.png",    FLAG_CLASSIC }, { "particles.png",   FLAG_CLASSIC },
	{ "rain.png",       FLAG_CLASSIC }, { "gui_classic.png", FLAG_CLASSIC },
	{ "icons.png",      FLAG_CLASSIC }, { "terrain.png",     FLAG_CLASSIC | FLAG_TERRAIN | FLAG_MODERN },
	{ "creeper.png",    FLAG_CLASSIC }, { "pig.png",         FLAG_CLASSIC },
	{ "sheep.png",      FLAG_CLASSIC }, { "sheep_fur.png",   FLAG_CLASSIC },
	{ "skeleton.png",   FLAG_CLASSIC }, { "spider.png",      FLAG_CLASSIC },
	{ "zombie.png",     FLAG_CLASSIC }, /* "arrows.png", "sign.png" */
	/* other files */
	{ "snow.png",       FLAG_MODERN },  { "chicken.png",     FLAG_MODERN },
	{ "animations.png", FLAG_MODERN },  { "gui.png",         FLAG_GUI }
};

struct ResourceSound Resources_Sounds[59] = {
	{ "dig_cloth1",  "5fd568d724ba7d53911b6cccf5636f859d2662e8" }, { "dig_cloth2",  "56c1d0ac0de2265018b2c41cb571cc6631101484" },
	{ "dig_cloth3",  "9c63f2a3681832dc32d206f6830360bfe94b5bfc" }, { "dig_cloth4",  "55da1856e77cfd31a7e8c3d358e1f856c5583198" },
	{ "dig_grass1",  "41cbf5dd08e951ad65883854e74d2e034929f572" }, { "dig_grass2",  "86cb1bb0c45625b18e00a64098cd425a38f6d3f2" },
	{ "dig_grass3",  "f7d7e5c7089c9b45fa5d1b31542eb455fad995db" }, { "dig_grass4",  "c7b1005d4926f6a2e2387a41ab1fb48a72f18e98" },
	{ "dig_gravel1", "e8b89f316f3e9989a87f6e6ff12db9abe0f8b09f" }, { "dig_gravel2", "c3b3797d04cb9640e1d3a72d5e96edb410388fa3" },
	{ "dig_gravel3", "48f7e1bb098abd36b9760cca27b9d4391a23de26" }, { "dig_gravel4", "7bf3553a4fe41a0078f4988a13d6e1ed8663ef4c" },
	{ "dig_sand1",   "9e59c3650c6c3fc0a475f1b753b2fcfef430bf81" }, { "dig_sand2",   "0fa4234797f336ada4e3735e013e44d1099afe57" },
	{ "dig_sand3",   "c75589cc0087069f387de127dd1499580498738e" }, { "dig_sand4",   "37afa06f97d58767a1cd1382386db878be1532dd" },
	{ "dig_snow1",   "e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c" }, { "dig_snow2",   "5887d10234c4f244ec5468080412f3e6ef9522f3" },
	{ "dig_snow3",   "a4bc069321a96236fde04a3820664cc23b2ea619" }, { "dig_snow4",   "e26fa3036cdab4c2264ceb19e1cd197a2a510227" },
	{ "dig_stone1",  "4e094ed8dfa98656d8fec52a7d20c5ee6098b6ad" }, { "dig_stone2",  "9c92f697142ae320584bf64c0d54381d59703528" },
	{ "dig_stone3",  "8f23c02475d388b23e5faa680eafe6b991d7a9d4" }, { "dig_stone4",  "363545a76277e5e47538b2dd3a0d6aa4f7a87d34" },
	{ "dig_wood1",   "9bc2a84d0aa98113fc52609976fae8fc88ea6333" }, { "dig_wood2",   "98102533e6085617a2962157b4f3658f59aea018" },
	{ "dig_wood3",   "45b2aef7b5049e81b39b58f8d631563fadcc778b" }, { "dig_wood4",   "dc66978374a46ab2b87db6472804185824868095" },
	{ "dig_glass1",  "7274a2231ed4544a37e599b7b014e589e5377094" }, { "dig_glass2",  "87c47bda3645c68f18a49e83cbf06e5302d087ff" },
	{ "dig_glass3",  "ad7d770b7fff3b64121f75bd60cecfc4866d1cd6" },

	{ "step_cloth1",  "5fd568d724ba7d53911b6cccf5636f859d2662e8" }, { "step_cloth2",  "56c1d0ac0de2265018b2c41cb571cc6631101484" },
	{ "step_cloth3",  "9c63f2a3681832dc32d206f6830360bfe94b5bfc" }, { "step_cloth4",  "55da1856e77cfd31a7e8c3d358e1f856c5583198" },
	{ "step_grass1",  "227ab99bf7c6cf0b2002e0f7957d0ff7e5cb0c96" }, { "step_grass2",  "5c971029d9284676dce1dda2c9d202f8c47163b2" },
	{ "step_grass3",  "76de0a736928eac5003691d73bdc2eda92116198" }, { "step_grass4",  "bc28801224a0aa77fdc93bb7c6c94beacdf77331" },
	{ "step_gravel1", "1d761cb3bcb45498719e4fba0751e1630e134f1a" }, { "step_gravel2", "ac7a7c8d106e26abc775b1b46150c083825d8ddc" },
	{ "step_gravel3", "c109b985a7e6d5d3828c92e00aefa49deca0eb8c" }, { "step_gravel4", "a47adece748059294c5f563c0fcac02fa0e4bab4" },
	{ "step_sand1",   "9813c8185197f4a4296649f27a9d738c4a6dfc78" }, { "step_sand2",   "bd1750c016f6bab40934eff0b0fb64c41c86e44b" },
	{ "step_sand3",   "ab07279288fa49215bada5c17627e6a54ad0437c" }, { "step_sand4",   "a474236fb0c75bd65a6010e87dbc000d345fc185" },
	{ "step_snow1",   "e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c" }, { "step_snow2",   "5887d10234c4f244ec5468080412f3e6ef9522f3" },
	{ "step_snow3",   "a4bc069321a96236fde04a3820664cc23b2ea619" }, { "step_snow4",   "e26fa3036cdab4c2264ceb19e1cd197a2a510227" },
	{ "step_stone1",  "4a2e3795ffd4d3aab0834b7e41903af3a8f7d197" }, { "step_stone2",  "22a383d9c22342305a4f16fec0bb479a885f8da2" },
	{ "step_stone3",  "a533e7ae975e62592de868e0d0572778614bd587" }, { "step_stone4",  "d5218034051a13322d7b5efc0b5a9af739615f04" },
	{ "step_wood1",   "afb01196e2179e3b15b48f3373cea4c155d56b84" }, { "step_wood2",   "1e82a43c30cf8fcbe05d0bc2760ecba5d2320314" },
	{ "step_wood3",   "27722125968ac60c0f191a961b17e406f1351c6e" }, { "step_wood4",   "29586f60bfe6f521dbc748919d4f0dc5b28beefd" }
};

struct ResourceMusic Resources_Music[7] = {
	{ "calm1.ogg", "50a59a4f56e4046701b758ddbb1c1587efa4cadf", 2472 },
	{ "calm2.ogg", "74da65c99aa578486efa7b69983d3533e14c0d6e", 1931 },
	{ "calm3.ogg", "14ae57a6bce3d4254daa8be2b098c2d99743cc3f", 2181 },
	{ "hal1.ogg",  "df1ff11b79757432c5c3f279e5ecde7b63ceda64", 1926 },
	{ "hal2.ogg",  "ceaaaa1d57dfdfbb0bd4da5ea39628b42897a687", 1714 },
	{ "hal3.ogg",  "dd85fb564e96ee2dbd4754f711ae9deb08a169f9", 1879 },
	{ "hal4.ogg",  "5e7d63e75c6e042f452bc5e151276911ef92fed8", 2499 }
};


/*########################################################################################################################*
*-------------------------------------------------------Texture patcher---------------------------------------------------*
*#########################################################################################################################*/


/*########################################################################################################################*
*--------------------------------------------------------Audio patcher----------------------------------------------------*
*#########################################################################################################################*/
#define WAV_FourCC(a, b, c, d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d)

static void Patcher_FixupWaveHeader(struct Stream* s, struct VorbisState* ctx) {
	uint8_t header[44];
	uint32_t length;
	ReturnCode res;

	res = s->Length(s, &length);
	if (res) { Launcher_ShowError(res, "getting .wav length"); return; }
	res = s->Seek(s, 0);
	if (res) { Launcher_ShowError(res, "seeking to .wav start"); return; }

	Stream_SetU32_BE(&header[0], WAV_FourCC('R','I','F','F'));
	Stream_SetU32_LE(&header[4], length - 8);
	Stream_SetU32_BE(&header[8], WAV_FourCC('W','A','V','E'));
	Stream_SetU32_BE(&header[12], WAV_FourCC('f','m','t',' '));
	Stream_SetU32_LE(&header[16], 16); /* fmt chunk size */
	Stream_SetU16_LE(&header[20], 1);  /* PCM audio format */
	Stream_SetU16_LE(&header[22], ctx->Channels);
	Stream_SetU32_LE(&header[24], ctx->SampleRate);

	Stream_SetU32_LE(&header[28], ctx->SampleRate * ctx->Channels * 2); /* byte rate */
	Stream_SetU16_LE(&header[32], ctx->Channels * 2);                   /* block align */
	Stream_SetU16_LE(&header[34], 16);                                  /* bits per sample */
	Stream_SetU32_BE(&header[36], WAV_FourCC('d','a','t','a'));
	Stream_SetU32_LE(&header[40], length - sizeof(header));

	res = Stream_Write(s, header, sizeof(header));
	if (res) Launcher_ShowError(res, "fixing .wav header");
}

static void Patcher_DecodeSound(struct Stream* s, struct VorbisState* ctx) {
	int16_t* samples;
	int count;
	ReturnCode res;

	/* ctx is all 0, so reuse it here for header */
	res = Stream_Write(s, ctx, 44);
	if (res) { Launcher_ShowError(res, "writing .wav header"); return; }

	res = Vorbis_DecodeHeaders(ctx);
	if (res) { Launcher_ShowError(res, "decoding .ogg header"); return; }
	samples = Mem_Alloc(ctx->BlockSizes[1] * ctx->Channels, 2, ".ogg samples");

	for (;;) {
		res = Vorbis_DecodeFrame(ctx);
		if (res == ERR_END_OF_STREAM) break;
		if (res) { Launcher_ShowError(res, "decoding .ogg"); break; }

		count = Vorbis_OutputFrame(ctx, samples);
		/* TODO: Do we need to account for big endian */
		res = Stream_Write(s, samples, count * 2);
		if (res) { Launcher_ShowError(res, "writing samples"); break; }
	}
	Mem_Free(samples);
}

static void Patcher_SaveSound(struct ResourceSound* sound, struct AsyncRequest* req) {
	String path; char pathBuffer[STRING_SIZE];
	uint8_t buffer[OGG_BUFFER_SIZE];
	struct Stream src, ogg, dst;
	struct VorbisState ctx = { 0 };
	ReturnCode res;

	Stream_ReadonlyMemory(&src, req->Data, req->Size);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "audio/%c.wav", sound->Name);

	res = Stream_CreateFile(&dst, &path);
	if (res) { Launcher_ShowError(res, "creating .wav file"); return; }

	Ogg_MakeStream(&ogg, buffer, &src);
	ctx.Source = &ogg;

	Patcher_DecodeSound(&dst, &ctx);
	Patcher_FixupWaveHeader(&dst, &ctx);

	res = dst.Close(&dst);
	if (res) Launcher_ShowError(res, "closing .wav file");
}

static void Patcher_SaveMusic(struct ResourceMusic* music, struct AsyncRequest* req) {
	String path; char pathBuffer[STRING_SIZE];
	ReturnCode res;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "audio/%c", music->Name);

	res = Stream_WriteAllTo(&path, req->Data, req->Size);
	if (res) Launcher_ShowError(res, "saving music file");
}


/*########################################################################################################################*
*-----------------------------------------------------------Fetcher-------------------------------------------------------*
*#########################################################################################################################*/
bool Fetcher_Working, Fetcher_Completed;
int  Fetcher_StatusCode, Fetcher_Downloaded;
ReturnCode Fetcher_Error;

CC_NOINLINE static void Fetcher_DownloadAudio(const char* name, const char* hash) {
	String url; char urlBuffer[STRING_SIZE * 2];
	String id = String_FromReadonly(name);

	String_InitArray(url, urlBuffer);
	String_Format3(&url, "http://resources.download.minecraft.net/%r%r/%c", 
					&hash[0], &hash[1], hash);
	AsyncDownloader_GetData(&url, false, &id);
}

void Fetcher_Run(void) {
	String id, url;
	int i, flags;

	if (Fetcher_Working) return;
	Fetcher_Error      = 0;
	Fetcher_StatusCode = 0;
	Fetcher_Downloaded = 0;

	Fetcher_Working    = true;
	Fetcher_Completed  = false;
	flags = Resources_GetFetchFlags();

	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (!(flags & Resources_Files[i].Flag)) continue;

		id  = String_FromReadonly(Resources_Files[i].Name);
		url = String_FromReadonly(Resources_Files[i].Url);
		AsyncDownloader_GetData(&url, false, &id);
	}

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		if (Resources_Music[i].Exists) continue;
		Fetcher_DownloadAudio(Resources_Music[i].Name,  Resources_Music[i].Hash);
	}
	for (i = 0; i < Array_Elems(Resources_Sounds); i++) {
		if (SoundsExist) continue;
		Fetcher_DownloadAudio(Resources_Sounds[i].Name, Resources_Sounds[i].Hash);
	}
}

static void Fetcher_Finish(void) {
	Fetcher_Completed = true;
	Fetcher_Working   = false;
}

CC_NOINLINE static bool Fetcher_Get(const String* id, struct AsyncRequest* req) {
	if (!AsyncDownloader_Get(id, req)) return false;

	if (req->Result) {
		Fetcher_Error = req->Result;
		Fetcher_Finish();
		return false;
	} else if (req->StatusCode != 200) {
		Fetcher_StatusCode = req->StatusCode;
		Fetcher_Finish();
		return false;
	} else if (!req->Data) {
		Fetcher_Error = ReturnCode_InvalidArg;
		Fetcher_Finish();
		return false;
	}

	Fetcher_Downloaded++;
	return true;
}

static void Fetcher_CheckMusic(struct ResourceMusic* music) {
	String id = String_FromReadonly(music->Name);
	struct AsyncRequest req;
	if (!Fetcher_Get(&id, &req)) return;

	music->Exists = true;
	Patcher_SaveMusic(music, &req);
	ASyncRequest_Free(&req);
}

static void Fetcher_CheckSound(struct ResourceSound* sound) {
	String id = String_FromReadonly(sound->Name);
	struct AsyncRequest req;
	if (!Fetcher_Get(&id, &req)) return;

	Patcher_SaveSound(sound, &req);
	ASyncRequest_Free(&req);
}

/* TODO: Implement this.. */
void Fetcher_Update(void) {
	int i;

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		if (Resources_Music[i].Exists) continue;
		Fetcher_CheckMusic(&Resources_Music[i]);
	}

	for (i = 0; i < Array_Elems(Resources_Sounds); i++) {
		Fetcher_CheckSound(&Resources_Sounds[i]);
	}

	if (Fetcher_Downloaded != Resources_Count) return; 
	Fetcher_Finish();
}