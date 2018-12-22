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
bool Textures_AllExist, Sounds_AllExist;
int Resources_Count, Resources_Size;
static int texturesFound;

CC_NOINLINE static struct ResourceTexture* Resources_FindTex(const String* name) {
	struct ResourceTexture* tex;
	int i;

	for (i = 0; i < Array_Elems(Resources_Textures); i++) {
		tex = &Resources_Textures[i];
		if (String_CaselessEqualsConst(name, tex->Filename)) return tex;
	}
	return NULL;
}

static void Resources_CheckMusic(void) {
	String path; char pathBuffer[FILENAME_SIZE];
	int i;
	String_InitArray(path, pathBuffer);

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		path.length = 0;
		String_Format1(&path, "audio/%c", Resources_Music[i].Name);

		Resources_Music[i].Downloaded = File_Exists(&path);
		if (Resources_Music[i].Downloaded) continue;

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
		Sounds_AllExist = false;

		Resources_Count += Array_Elems(Resources_Sounds);
		Resources_Size  += 417;
		return;
	}
	Sounds_AllExist = true;
}

static bool Resources_SelectZipEntry(const String* path) {
	String name = *path;
	Utils_UNSAFE_GetFilename(&name);

	if (Resources_FindTex(&name)) texturesFound++;
	return false;
}

static void Resources_CheckTextures(void) {
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
	if (res) Launcher_ShowError(res, "inspecting default.zip");

	/* if somehow have say "gui.png", "GUI.png" */
	Textures_AllExist = texturesFound >= Array_Elems(Resources_Textures);
}

void Resources_CheckExistence(void) {
	int i;
	Resources_CheckTextures();
	Resources_CheckMusic();
	Resources_CheckSounds();

	if (Textures_AllExist) return;
	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		Resources_Count++;
		Resources_Size += Resources_Files[i].Size;
	}
}

struct ResourceFile Resources_Files[4] = {
	{ "classic jar", "http://launcher.mojang.com/mc/game/c0.30_01c/client/54622801f5ef1bcc1549a842c5b04cb5d5583005/client.jar",  291 },
	{ "1.6.2 jar",   "http://launcher.mojang.com/mc/game/1.6.2/client/b6cb68afde1d9cf4a20cbf27fa90d0828bf440a4/client.jar",     4621 },
	{ "gui.png patch",     "http://static.classicube.net/terrain-patch2.png",  7 },
	{ "terrain.png patch", "http://static.classicube.net/gui.png",            21 }
};

struct ResourceTexture Resources_Textures[20] = {
	/* classic jar files */
	{ "char.png"     }, { "clouds.png"      }, { "default.png" }, { "particles.png" },
	{ "rain.png"     }, { "gui_classic.png" }, { "icons.png"   }, { "terrain.png"   },
	{ "creeper.png"  }, { "pig.png"         }, { "sheep.png"   }, { "sheep_fur.png" },
	{ "skeleton.png" }, { "spider.png"      }, { "zombie.png"  }, /* "arrows.png", "sign.png" */
	/* other files */
	{ "snow.png"     }, { "chicken.png"     }, { "gui.png"     },
	{ "animations.png" }, { "animations.txt" }
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
*---------------------------------------------------------Zip writer------------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode ZipPatcher_LocalFile(struct Stream* s, struct ResourceTexture* e) {
	String name = String_FromReadonly(e->Filename);
	uint8_t header[30 + STRING_SIZE];
	ReturnCode res;
	if ((res = s->Position(s, &e->Offset))) return res;

	Stream_SetU32_LE(&header[0], 0x04034b50);   /* signature */
	Stream_SetU16_LE(&header[4],  20);          /* version needed */
	Stream_SetU16_LE(&header[6],  0);           /* bitflags */
	Stream_SetU16_LE(&header[8] , 0);           /* compression method */
	Stream_SetU16_LE(&header[10], 0);           /* last modified */
	Stream_SetU16_LE(&header[12], 0);           /* last modified */
	
	Stream_SetU32_LE(&header[14], e->Crc32);    /* CRC32 */
	Stream_SetU32_LE(&header[18], e->Size);     /* Compressed size */
	Stream_SetU32_LE(&header[22], e->Size);     /* Uncompressed size */
	 
	Stream_SetU16_LE(&header[26], name.length); /* name length */
	Stream_SetU16_LE(&header[28], 0);           /* extra field length */

	Mem_Copy(&header[30], name.buffer, name.length);
	return Stream_Write(s, header, 30 + name.length);
}

static ReturnCode ZipPatcher_CentralDir(struct Stream* s, struct ResourceTexture* e) {
	String name = String_FromReadonly(e->Filename);
	uint8_t header[46 + STRING_SIZE];
	struct DateTime now;
	int modTime, modDate;

	DateTime_CurrentLocal(&now);
	modTime = (now.Second / 2) | (now.Minute << 5) | (now.Hour << 11);
	modDate = (now.Day) | (now.Month << 5) | ((now.Year - 1980) << 9);

	Stream_SetU32_LE(&header[0],  0x02014b50);  /* signature */
	Stream_SetU16_LE(&header[4],  20);          /* version */
	Stream_SetU16_LE(&header[6],  20);          /* version needed */
	Stream_SetU16_LE(&header[8],  0);           /* bitflags */
	Stream_SetU16_LE(&header[10], 0);           /* compression method */
	Stream_SetU16_LE(&header[12], modTime);     /* last modified */
	Stream_SetU16_LE(&header[14], modDate);     /* last modified */

	Stream_SetU32_LE(&header[16], e->Crc32);    /* CRC32 */
	Stream_SetU32_LE(&header[20], e->Size);     /* compressed size */
	Stream_SetU32_LE(&header[24], e->Size);     /* uncompressed size */

	Stream_SetU16_LE(&header[28], name.length); /* name length */
	Stream_SetU16_LE(&header[30], 0);           /* extra field length */
	Stream_SetU16_LE(&header[32], 0);           /* file comment length */
	Stream_SetU16_LE(&header[34], 0);           /* disk number */
	Stream_SetU16_LE(&header[36], 0);           /* internal attributes */
	Stream_SetU32_LE(&header[38], 0);           /* external attributes */
	Stream_SetU32_LE(&header[42], e->Offset);   /* local header offset */

	Mem_Copy(&header[46], name.buffer, name.length);
	return Stream_Write(s, header, 46 + name.length);
}

static ReturnCode ZipPatcher_EndOfCentralDir(struct Stream* s, uint32_t centralDirBeg, uint32_t centralDirEnd) {
	uint8_t header[22];

	Stream_SetU32_LE(&header[0], 0x06054b50); /* signature */
	Stream_SetU16_LE(&header[4], 0);          /* disk number */
	Stream_SetU16_LE(&header[6], 0);          /* disk number of start */
	Stream_SetU16_LE(&header[8],  Array_Elems(Resources_Textures)); /* disk entries */
	Stream_SetU16_LE(&header[10], Array_Elems(Resources_Textures)); /* total entries */
	Stream_SetU32_LE(&header[12], centralDirEnd - centralDirBeg);   /* central dir size */
	Stream_SetU32_LE(&header[16], centralDirBeg);                   /* central dir start */
	Stream_SetU16_LE(&header[20], 0);         /* comment length */
	return Stream_Write(s, header, 22);
}

static ReturnCode ZipPatcher_FixupLocalFile(struct Stream* s, struct ResourceTexture* e) {
	String name = String_FromReadonly(e->Filename);
	uint8_t tmp[2048];
	uint32_t dataBeg, dataEnd;
	uint32_t i, crc, toRead, read;
	ReturnCode res;

	dataBeg = e->Offset + 30 + name.length;
	if ((res = s->Position(s, &dataEnd))) return res;
	e->Size = dataEnd - dataBeg;

	/* work out the CRC 32 */
	crc = 0xffffffffUL;
	if ((res = s->Seek(s, dataBeg))) return res;

	for (; dataBeg < dataEnd; dataBeg += read) {
		toRead = dataEnd - dataBeg;
		toRead = min(toRead, sizeof(tmp));

		if ((res = s->Read(s, tmp, toRead, &read))) return res;
		if (!read) return ERR_END_OF_STREAM;

		for (i = 0; i < read; i++) {
			crc = Utils_Crc32Table[(crc ^ tmp[i]) & 0xFF] ^ (crc >> 8);
		}
	}
	e->Crc32 = crc ^ 0xffffffffUL;

	/* then fixup the header */
	if ((res = s->Seek(s, e->Offset)))      return res;
	if ((res = ZipPatcher_LocalFile(s, e))) return res;
	return s->Seek(s, dataEnd);
}

static ReturnCode ZipPatcher_WriteData(struct Stream* s, struct ResourceTexture* tex, const uint8_t* data, uint32_t len) {
	ReturnCode res;
	tex->Size  = len;
	tex->Crc32 = Utils_CRC32(data, len);

	res = ZipPatcher_LocalFile(s, tex);
	if (res) return res;
	return Stream_Write(s, data, len);
}

/*static ReturnCode ZipPatcher_WriteStream(struct Stream* s, struct ResourceTexture* tex, struct Stream* src) {
	uint8_t tmp[2048];
	uint32_t read;
	ReturnCode res;
	if ((res = ZipPatcher_LocalFile(s, tex))) return res;

	for (;;) {
		res = src->Read(src, tmp, sizeof(tmp), &read);
		if (res)   return res;
		if (!read) break;

		if ((res = Stream_Write(s, tmp, read))) return res;
	}
	return ZipPatcher_FixupLocalFile(s, tex);
}*/
static ReturnCode ZipPatcher_WriteStream(struct Stream* s, struct ResourceTexture* tex, struct Stream* src) {
	uint8_t tmp[2048];
	uint32_t read;
	struct Stream crc32;
	ReturnCode res;

	res = ZipPatcher_LocalFile(s, tex);
	if (res) return res;
	Stream_WriteonlyCrc32(&crc32, s);

	for (tex->Size = 0; ; tex->Size += read) {
		res = src->Read(src, tmp, sizeof(tmp), &read);
		if (res)   return res;
		if (!read) break;

		res = Stream_Write(&crc32, tmp, read);
		if (res) return res;
	}

	tex->Crc32 = crc32.Meta.CRC32.CRC32 ^ 0xFFFFFFFFUL;
	return ZipPatcher_FixupLocalFile(s, tex);
}

static ReturnCode ZipPatcher_WritePng(struct Stream* s, struct ResourceTexture* tex, Bitmap* src) {
	ReturnCode res;

	if ((res = ZipPatcher_LocalFile(s, tex)))   return res;
	if ((res = Png_Encode(src, s, NULL, true))) return res;
	return ZipPatcher_FixupLocalFile(s, tex);
}


/*########################################################################################################################*
*-------------------------------------------------------Texture patcher---------------------------------------------------*
*#########################################################################################################################*/
#define ANIMS_TXT_CONTENTS \
"# This file defines the animations used in a texture pack for ClassicalSharp and other supporting applications.\r\n" \
"# Each line is in the format : <TileX> <TileY> <FrameX> <FrameY> <Frame size> <Frames count> <Tick delay>\r\n" \
"# - TileX and TileY are the coordinates of the tile in terrain.png that will be replaced by the animation frames.\r\n" \
"#     Essentially, TileX and TileY are the remainder and quotient of an ID in F10 menu divided by 16\r\n" \
"#     For instance, obsidian texture(37) has TileX of 5, and TileY of 2\r\n" \
"# - FrameX and FrameY are the pixel coordinates of the first animation frame in animations.png.\r\n" \
"# - Frame Size is the size in pixels of an animation frame.\r\n" \
"# - Frames count is the number of used frames.  The first frame is located at\r\n" \
"#     (FrameX, FrameY), the second one at (FrameX + FrameSize, FrameY) and so on.\r\n" \
"# - Tick delay is the number of ticks a frame doesn't change. For instance, delay of 0\r\n" \
"#     means that the tile would be replaced every tick, while delay of 2 means\r\n" \
"#     'replace with frame 1, don't change frame, don't change frame, replace with frame 2'.\r\n" \
"# NOTE: If a file called 'uselavaanim' is in the texture pack, ClassicalSharp 0.99.2 onwards uses its built - in dynamic generation for the lava texture animation.\r\n" \
"# NOTE : If a file called 'usewateranim' is in the texture pack, ClassicalSharp 0.99.5 onwards uses its built - in dynamic generation for the water texture animation.\r\n" \
"\r\n" \
"# fire\r\n" \
"6 2 0 0 16 32 0"
static Bitmap terrainBmp;

static bool ClassicPatcher_SelectEntry(const String* path ) {
	String name = *path;
	Utils_UNSAFE_GetFilename(&name);
	return Resources_FindTex(&name) != NULL;
}

static ReturnCode ClassicPatcher_ProcessEntry(const String* path, struct Stream* data, void* obj) {
	static const String guiClassicPng = String_FromConst("gui_classic.png");
	struct Stream* s = obj;
	struct ResourceTexture* entry;
	String name;

	/* terrain.png requires special patching */
	if (String_CaselessEqualsConst(path, "terrain.png")) {
		return Png_Decode(&terrainBmp, data);
	}

	name = *path;
	Utils_UNSAFE_GetFilename(&name);
	if (String_CaselessEqualsConst(&name, "gui.png")) name = guiClassicPng;

	entry = Resources_FindTex(&name);
	return ZipPatcher_WriteStream(s, entry, data);
}

static ReturnCode ClassicPatcher_ExtractFiles(struct Stream* s) {
	struct ZipState zip;
	struct Stream src;

	Stream_ReadonlyMemory(&src, Resources_Files[0].Data, Resources_Files[0].Len);
	Zip_Init(&zip, &src);

	zip.Obj = s;
	zip.SelectEntry  = ClassicPatcher_SelectEntry;
	zip.ProcessEntry = ClassicPatcher_ProcessEntry;
	return Zip_Extract(&zip);
}

/* the x,y of tiles in terrain.png which get patched */
static struct TilePatch { const char* Name; uint8_t X1,Y1, X2,Y2; } modern_tiles[12] = {
	{ "assets/minecraft/textures/blocks/sandstone_bottom.png", 9,3 },
	{ "assets/minecraft/textures/blocks/sandstone_normal.png", 9,2 },
	{ "assets/minecraft/textures/blocks/sandstone_top.png", 9,1, },
	{ "assets/minecraft/textures/blocks/quartz_block_lines_top.png", 10,3, 10,1 },
	{ "assets/minecraft/textures/blocks/quartz_block_lines.png", 10,2 },
	{ "assets/minecraft/textures/blocks/stonebrick.png", 4,3 },
	{ "assets/minecraft/textures/blocks/snow.png", 2,3 },
	{ "assets/minecraft/textures/blocks/wool_colored_blue.png", 3,5 },
	{"assets/minecraft/textures/blocks/wool_colored_brown.png", 2,5 },
	{"assets/minecraft/textures/blocks/wool_colored_cyan.png", 4,5 },
	{"assets/minecraft/textures/blocks/wool_colored_green.png", 1,5 },
	{"assets/minecraft/textures/blocks/wool_colored_pink.png", 0,5 }
};

CC_NOINLINE static struct TilePatch* ModernPatcher_GetTile(const String* path) {
	int i;
	for (i = 0; i < Array_Elems(modern_tiles); i++) {
		if (String_CaselessEqualsConst(path, modern_tiles[i].Name)) return &modern_tiles[i];
	}
	return NULL;
}

static ReturnCode ModernPatcher_PatchTile(struct Stream* data, struct TilePatch* tile) {
	Bitmap bmp;
	ReturnCode res;

	if ((res = Png_Decode(&bmp, data))) return res;
	Bitmap_CopyBlock(0, 0, tile->X1 * 16, tile->Y1 * 16, &bmp, &terrainBmp, 16);

	/* only quartz needs copying to two tiles */
	if (tile->Y2) {
		Bitmap_CopyBlock(0, 0, tile->X2 * 16, tile->Y2 * 16, &bmp, &terrainBmp, 16);
	}

	Mem_Free(bmp.Scan0);
	return 0;
}


static bool ModernPatcher_SelectEntry(const String* path) {
	return
		String_CaselessEqualsConst(path, "assets/minecraft/textures/environment/snow.png") ||
		String_CaselessEqualsConst(path, "assets/minecraft/textures/entity/chicken.png")   ||
		String_CaselessEqualsConst(path, "assets/minecraft/textures/blocks/fire_layer_1.png") ||
		ModernPatcher_GetTile(path) != NULL;
}

static ReturnCode ModernPatcher_MakeAnimations(struct Stream* s, struct Stream* data) {
	static const String animsPng = String_FromConst("animations.png");
	struct ResourceTexture* entry;
	uint8_t anim_data[Bitmap_DataSize(512, 16)];
	Bitmap anim, bmp;
	ReturnCode res;
	int i;

	if ((res = Png_Decode(&bmp, data))) return res;
	Bitmap_Create(&anim, 512, 16, anim_data);

	for (i = 0; i < 512; i += 16) {
		Bitmap_CopyBlock(0, i, i, 0, &bmp, &anim, 16);
	}

	Mem_Free(bmp.Scan0);
	entry = Resources_FindTex(&animsPng);
	return ZipPatcher_WritePng(s, entry, &anim);
}

static ReturnCode ModernPatcher_ProcessEntry(const String* path, struct Stream* data, void* obj) {
	struct Stream* s = obj;
	struct ResourceTexture* entry;
	struct TilePatch* tile;
	String name;

	if (String_CaselessEqualsConst(path, "assets/minecraft/textures/environment/snow.png")
		|| String_CaselessEqualsConst(path, "assets/minecraft/textures/entity/chicken.png")) {
		name = *path;
		Utils_UNSAFE_GetFilename(&name);

		entry = Resources_FindTex(&name);
		return ZipPatcher_WriteStream(s, entry, data);
	}

	if (String_CaselessEqualsConst(path, "assets/minecraft/textures/blocks/fire_layer_1.png")) {
		return ModernPatcher_MakeAnimations(s, data);
	}

	tile = ModernPatcher_GetTile(path);
	return ModernPatcher_PatchTile(data, tile);
}

static ReturnCode ModernPatcher_ExtractFiles(struct Stream* s) {
	struct ZipState zip;
	struct Stream src;

	Stream_ReadonlyMemory(&src, Resources_Files[1].Data, Resources_Files[1].Len);
	Zip_Init(&zip, &src);

	zip.Obj = s;
	zip.SelectEntry  = ModernPatcher_SelectEntry;
	zip.ProcessEntry = ModernPatcher_ProcessEntry;
	return Zip_Extract(&zip);
}

static ReturnCode TexPatcher_NewFiles(struct Stream* s) {
	static const String guiPng   = String_FromConst("gui.png");
	static const String animsTxt = String_FromConst("animations.txt");
	struct ResourceTexture* entry;
	ReturnCode res;

	/* make our own animations.txt */
	entry = Resources_FindTex(&animsTxt);
	res   = ZipPatcher_WriteData(s, entry, ANIMS_TXT_CONTENTS, sizeof(ANIMS_TXT_CONTENTS) - 1);
	if (res) return res;

	/* make ClassiCube gui.png */
	entry = Resources_FindTex(&guiPng);
	res   = ZipPatcher_WriteData(s, entry, Resources_Files[3].Data, Resources_Files[3].Len);

	return res;
}

static void TexPatcher_PatchTile(Bitmap* src, int srcX, int srcY, int dstX, int dstY) {
	Bitmap_CopyBlock(srcX, srcY, dstX * 16, dstY * 16, src, &terrainBmp, 16);
}

static ReturnCode TexPatcher_Terrain(struct Stream* s) {
	static const String terrainPng = String_FromConst("terrain.png");
	struct ResourceTexture* entry;
	Bitmap bmp;
	struct Stream src;
	ReturnCode res;

	Stream_ReadonlyMemory(&src, Resources_Files[2].Data, Resources_Files[2].Len);
	if ((res = Png_Decode(&bmp, &src))) return res;

	TexPatcher_PatchTile(&bmp,  0,0, 3,3);
	TexPatcher_PatchTile(&bmp, 16,0, 6,3);
	TexPatcher_PatchTile(&bmp, 32,0, 6,2);

	TexPatcher_PatchTile(&bmp,  0,16,  5,3);
	TexPatcher_PatchTile(&bmp, 16,16,  6,5);
	TexPatcher_PatchTile(&bmp, 32,16, 11,0);

	entry = Resources_FindTex(&terrainPng);
	res   = ZipPatcher_WritePng(s, entry, &terrainBmp);
	Mem_Free(bmp.Scan0);
	return res;
}

static ReturnCode TexPatcher_WriteEntries(struct Stream* s) {
	uint32_t beg, end;
	int i;
	ReturnCode res;

	if ((res = ClassicPatcher_ExtractFiles(s))) return res;
	if ((res = ModernPatcher_ExtractFiles(s)))  return res;
	if ((res = TexPatcher_NewFiles(s)))         return res;
	if ((res = TexPatcher_Terrain(s)))          return res;
	
	if ((res = s->Position(s, &beg))) return res;
	for (i = 0; i < Array_Elems(Resources_Textures); i++) {
		if ((res = ZipPatcher_CentralDir(s, &Resources_Textures[i]))) return res;
	}

	if ((res = s->Position(s, &end))) return res;
	return ZipPatcher_EndOfCentralDir(s, beg, end);
}

static void TexPatcher_MakeDefaultZip(void) {
	const static String path = String_FromConst("texpacks/default.zip");
	struct Stream s;
	int i;
	ReturnCode res;

	res = Stream_CreateFile(&s, &path);
	if (res) {
		Launcher_ShowError(res, "creating default.zip");
	} else {
		res = TexPatcher_WriteEntries(&s);
		if (res) Launcher_ShowError(res, "making default.zip");

		res = s.Close(&s);
		if (res) Launcher_ShowError(res, "closing default.zip");
	}

	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		Mem_Free(Resources_Files[i].Data);
		Resources_Files[i].Data = NULL;
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Audio patcher----------------------------------------------------*
*#########################################################################################################################*/
#define WAV_FourCC(a, b, c, d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d)

static void SoundPatcher_FixupHeader(struct Stream* s, struct VorbisState* ctx) {
	uint8_t header[44];
	uint32_t length;
	ReturnCode res;

	res = s->Length(s, &length);
	if (res) { Launcher_ShowError(res, "getting .wav length"); return; }
	res = s->Seek(s, 0);
	if (res) { Launcher_ShowError(res, "seeking to .wav start"); return; }

	Stream_SetU32_BE(&header[0],  WAV_FourCC('R','I','F','F'));
	Stream_SetU32_LE(&header[4],  length - 8);
	Stream_SetU32_BE(&header[8],  WAV_FourCC('W','A','V','E'));
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

static void SoundPatcher_DecodeAudio(struct Stream* s, struct VorbisState* ctx) {
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

static void SoundPatcher_Save(struct ResourceSound* sound, struct AsyncRequest* req) {
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

	SoundPatcher_DecodeAudio(&dst, &ctx);
	SoundPatcher_FixupHeader(&dst, &ctx);

	res = dst.Close(&dst);
	if (res) Launcher_ShowError(res, "closing .wav file");
}

static void MusicPatcher_Save(struct ResourceMusic* music, struct AsyncRequest* req) {
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
	int i;

	if (Fetcher_Working) return;
	Fetcher_Error      = 0;
	Fetcher_StatusCode = 0;
	Fetcher_Downloaded = 0;

	Fetcher_Working    = true;
	Fetcher_Completed  = false;

	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (Textures_AllExist) continue;

		id  = String_FromReadonly(Resources_Files[i].Name);
		url = String_FromReadonly(Resources_Files[i].Url);
		AsyncDownloader_GetData(&url, false, &id);
	}

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		if (Resources_Music[i].Downloaded) continue;
		Fetcher_DownloadAudio(Resources_Music[i].Name,  Resources_Music[i].Hash);
	}
	for (i = 0; i < Array_Elems(Resources_Sounds); i++) {
		if (Sounds_AllExist) continue;
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

static void Fetcher_CheckFile(struct ResourceFile* file) {
	String id = String_FromReadonly(file->Name);
	struct AsyncRequest req;
	if (!Fetcher_Get(&id, &req)) return;
	
	file->Downloaded = true;
	file->Data       = req.Data;
	file->Len        = req.Size;
	/* don't free request */
}

static void Fetcher_CheckMusic(struct ResourceMusic* music) {
	String id = String_FromReadonly(music->Name);
	struct AsyncRequest req;
	if (!Fetcher_Get(&id, &req)) return;

	music->Downloaded = true;
	MusicPatcher_Save(music, &req);
	ASyncRequest_Free(&req);
}

static void Fetcher_CheckSound(struct ResourceSound* sound) {
	String id = String_FromReadonly(sound->Name);
	struct AsyncRequest req;
	if (!Fetcher_Get(&id, &req)) return;

	SoundPatcher_Save(sound, &req);
	ASyncRequest_Free(&req);
}

/* TODO: Implement this.. */
/* TODO: How expensive is it to constantly do 'Get' over and make all these strings */
void Fetcher_Update(void) {
	int i;

	for (i = 0; i < Array_Elems(Resources_Files); i++) {
		if (Resources_Files[i].Downloaded) continue;
		Fetcher_CheckFile(&Resources_Files[i]);
	}
	if (Resources_Files[3].Data) TexPatcher_MakeDefaultZip();

	for (i = 0; i < Array_Elems(Resources_Music); i++) {
		if (Resources_Music[i].Downloaded) continue;
		Fetcher_CheckMusic(&Resources_Music[i]);
	}

	for (i = 0; i < Array_Elems(Resources_Sounds); i++) {
		Fetcher_CheckSound(&Resources_Sounds[i]);
	}

	if (Fetcher_Downloaded != Resources_Count) return; 
	Fetcher_Finish();
}
