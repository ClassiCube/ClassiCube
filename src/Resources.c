#include "Resources.h"
#ifndef CC_BUILD_WEB
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
#include "Logger.h"
#include "LWeb.h"

/*########################################################################################################################*
*--------------------------------------------------------Resources list---------------------------------------------------*
*#########################################################################################################################*/
static struct FileResource {
	const char* name;
	const char* url;
	short size;
	cc_bool downloaded;
	int reqID;
	/* downloaded archive */
	cc_uint8* data; cc_uint32 len;
} fileResources[4] = {
	{ "classic jar", "http://launcher.mojang.com/mc/game/c0.30_01c/client/54622801f5ef1bcc1549a842c5b04cb5d5583005/client.jar",  291 },
	{ "1.6.2 jar",   "http://launcher.mojang.com/mc/game/1.6.2/client/b6cb68afde1d9cf4a20cbf27fa90d0828bf440a4/client.jar",     4621 },
	{ "terrain.png patch", RESOURCE_SERVER "/terrain-patch2.png",  7 },
#ifdef CC_BUILD_ANDROID
	{ "new textures",      RESOURCE_SERVER "/default.zip",        87 }
#else
	{ "gui.png patch",     RESOURCE_SERVER "/gui.png",            21 }
#endif
};

static struct ResourceTexture {
	const char* filename;
	/* zip data */
	cc_uint32 size, offset, crc32;
} textureResources[] = {
	/* classic jar files */
	{ "char.png"     }, { "clouds.png"      }, { "default.png" }, { "particles.png" },
	{ "rain.png"     }, { "gui_classic.png" }, { "icons.png"   }, { "terrain.png"   },
	{ "creeper.png"  }, { "pig.png"         }, { "sheep.png"   }, { "sheep_fur.png" },
	{ "skeleton.png" }, { "spider.png"      }, { "zombie.png"  }, /* "arrows.png", "sign.png" */
	/* other files */
	{ "snow.png"     }, { "chicken.png"     }, { "gui.png"     },
	{ "animations.png" }, { "animations.txt" },
#ifdef CC_BUILD_ANDROID
	{ "touch.png" }
#endif
};

static struct SoundResource {
	const char* name;
	const char* hash;
	int reqID;
} soundResources[59] = {
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
	{ "step_grass1",  "41cbf5dd08e951ad65883854e74d2e034929f572" }, { "step_grass2",  "86cb1bb0c45625b18e00a64098cd425a38f6d3f2" },
	{ "step_grass3",  "f7d7e5c7089c9b45fa5d1b31542eb455fad995db" }, { "step_grass4",  "c7b1005d4926f6a2e2387a41ab1fb48a72f18e98" },
	{ "step_gravel1", "e8b89f316f3e9989a87f6e6ff12db9abe0f8b09f" }, { "step_gravel2", "c3b3797d04cb9640e1d3a72d5e96edb410388fa3" },
	{ "step_gravel3", "48f7e1bb098abd36b9760cca27b9d4391a23de26" }, { "step_gravel4", "7bf3553a4fe41a0078f4988a13d6e1ed8663ef4c" },
	{ "step_sand1",   "9e59c3650c6c3fc0a475f1b753b2fcfef430bf81" }, { "step_sand2",   "0fa4234797f336ada4e3735e013e44d1099afe57" },
	{ "step_sand3",   "c75589cc0087069f387de127dd1499580498738e" }, { "step_sand4",   "37afa06f97d58767a1cd1382386db878be1532dd" },
	{ "step_snow1",   "e9bab7d3d15541f0aaa93fad31ad37fd07e03a6c" }, { "step_snow2",   "5887d10234c4f244ec5468080412f3e6ef9522f3" },
	{ "step_snow3",   "a4bc069321a96236fde04a3820664cc23b2ea619" }, { "step_snow4",   "e26fa3036cdab4c2264ceb19e1cd197a2a510227" },
	{ "step_stone1",  "4e094ed8dfa98656d8fec52a7d20c5ee6098b6ad" }, { "step_stone2",  "9c92f697142ae320584bf64c0d54381d59703528" },
	{ "step_stone3",  "8f23c02475d388b23e5faa680eafe6b991d7a9d4" }, { "step_stone4",  "363545a76277e5e47538b2dd3a0d6aa4f7a87d34" },
	{ "step_wood1",   "9bc2a84d0aa98113fc52609976fae8fc88ea6333" }, { "step_wood2",   "98102533e6085617a2962157b4f3658f59aea018" },
	{ "step_wood3",   "45b2aef7b5049e81b39b58f8d631563fadcc778b" }, { "step_wood4",   "dc66978374a46ab2b87db6472804185824868095" }
};

static struct MusicResource {
	const char* name;
	const char* hash;
	short size;
	cc_bool downloaded;
	int reqID;
} musicResources[7] = {
	{ "calm1.ogg", "50a59a4f56e4046701b758ddbb1c1587efa4cadf", 2472 },
	{ "calm2.ogg", "74da65c99aa578486efa7b69983d3533e14c0d6e", 1931 },
	{ "calm3.ogg", "14ae57a6bce3d4254daa8be2b098c2d99743cc3f", 2181 },
	{ "hal1.ogg",  "df1ff11b79757432c5c3f279e5ecde7b63ceda64", 1926 },
	{ "hal2.ogg",  "ceaaaa1d57dfdfbb0bd4da5ea39628b42897a687", 1714 },
	{ "hal3.ogg",  "dd85fb564e96ee2dbd4754f711ae9deb08a169f9", 1879 },
	{ "hal4.ogg",  "5e7d63e75c6e042f452bc5e151276911ef92fed8", 2499 }
};


/*########################################################################################################################*
*---------------------------------------------------------List/Checker----------------------------------------------------*
*#########################################################################################################################*/
int Resources_Count, Resources_Size;
static cc_bool allSoundsExist, allTexturesExist;
static int texturesFound;

CC_NOINLINE static struct ResourceTexture* Resources_FindTex(const cc_string* name) {
	struct ResourceTexture* tex;
	int i;

	for (i = 0; i < Array_Elems(textureResources); i++) {
		tex = &textureResources[i];
		if (String_CaselessEqualsConst(name, tex->filename)) return tex;
	}
	return NULL;
}

static void Resources_CheckMusic(void) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	int i;
	String_InitArray(path, pathBuffer);

	for (i = 0; i < Array_Elems(musicResources); i++) {
		path.length = 0;
		String_Format1(&path, "audio/%c", musicResources[i].name);

		musicResources[i].downloaded = File_Exists(&path);
		if (musicResources[i].downloaded) continue;

		Resources_Size += musicResources[i].size;
		Resources_Count++;
	}
}

static void Resources_CheckSounds(void) {
	cc_string path; char pathBuffer[FILENAME_SIZE];
	int i;
	String_InitArray(path, pathBuffer);

	for (i = 0; i < Array_Elems(soundResources); i++) {
		path.length = 0;
		String_Format1(&path, "audio/%c.wav", soundResources[i].name);

		if (File_Exists(&path)) continue;
		allSoundsExist = false;

		Resources_Count += Array_Elems(soundResources);
		Resources_Size  += 417;
		return;
	}
	allSoundsExist = true;
}

static cc_bool Resources_SelectZipEntry(const cc_string* path) {
	cc_string name = *path;
	Utils_UNSAFE_GetFilename(&name);

	if (Resources_FindTex(&name)) texturesFound++;
	return false;
}

static void Resources_CheckTextures(void) {
	static const cc_string path = String_FromConst("texpacks/default.zip");
	struct Stream stream;
	struct ZipState state;
	cc_result res;

	res = Stream_OpenFile(&stream, &path);
	if (res == ReturnCode_FileNotFound) return;

	if (res) { Logger_SysWarn(res, "checking default.zip"); return; }
	Zip_Init(&state, &stream);
	state.SelectEntry = Resources_SelectZipEntry;

	res = Zip_Extract(&state);
	stream.Close(&stream);
	if (res) Logger_SysWarn(res, "inspecting default.zip");

	/* if somehow have say "gui.png", "GUI.png" */
	allTexturesExist = texturesFound >= Array_Elems(textureResources);
}

void Resources_CheckExistence(void) {
	int i;
	Resources_Count = 0;
	Resources_Size  = 0;

	Resources_CheckTextures();
	Resources_CheckMusic();
	Resources_CheckSounds();

	if (allTexturesExist) return;
	for (i = 0; i < Array_Elems(fileResources); i++) {
		Resources_Count++;
		Resources_Size += fileResources[i].size;
	}
}


/*########################################################################################################################*
*---------------------------------------------------------Zip writer------------------------------------------------------*
*#########################################################################################################################*/
static cc_result ZipPatcher_LocalFile(struct Stream* s, struct ResourceTexture* e) {
	int filenameLen = String_Length(e->filename);
	cc_uint8 header[30 + STRING_SIZE];
	cc_result res;
	if ((res = s->Position(s, &e->offset))) return res;

	Stream_SetU32_LE(header + 0,  0x04034b50);  /* signature */
	Stream_SetU16_LE(header + 4,  20);          /* version needed */
	Stream_SetU16_LE(header + 6,  0);           /* bitflags */
	Stream_SetU16_LE(header + 8,  0);           /* compression method */
	Stream_SetU16_LE(header + 10, 0);           /* last modified */
	Stream_SetU16_LE(header + 12, 0);           /* last modified */
	
	Stream_SetU32_LE(header + 14, e->crc32);    /* CRC32 */
	Stream_SetU32_LE(header + 18, e->size);     /* Compressed size */
	Stream_SetU32_LE(header + 22, e->size);     /* Uncompressed size */
	 
	Stream_SetU16_LE(header + 26, filenameLen); /* name length */
	Stream_SetU16_LE(header + 28, 0);           /* extra field length */

	Mem_Copy(header + 30, e->filename, filenameLen);
	return Stream_Write(s, header, 30 + filenameLen);
}

static cc_result ZipPatcher_CentralDir(struct Stream* s, struct ResourceTexture* e) {
	int filenameLen = String_Length(e->filename);
	cc_uint8 header[46 + STRING_SIZE];
	struct DateTime now;
	int modTime, modDate;

	DateTime_CurrentLocal(&now);
	modTime = (now.second / 2) | (now.minute << 5) | (now.hour << 11);
	modDate = (now.day) | (now.month << 5) | ((now.year - 1980) << 9);

	Stream_SetU32_LE(header + 0,  0x02014b50);  /* signature */
	Stream_SetU16_LE(header + 4,  20);          /* version */
	Stream_SetU16_LE(header + 6,  20);          /* version needed */
	Stream_SetU16_LE(header + 8,  0);           /* bitflags */
	Stream_SetU16_LE(header + 10, 0);           /* compression method */
	Stream_SetU16_LE(header + 12, modTime);     /* last modified */
	Stream_SetU16_LE(header + 14, modDate);     /* last modified */

	Stream_SetU32_LE(header + 16, e->crc32);    /* CRC32 */
	Stream_SetU32_LE(header + 20, e->size);     /* compressed size */
	Stream_SetU32_LE(header + 24, e->size);     /* uncompressed size */

	Stream_SetU16_LE(header + 28, filenameLen); /* name length */
	Stream_SetU16_LE(header + 30, 0);           /* extra field length */
	Stream_SetU16_LE(header + 32, 0);           /* file comment length */
	Stream_SetU16_LE(header + 34, 0);           /* disk number */
	Stream_SetU16_LE(header + 36, 0);           /* internal attributes */
	Stream_SetU32_LE(header + 38, 0);           /* external attributes */
	Stream_SetU32_LE(header + 42, e->offset);   /* local header offset */

	Mem_Copy(header + 46, e->filename, filenameLen);
	return Stream_Write(s, header, 46 + filenameLen);
}

static cc_result ZipPatcher_EndOfCentralDir(struct Stream* s, cc_uint32 centralDirBeg, cc_uint32 centralDirEnd) {
	cc_uint8 header[22];

	Stream_SetU32_LE(header + 0,  0x06054b50); /* signature */
	Stream_SetU16_LE(header + 4,  0);          /* disk number */
	Stream_SetU16_LE(header + 6,  0);          /* disk number of start */
	Stream_SetU16_LE(header + 8,  Array_Elems(textureResources));  /* disk entries */
	Stream_SetU16_LE(header + 10, Array_Elems(textureResources));  /* total entries */
	Stream_SetU32_LE(header + 12, centralDirEnd - centralDirBeg);  /* central dir size */
	Stream_SetU32_LE(header + 16, centralDirBeg);                  /* central dir start */
	Stream_SetU16_LE(header + 20, 0);         /* comment length */
	return Stream_Write(s, header, 22);
}

static cc_result ZipPatcher_FixupLocalFile(struct Stream* s, struct ResourceTexture* e) {
	int filenameLen = String_Length(e->filename);
	cc_uint8 tmp[2048];
	cc_uint32 dataBeg, dataEnd;
	cc_uint32 i, crc, toRead, read;
	cc_result res;

	dataBeg = e->offset + 30 + filenameLen;
	if ((res = s->Position(s, &dataEnd))) return res;
	e->size = dataEnd - dataBeg;

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
	e->crc32 = crc ^ 0xffffffffUL;

	/* then fixup the header */
	if ((res = s->Seek(s, e->offset)))      return res;
	if ((res = ZipPatcher_LocalFile(s, e))) return res;
	return s->Seek(s, dataEnd);
}

static cc_result ZipPatcher_WriteData(struct Stream* dst, struct ResourceTexture* tex, const cc_uint8* data, cc_uint32 len) {
	cc_result res;
	tex->size  = len;
	tex->crc32 = Utils_CRC32(data, len);

	res = ZipPatcher_LocalFile(dst, tex);
	if (res) return res;
	return Stream_Write(dst, data, len);
}

static cc_result ZipPatcher_WriteZipEntry(struct Stream* src, struct ResourceTexture* tex, struct ZipState* state) {
	cc_uint8 tmp[2048];
	cc_uint32 read;
	struct Stream* dst = (struct Stream*)state->obj;
	cc_result res;

	tex->size  = state->_curEntry->UncompressedSize;
	tex->crc32 = state->_curEntry->CRC32;
	res = ZipPatcher_LocalFile(dst, tex);
	if (res) return res;

	for (;;) {
		res = src->Read(src, tmp, sizeof(tmp), &read);
		if (res)   return res;
		if (!read) break;

		if ((res = Stream_Write(dst, tmp, read))) return res;
	}
	return 0;
}

static cc_result ZipPatcher_WritePng(struct Stream* s, struct ResourceTexture* tex, struct Bitmap* src) {
	cc_result res;

	if ((res = ZipPatcher_LocalFile(s, tex)))   return res;
	if ((res = Png_Encode(src, s, NULL, true))) return res;
	return ZipPatcher_FixupLocalFile(s, tex);
}


/*########################################################################################################################*
*-------------------------------------------------------Texture patcher---------------------------------------------------*
*#########################################################################################################################*/
#define ANIMS_TXT \
"# This file defines the animations used in a texture pack for ClassiCube.\r\n" \
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
"# NOTE: If a file called 'uselavaanim' is in the texture pack, the game instead generates the lava texture animation.\r\n" \
"# NOTE: If a file called 'usewateranim' is in the texture pack, the game instead generates the water texture animation.\r\n" \
"\r\n" \
"# fire\r\n" \
"6 2 0 0 16 32 0"
static struct Bitmap terrainBmp;

static cc_bool ClassicPatcher_SelectEntry(const cc_string* path) {
	cc_string name = *path;
	Utils_UNSAFE_GetFilename(&name);
	return Resources_FindTex(&name) != NULL;
}

static cc_result ClassicPatcher_ProcessEntry(const cc_string* path, struct Stream* data, struct ZipState* state) {
	static const cc_string guiClassicPng = String_FromConst("gui_classic.png");
	struct ResourceTexture* entry;
	cc_string name;

	/* terrain.png requires special patching */
	if (String_CaselessEqualsConst(path, "terrain.png")) {
		return Png_Decode(&terrainBmp, data);
	}

	name = *path;
	Utils_UNSAFE_GetFilename(&name);
	if (String_CaselessEqualsConst(&name, "gui.png")) name = guiClassicPng;

	entry = Resources_FindTex(&name);
	return ZipPatcher_WriteZipEntry(data, entry, state);
}

static cc_result ClassicPatcher_ExtractFiles(struct Stream* s) {
	struct ZipState zip;
	struct Stream src;

	Stream_ReadonlyMemory(&src, fileResources[0].data, fileResources[0].len);
	Zip_Init(&zip, &src);

	zip.obj = s;
	zip.SelectEntry  = ClassicPatcher_SelectEntry;
	zip.ProcessEntry = ClassicPatcher_ProcessEntry;
	return Zip_Extract(&zip);
}

/* the x,y of tiles in terrain.png which get patched */
static const struct TilePatch { const char* name; cc_uint8 x1,y1, x2,y2; } modern_tiles[12] = {
	{ "assets/minecraft/textures/blocks/sandstone_bottom.png", 9,3 },
	{ "assets/minecraft/textures/blocks/sandstone_normal.png", 9,2 },
	{ "assets/minecraft/textures/blocks/sandstone_top.png", 9,1, },
	{ "assets/minecraft/textures/blocks/quartz_block_lines_top.png", 10,3, 10,1 },
	{ "assets/minecraft/textures/blocks/quartz_block_lines.png", 10,2 },
	{ "assets/minecraft/textures/blocks/stonebrick.png", 4,3 },
	{ "assets/minecraft/textures/blocks/snow.png", 2,3 },
	{ "assets/minecraft/textures/blocks/wool_colored_blue.png",  3,5 },
	{ "assets/minecraft/textures/blocks/wool_colored_brown.png", 2,5 },
	{ "assets/minecraft/textures/blocks/wool_colored_cyan.png",  4,5 },
	{ "assets/minecraft/textures/blocks/wool_colored_green.png", 1,5 },
	{ "assets/minecraft/textures/blocks/wool_colored_pink.png",  0,5 }
};

CC_NOINLINE static const struct TilePatch* ModernPatcher_GetTile(const cc_string* path) {
	int i;
	for (i = 0; i < Array_Elems(modern_tiles); i++) {
		if (String_CaselessEqualsConst(path, modern_tiles[i].name)) return &modern_tiles[i];
	}
	return NULL;
}

static cc_result ModernPatcher_PatchTile(struct Stream* data, const struct TilePatch* tile) {
	struct Bitmap bmp;
	cc_result res;

	if ((res = Png_Decode(&bmp, data))) return res;
	Bitmap_UNSAFE_CopyBlock(0, 0, tile->x1 * 16, tile->y1 * 16, &bmp, &terrainBmp, 16);

	/* only quartz needs copying to two tiles */
	if (tile->y2) {
		Bitmap_UNSAFE_CopyBlock(0, 0, tile->x2 * 16, tile->y2 * 16, &bmp, &terrainBmp, 16);
	}

	Mem_Free(bmp.scan0);
	return 0;
}


static cc_bool ModernPatcher_SelectEntry(const cc_string* path) {
	return
		String_CaselessEqualsConst(path, "assets/minecraft/textures/environment/snow.png") ||
		String_CaselessEqualsConst(path, "assets/minecraft/textures/entity/chicken.png")   ||
		String_CaselessEqualsConst(path, "assets/minecraft/textures/blocks/fire_layer_1.png") ||
		ModernPatcher_GetTile(path) != NULL;
}

static cc_result ModernPatcher_MakeAnimations(struct Stream* s, struct Stream* data) {
	static const cc_string animsPng = String_FromConst("animations.png");
	struct ResourceTexture* entry;
	BitmapCol pixels[512 * 16];
	struct Bitmap anim, bmp;
	cc_result res;
	int i;

	if ((res = Png_Decode(&bmp, data))) return res;
	Bitmap_Init(anim, 512, 16, pixels);

	for (i = 0; i < 512; i += 16) {
		Bitmap_UNSAFE_CopyBlock(0, i, i, 0, &bmp, &anim, 16);
	}

	Mem_Free(bmp.scan0);
	entry = Resources_FindTex(&animsPng);
	return ZipPatcher_WritePng(s, entry, &anim);
}

static cc_result ModernPatcher_ProcessEntry(const cc_string* path, struct Stream* data, struct ZipState* state) {
	struct ResourceTexture* entry;
	const struct TilePatch* tile;
	cc_string name;

	if (String_CaselessEqualsConst(path, "assets/minecraft/textures/environment/snow.png")
		|| String_CaselessEqualsConst(path, "assets/minecraft/textures/entity/chicken.png")) {
		name = *path;
		Utils_UNSAFE_GetFilename(&name);

		entry = Resources_FindTex(&name);
		return ZipPatcher_WriteZipEntry(data, entry, state);
	}

	if (String_CaselessEqualsConst(path, "assets/minecraft/textures/blocks/fire_layer_1.png")) {
		struct Stream* dst = (struct Stream*)state->obj;
		return ModernPatcher_MakeAnimations(dst, data);
	}

	tile = ModernPatcher_GetTile(path);
	return ModernPatcher_PatchTile(data, tile);
}

static cc_result ModernPatcher_ExtractFiles(struct Stream* s) {
	struct ZipState zip;
	struct Stream src;

	Stream_ReadonlyMemory(&src, fileResources[1].data, fileResources[1].len);
	Zip_Init(&zip, &src);

	zip.obj = s;
	zip.SelectEntry  = ModernPatcher_SelectEntry;
	zip.ProcessEntry = ModernPatcher_ProcessEntry;
	return Zip_Extract(&zip);
}

#ifdef CC_BUILD_ANDROID
/* Android has both a gui.png and a touch.png */
/* TODO: Unify both android and desktop platforms to both just extract from default.zip */
static cc_bool TexPatcher_SelectEntry(const cc_string* path) {
	return String_CaselessEqualsConst(path, "gui.png") || String_CaselessEqualsConst(path, "touch.png");
}
static cc_result TexPatcher_ProcessEntry(const cc_string* path, struct Stream* data, struct ZipState* state) {
	struct ResourceTexture* entry = Resources_FindTex(path);
	return ZipPatcher_WriteZipEntry(data, entry, state);
}

static cc_result TexPactcher_ExtractGui(struct Stream* s) {
	struct ZipState zip;
	struct Stream src;

	Stream_ReadonlyMemory(&src, fileResources[3].data, fileResources[3].len);
	Zip_Init(&zip, &src);

	zip.obj = s;
	zip.SelectEntry  = TexPatcher_SelectEntry;
	zip.ProcessEntry = TexPatcher_ProcessEntry;
	return Zip_Extract(&zip);
}
#else
static cc_result TexPactcher_ExtractGui(struct Stream* s) {
	static const cc_string guiPng = String_FromConst("gui.png");
	struct ResourceTexture* entry = Resources_FindTex(&guiPng);
	return ZipPatcher_WriteData(s, entry, fileResources[3].data, fileResources[3].len);
}
#endif

static cc_result TexPatcher_NewFiles(struct Stream* s) {
	static const cc_string guiPng   = String_FromConst("gui.png");
	static const cc_string animsTxt = String_FromConst("animations.txt");
	struct ResourceTexture* entry;
	cc_result res;

	/* make default animations.txt */
	entry = Resources_FindTex(&animsTxt);
	res   = ZipPatcher_WriteData(s, entry, (const cc_uint8*)ANIMS_TXT, sizeof(ANIMS_TXT) - 1);
	if (res) return res;

	/* make ClassiCube gui.png */
	return TexPactcher_ExtractGui(s);
}

static void TexPatcher_PatchTile(struct Bitmap* src, int srcX, int srcY, int dstX, int dstY) {
	Bitmap_UNSAFE_CopyBlock(srcX, srcY, dstX * 16, dstY * 16, src, &terrainBmp, 16);
}

static cc_result TexPatcher_Terrain(struct Stream* s) {
	static const cc_string terrainPng = String_FromConst("terrain.png");
	struct ResourceTexture* entry;
	struct Bitmap bmp;
	struct Stream src;
	cc_result res;

	Stream_ReadonlyMemory(&src, fileResources[2].data, fileResources[2].len);
	if ((res = Png_Decode(&bmp, &src))) return res;

	TexPatcher_PatchTile(&bmp,  0,0, 3,3);
	TexPatcher_PatchTile(&bmp, 16,0, 6,3);
	TexPatcher_PatchTile(&bmp, 32,0, 6,2);

	TexPatcher_PatchTile(&bmp,  0,16,  5,3);
	TexPatcher_PatchTile(&bmp, 16,16,  6,5);
	TexPatcher_PatchTile(&bmp, 32,16, 11,0);

	entry = Resources_FindTex(&terrainPng);
	res   = ZipPatcher_WritePng(s, entry, &terrainBmp);
	Mem_Free(bmp.scan0);
	return res;
}

static cc_result TexPatcher_WriteEntries(struct Stream* s) {
	cc_uint32 beg, end;
	int i;
	cc_result res;

	if ((res = ClassicPatcher_ExtractFiles(s))) return res;
	if ((res = ModernPatcher_ExtractFiles(s)))  return res;
	if ((res = TexPatcher_NewFiles(s)))         return res;
	if ((res = TexPatcher_Terrain(s)))          return res;
	
	if ((res = s->Position(s, &beg))) return res;
	for (i = 0; i < Array_Elems(textureResources); i++) {
		if ((res = ZipPatcher_CentralDir(s, &textureResources[i]))) return res;
	}

	if ((res = s->Position(s, &end))) return res;
	return ZipPatcher_EndOfCentralDir(s, beg, end);
}

static void TexPatcher_MakeDefaultZip(void) {
	static const cc_string path = String_FromConst("texpacks/default.zip");
	struct Stream s;
	int i;
	cc_result res;

	res = Stream_CreateFile(&s, &path);
	if (res) {
		Logger_SysWarn(res, "creating default.zip");
	} else {
		res = TexPatcher_WriteEntries(&s);
		if (res) Logger_SysWarn(res, "making default.zip");

		res = s.Close(&s);
		if (res) Logger_SysWarn(res, "closing default.zip");
	}

	for (i = 0; i < Array_Elems(fileResources); i++) {
		Mem_Free(fileResources[i].data);
		fileResources[i].data = NULL;
	}
}


/*########################################################################################################################*
*--------------------------------------------------------Audio patcher----------------------------------------------------*
*#########################################################################################################################*/
#define WAV_FourCC(a, b, c, d) (((cc_uint32)a << 24) | ((cc_uint32)b << 16) | ((cc_uint32)c << 8) | (cc_uint32)d)
#define WAV_HDR_SIZE 44

/* Fixes up the .WAV header after having written all samples */
static void SoundPatcher_FixupHeader(struct Stream* s, struct VorbisState* ctx, cc_uint32 len) {
	cc_uint8 header[WAV_HDR_SIZE];
	cc_result res = s->Seek(s, 0);
	if (res) { Logger_SysWarn(res, "seeking to .wav start"); return; }

	Stream_SetU32_BE(header +  0, WAV_FourCC('R','I','F','F'));
	Stream_SetU32_LE(header +  4, len - 8);
	Stream_SetU32_BE(header +  8, WAV_FourCC('W','A','V','E'));
	Stream_SetU32_BE(header + 12, WAV_FourCC('f','m','t',' '));
	Stream_SetU32_LE(header + 16, 16); /* fmt chunk size */
	Stream_SetU16_LE(header + 20, 1);  /* PCM audio format */
	Stream_SetU16_LE(header + 22, ctx->channels);
	Stream_SetU32_LE(header + 24, ctx->sampleRate);

	Stream_SetU32_LE(header + 28, ctx->sampleRate * ctx->channels * 2); /* byte rate */
	Stream_SetU16_LE(header + 32, ctx->channels * 2);                   /* block align */
	Stream_SetU16_LE(header + 34, 16);                                  /* bits per sample */
	Stream_SetU32_BE(header + 36, WAV_FourCC('d','a','t','a'));
	Stream_SetU32_LE(header + 40, len - WAV_HDR_SIZE);

	res = Stream_Write(s, header, WAV_HDR_SIZE);
	if (res) Logger_SysWarn(res, "fixing .wav header");
}

/* Decodes all samples, then produces a .WAV file from them */
static void SoundPatcher_WriteWav(struct Stream* s, struct VorbisState* ctx) {
	cc_int16* samples;
	cc_uint32 len = WAV_HDR_SIZE;
	cc_result res;
	int count;

	/* ctx is all 0, so reuse here for empty header */
	res = Stream_Write(s, (const cc_uint8*)ctx, WAV_HDR_SIZE);
	if (res) { Logger_SysWarn(res, "writing .wav header"); return; }

	res = Vorbis_DecodeHeaders(ctx);
	if (res) { Logger_SysWarn(res, "decoding .ogg header"); return; }

	samples = (cc_int16*)Mem_TryAlloc(ctx->blockSizes[1] * ctx->channels, 2);
	if (!samples) { Logger_SysWarn(ERR_OUT_OF_MEMORY, "allocating .ogg samples"); return; }

	for (;;) {
		res = Vorbis_DecodeFrame(ctx);
		if (res == ERR_END_OF_STREAM) {
			/* reached end of samples, so done */
			SoundPatcher_FixupHeader(s, ctx, len); break;
		}
		if (res) { Logger_SysWarn(res, "decoding .ogg"); break; }

		count = Vorbis_OutputFrame(ctx, samples);
		len  += count * 2;
		/* TODO: Do we need to account for big endian */
		res = Stream_Write(s, samples, count * 2);
		if (res) { Logger_SysWarn(res, "writing samples"); break; }
	}
	Mem_Free(samples);
}

static void SoundPatcher_Save(const char* name, struct HttpRequest* req) {
	cc_string path; char pathBuffer[STRING_SIZE];
	struct OggState ogg;
	struct Stream src, dst;
	struct VorbisState ctx = { 0 };
	cc_result res;

	Stream_ReadonlyMemory(&src, req->data, req->size);
	String_InitArray(path, pathBuffer);
	String_Format1(&path, "audio/%c.wav", name);

	res = Stream_CreateFile(&dst, &path);
	if (res) { Logger_SysWarn(res, "creating .wav file"); return; }

	Ogg_Init(&ogg, &src);
	ctx.source = &ogg;
	SoundPatcher_WriteWav(&dst, &ctx);

	res = dst.Close(&dst);
	if (res) Logger_SysWarn(res, "closing .wav file");
	Vorbis_Free(&ctx);
}

static void MusicPatcher_Save(const char* name, struct HttpRequest* req) {
	cc_string path; char pathBuffer[STRING_SIZE];
	cc_result res;

	String_InitArray(path, pathBuffer);
	String_Format1(&path, "audio/%c", name);

	res = Stream_WriteAllTo(&path, req->data, req->size);
	if (res) Logger_SysWarn(res, "saving music file");
}


/*########################################################################################################################*
*-----------------------------------------------------------Fetcher-------------------------------------------------------*
*#########################################################################################################################*/
cc_bool Fetcher_Working, Fetcher_Completed, Fetcher_Failed;
int  Fetcher_StatusCode, Fetcher_Downloaded;
cc_result Fetcher_Result;

CC_NOINLINE static int Fetcher_DownloadAudio(const char* hash) {
	cc_string url; char urlBuffer[URL_MAX_SIZE];

	String_InitArray(url, urlBuffer);
	String_Format3(&url, "http://resources.download.minecraft.net/%r%r/%c", 
					&hash[0], &hash[1], hash);
	return Http_AsyncGetData(&url, false);
}

const char* Fetcher_RequestName(int reqID) {
	int i;

	for (i = 0; i < Array_Elems(fileResources); i++) {
		if (reqID == fileResources[i].reqID)  return fileResources[i].name;
	}
	for (i = 0; i < Array_Elems(musicResources); i++) {
		if (reqID == musicResources[i].reqID) return musicResources[i].name;
	}
	for (i = 0; i < Array_Elems(soundResources); i++) {
		if (reqID == soundResources[i].reqID) return soundResources[i].name;
	}
	return NULL;
}

void Fetcher_Run(void) {
	cc_string url;
	int i;

	Fetcher_Failed     = false;
	Fetcher_Downloaded = 0;
	Fetcher_Working    = true;
	Fetcher_Completed  = false;

	for (i = 0; i < Array_Elems(fileResources); i++) {
		if (allTexturesExist) continue;

		url = String_FromReadonly(fileResources[i].url);
		fileResources[i].reqID = Http_AsyncGetData(&url, false);
	}

	for (i = 0; i < Array_Elems(musicResources); i++) {
		if (musicResources[i].downloaded) continue;
		musicResources[i].reqID = Fetcher_DownloadAudio(musicResources[i].hash);
	}
	for (i = 0; i < Array_Elems(soundResources); i++) {
		if (allSoundsExist) continue;
		soundResources[i].reqID = Fetcher_DownloadAudio(soundResources[i].hash);
	}
}

static void Fetcher_Finish(void) {
	Fetcher_Completed = true;
	Fetcher_Working   = false;
}

CC_NOINLINE static cc_bool Fetcher_Get(int reqID, struct HttpRequest* req) {
	if (!Http_GetResult(reqID, req)) return false;

	if (req->success) {
		Fetcher_Downloaded++;
		return true;
	}

	Fetcher_Failed     = true;
	Fetcher_Result     = req->result;
	Fetcher_StatusCode = req->statusCode;
	Fetcher_Finish();
	return false;
}

static void Fetcher_CheckFile(struct FileResource* file) {
	struct HttpRequest req;
	if (!Fetcher_Get(file->reqID, &req)) return;
	
	file->downloaded = true;
	file->data       = req.data;
	file->len        = req.size;
	/* don't free request */
}

static void Fetcher_CheckMusic(struct MusicResource* music) {
	struct HttpRequest req;
	if (!Fetcher_Get(music->reqID, &req)) return;

	music->downloaded = true;
	MusicPatcher_Save(music->name, &req);
	Mem_Free(req.data);
}

static void Fetcher_CheckSound(const struct SoundResource* sound) {
	struct HttpRequest req;
	if (!Fetcher_Get(sound->reqID, &req)) return;

	SoundPatcher_Save(sound->name, &req);
	Mem_Free(req.data);
}

/* TODO: Implement this.. */
/* TODO: How expensive is it to constantly do 'Get' over and make all these strings */
void Fetcher_Update(void) {
	int i;

	for (i = 0; i < Array_Elems(fileResources); i++) {
		if (fileResources[i].downloaded) continue;
		Fetcher_CheckFile(&fileResources[i]);
	}
	if (fileResources[3].data) TexPatcher_MakeDefaultZip();

	for (i = 0; i < Array_Elems(musicResources); i++) {
		if (musicResources[i].downloaded) continue;
		Fetcher_CheckMusic(&musicResources[i]);
	}

	for (i = 0; i < Array_Elems(soundResources); i++) {
		Fetcher_CheckSound(&soundResources[i]);
	}

	if (Fetcher_Downloaded != Resources_Count) return; 
	Fetcher_Finish();
}
#endif
