#include "Resources.h"
#include "Funcs.h"
#include "String.h"
#include "Constants.h"
#include "Deflate.h"
#include "Stream.h"
#include "Platform.h"
#include "Launcher.h"
#include "Utils.h"


/*########################################################################################################################*
*---------------------------------------------------------List/Checker----------------------------------------------------*
*#########################################################################################################################*/
bool DigSoundsExist, StepSoundsExist;
int Resources_Size, Resources_Count;

static void Resources_CheckFiles(void) {
	int flags = Resources_GetFetchFlags();
	if (flags & FLAG_CLASSIC) { Resources_Size += 291;  Resources_Count++; }
	if (flags & FLAG_MODERN)  { Resources_Size += 4621; Resources_Count++; }
	if (flags & FLAG_TERRAIN) { Resources_Size += 7;    Resources_Count++; }
	if (flags & FLAG_GUI)     { Resources_Size += 21;   Resources_Count++; }
}

static void Resources_CheckMusic(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i = 0;

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

CC_NOINLINE static bool Resources_CheckExist(const char* prefix, struct ResourceSound* sounds, int count) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i = 0;

	String_InitArray(path, pathBuffer);
	for (i = 0; i < count; i++) {
		path.length = 0;
		String_Format2(&path, "audio/%c_%c.wav", prefix, sounds[i].Name);

		if (!File_Exists(&path)) return false;
	}
	return true;
}

static void Resources_CheckSounds(void) {
	DigSoundsExist = Resources_CheckExist("dig", Resources_Dig, Array_Elems(Resources_Dig));
	if (!DigSoundsExist) {
		Resources_Count += Array_Elems(Resources_Dig);
		Resources_Size  += 173;
	}

	StepSoundsExist = Resources_CheckExist("step", Resources_Step, Array_Elems(Resources_Step));
	if (!StepSoundsExist) {
		Resources_Count += Array_Elems(Resources_Step);
		Resources_Size  += 244;
	}
}

static bool Resources_SelectZipEntry(const String* path) {
	String name;
	int i;

	name = *path;
	Utils_UNSAFE_GetFilename(&name);
	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (Resources_Files[i].Exists) continue;

		if (!String_CaselessEqualsConst(&name, Resources_Files[i].Filename)) continue;
		Resources_Files[i].Exists = true;
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
	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (Resources_Files[i].Exists) continue;

		flags |= Resources_Files[i].Flags;
	}
	return flags;
}

void Resources_CheckExistence(void) {
	Resources_CheckDefaultZip();
	Resources_CheckFiles();
	Resources_CheckMusic();
	Resources_CheckSounds();
}

struct ResourceFile Resources_Files[19] = {
	/* classic jar files */
	{ "char.png",       FLAG_CLASSIC }, { "clouds.png",      FLAG_CLASSIC },
	{ "default.png",    FLAG_CLASSIC }, { "particles.png",   FLAG_CLASSIC },
	{ "rain.png",       FLAG_CLASSIC }, { "gui_classic.png", FLAG_CLASSIC },
	{ "icons.png",      FLAG_CLASSIC },
	{ "terrain.png",    FLAG_CLASSIC | FLAG_TERRAIN | FLAG_MODERN },
	{ "creeper.png",    FLAG_CLASSIC }, { "pig.png",         FLAG_CLASSIC },
	{ "sheep.png",      FLAG_CLASSIC }, { "sheep_fur.png",   FLAG_CLASSIC },
	{ "skeleton.png",   FLAG_CLASSIC }, { "spider.png",      FLAG_CLASSIC },
	{ "zombie.png",     FLAG_CLASSIC }, /* "arrows.png", "sign.png" */
	/* other files */
	{ "snow.png",       FLAG_MODERN },  { "chicken.png",     FLAG_MODERN },
	{ "animations.png", FLAG_MODERN },  { "gui.png",         FLAG_GUI }
};

struct ResourceSound Resources_Dig[31] = {
	{ "cloth1",  "5f/5fd568d724ba7d53911b6cccf5636f859d2662e8" }, { "cloth2",  "56/56c1d0ac0de2265018b2c41cb571cc6631101484" },
	{ "cloth3",  "9c/9c63f2a3681832dc32d206f6830360bfe94b5bfc" }, { "cloth4",  "55/55da1856e77cfd31a7e8c3d358e1f856c5583198" },
	{ "grass1",  "41/41cbf5dd08e951ad65883854e74d2e034929f572" }, { "grass2",  "86/86cb1bb0c45625b18e00a64098cd425a38f6d3f2" },
	{ "grass3",  "f7/f7d7e5c7089c9b45fa5d1b31542eb455fad995db" }, { "grass4",  "c7/c7b1005d4926f6a2e2387a41ab1fb48a72f18e98" },
	{ "gravel1", "e8/e8b89f316f3e9989a87f6e6ff12db9abe0f8b09f" }, { "gravel2", "c3/c3b3797d04cb9640e1d3a72d5e96edb410388fa3" },
	{ "gravel3", "48/48f7e1bb098abd36b9760cca27b9d4391a23de26" }, { "gravel4", "7b/7bf3553a4fe41a0078f4988a13d6e1ed8663ef4c" },
	{ "sand1",   "9e/9e59c3650c6c3fc0a475f1b753b2fcfef430bf81" }, { "sand2",   "0f/0fa4234797f336ada4e3735e013e44d1099afe57" },
	{ "sand3",   "c7/c75589cc0087069f387de127dd1499580498738e" }, { "sand4",   "37/37afa06f97d58767a1cd1382386db878be1532dd" },
	{ "snow1",   "e9/e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c" }, { "snow2",   "58/5887d10234c4f244ec5468080412f3e6ef9522f3" },
	{ "snow3",   "a4/a4bc069321a96236fde04a3820664cc23b2ea619" }, { "snow4",   "e2/e26fa3036cdab4c2264ceb19e1cd197a2a510227" },
	{ "stone1",  "4e/4e094ed8dfa98656d8fec52a7d20c5ee6098b6ad" }, { "stone2",  "9c/9c92f697142ae320584bf64c0d54381d59703528" },
	{ "stone3",  "8f/8f23c02475d388b23e5faa680eafe6b991d7a9d4" }, { "stone4",  "36/363545a76277e5e47538b2dd3a0d6aa4f7a87d34" },
	{ "wood1",   "9b/9bc2a84d0aa98113fc52609976fae8fc88ea6333" }, { "wood2",   "98/98102533e6085617a2962157b4f3658f59aea018" },
	{ "wood3",   "45/45b2aef7b5049e81b39b58f8d631563fadcc778b" }, { "wood4",   "dc/dc66978374a46ab2b87db6472804185824868095" },
	{ "glass1",  "72/7274a2231ed4544a37e599b7b014e589e5377094" }, { "glass2",  "87/87c47bda3645c68f18a49e83cbf06e5302d087ff" },
	{ "glass3",  "ad/ad7d770b7fff3b64121f75bd60cecfc4866d1cd6" }
};

struct ResourceSound Resources_Step[28] = {
	{ "cloth1",  "5f/5fd568d724ba7d53911b6cccf5636f859d2662e8" }, { "cloth2",  "56/56c1d0ac0de2265018b2c41cb571cc6631101484" },
	{ "cloth3",  "9c/9c63f2a3681832dc32d206f6830360bfe94b5bfc" }, { "cloth4",  "55/55da1856e77cfd31a7e8c3d358e1f856c5583198" },
	{ "grass1",  "22/227ab99bf7c6cf0b2002e0f7957d0ff7e5cb0c96" }, { "grass2",  "5c/5c971029d9284676dce1dda2c9d202f8c47163b2" },
	{ "grass3",  "76/76de0a736928eac5003691d73bdc2eda92116198" }, { "grass4",  "bc/bc28801224a0aa77fdc93bb7c6c94beacdf77331" },
	{ "gravel1", "1d/1d761cb3bcb45498719e4fba0751e1630e134f1a" }, { "gravel2", "ac/ac7a7c8d106e26abc775b1b46150c083825d8ddc" },
	{ "gravel3", "c1/c109b985a7e6d5d3828c92e00aefa49deca0eb8c" }, { "gravel4", "a4/a47adece748059294c5f563c0fcac02fa0e4bab4" },
	{ "sand1",   "98/9813c8185197f4a4296649f27a9d738c4a6dfc78" }, { "sand2",   "bd/bd1750c016f6bab40934eff0b0fb64c41c86e44b" },
	{ "sand3",   "ab/ab07279288fa49215bada5c17627e6a54ad0437c" }, { "sand4",   "a4/a474236fb0c75bd65a6010e87dbc000d345fc185" },
	{ "snow1",   "e9/e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c" }, { "snow2",   "58/5887d10234c4f244ec5468080412f3e6ef9522f3" },
	{ "snow3",   "a4/a4bc069321a96236fde04a3820664cc23b2ea619" }, { "snow4",   "e2/e26fa3036cdab4c2264ceb19e1cd197a2a510227" },
	{ "stone1",  "4a/4a2e3795ffd4d3aab0834b7e41903af3a8f7d197" }, { "stone2",  "22/22a383d9c22342305a4f16fec0bb479a885f8da2" },
	{ "stone3",  "a5/a533e7ae975e62592de868e0d0572778614bd587" }, { "stone4",  "d5/d5218034051a13322d7b5efc0b5a9af739615f04" },
	{ "wood1",   "af/afb01196e2179e3b15b48f3373cea4c155d56b84" }, { "wood2",   "1e/1e82a43c30cf8fcbe05d0bc2760ecba5d2320314" },
	{ "wood3",   "27/27722125968ac60c0f191a961b17e406f1351c6e" }, { "wood4",   "29/29586f60bfe6f521dbc748919d4f0dc5b28beefd" }
};

struct ResourceMusic Resources_Music[7] = {
	{ "calm1.ogg", "50/50a59a4f56e4046701b758ddbb1c1587efa4cadf", 2472 },
	{ "calm2.ogg", "74/74da65c99aa578486efa7b69983d3533e14c0d6e", 1931 },
	{ "calm3.ogg", "14/14ae57a6bce3d4254daa8be2b098c2d99743cc3f", 2181 },
	{ "hal1.ogg",  "df/df1ff11b79757432c5c3f279e5ecde7b63ceda64", 1926 },
	{ "hal2.ogg",  "ce/ceaaaa1d57dfdfbb0bd4da5ea39628b42897a687", 1714 },
	{ "hal3.ogg",  "dd/dd85fb564e96ee2dbd4754f711ae9deb08a169f9", 1879 },
	{ "hal4.ogg",  "5e/5e7d63e75c6e042f452bc5e151276911ef92fed8", 2499 }
};


/*########################################################################################################################*
*-----------------------------------------------------------Fetcher-------------------------------------------------------*
*#########################################################################################################################*/
struct FetchResourcesData FetchResourcesTask;
static void Fetcher_DownloadAll(void) {
	String id; char idBuffer[STRING_SIZE];
	String url; char urlBuffer[STRING_SIZE];
	int i;

	String_InitArray(id,  idBuffer);
	String_InitArray(url, urlBuffer);
}

static void Fetcher_Next(void) {

}

/* TODO: Implement this.. */
void FetchResourcesTask_Run(FetchResourcesStatus setStatus) { 
}
