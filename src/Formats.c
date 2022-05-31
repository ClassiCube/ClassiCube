#include "Formats.h"
#include "String.h"
#include "World.h"
#include "Deflate.h"
#include "Block.h"
#include "Entity.h"
#include "Platform.h"
#include "ExtMath.h"
#include "Logger.h"
#include "Game.h"
#include "Server.h"
#include "Event.h"
#include "Funcs.h"
#include "Errors.h"
#include "Stream.h"
#include "Chat.h"
#include "Inventory.h"
#include "TexturePack.h"
#include "Utils.h"


/*########################################################################################################################*
*--------------------------------------------------------General----------------------------------------------------------*
*#########################################################################################################################*/
static cc_result Map_ReadBlocks(struct Stream* stream) {
	World.Volume = World.Width * World.Length * World.Height;
	World.Blocks = (BlockRaw*)Mem_TryAlloc(World.Volume, 1);

	if (!World.Blocks) return ERR_OUT_OF_MEMORY;
	return Stream_Read(stream, World.Blocks, World.Volume);
}

static cc_result Map_SkipGZipHeader(struct Stream* stream) {
	struct GZipHeader gzHeader;
	cc_result res;
	GZipHeader_Init(&gzHeader);

	while (!gzHeader.done) {
		if ((res = GZipHeader_Read(stream, &gzHeader))) return res;
	}
	return 0;
}

IMapImporter Map_FindImporter(const cc_string* path) {
	static const cc_string cw   = String_FromConst(".cw"),  lvl = String_FromConst(".lvl");
	static const cc_string fcm  = String_FromConst(".fcm"), dat = String_FromConst(".dat");
	static const cc_string mine = String_FromConst(".mine");

	if (String_CaselessEnds(path,   &cw))  return Cw_Load;
	if (String_CaselessEnds(path,  &lvl)) return Lvl_Load;
	if (String_CaselessEnds(path,  &fcm)) return Fcm_Load;
	if (String_CaselessEnds(path,  &dat)) return Dat_Load;
	if (String_CaselessEnds(path, &mine)) return Dat_Load;

	return NULL;
}

cc_result Map_LoadFrom(const cc_string* path) {
	cc_string relPath, fileName, fileExt;
	IMapImporter importer;
	struct Stream stream;
	cc_result res;
	Game_Reset();
	
	res = Stream_OpenFile(&stream, path);
	if (res) { Logger_SysWarn2(res, "opening", path); return res; }

	importer = Map_FindImporter(path);
	if (!importer) {
		res = ERR_NOT_SUPPORTED;
	} else if ((res = importer(&stream))) {
		World_Reset();
	}

	/* No point logging error for closing readonly file */
	(void)stream.Close(&stream);
	if (res) Logger_SysWarn2(res, "decoding", path);

	World_SetNewMap(World.Blocks, World.Width, World.Height, World.Length);
	LocalPlayer_MoveToSpawn();

	relPath = *path;
	Utils_UNSAFE_GetFilename(&relPath);
	String_UNSAFE_Separate(&relPath, '.', &fileName, &fileExt);
	String_Copy(&World.Name, &fileName);
	return res;
}


/*########################################################################################################################*
*--------------------------------------------------MCSharp level Format---------------------------------------------------*
*#########################################################################################################################*/
#define LVL_CUSTOMTILE 163
#define LVL_CHUNKSIZE 16
/* MCSharp* format is a GZIP compressed binary map format. All metadata is discarded.
	U16 "Identifier" (must be 1874)
	U16 "Width",  "Length", "Height"
	U16 "SpawnX", "SpawnZ", "SpawnY"
	U8  "Yaw", "Pitch"
	U16 "Build permissions" (ignored)
	U8* "Blocks"
	
	-- this data is only in MCGalaxy maps
	U8  "Identifier" (0xBD for 'block definitions', i.e. custom blocks)
	U8* "Data"       (16x16x16 sparsely allocated chunks)
}*/

static const cc_uint8 Lvl_table[256] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65,  0,  0,  0,  0, 39, 36, 36, 10, 46, 21, 22, 22, 22, 22,
	 4,  0, 22, 21,  0, 22, 23, 24, 22, 26, 27, 28, 30, 31, 32, 33,
	34, 35, 36, 22, 20, 49, 45,  1,  4,  0,  9, 11,  4, 19,  5, 17,
	10, 49, 20,  1, 18, 12,  5, 25, 46, 44, 17, 49, 20,  1, 18, 12,
	 5, 25, 36, 34,  0,  9, 11, 46, 44,  0,  9, 11,  8, 10, 22, 27,
	22,  8, 10, 28, 17, 49, 20,  1, 18, 12,  5, 25, 46, 44, 11,  9,
	 0,  9, 11, 163, 0,  0,  9, 11,  0,  0,  0,  0,  0,  0,  0, 28,
	22, 21, 11,  0,  0,  0, 46, 46, 10, 10, 46, 20, 41, 42, 11,  9,
	 0,  8, 10, 10,  8,  0, 22, 22,  0,  0,  0,  0,  0,  0,  0,  0,
	 0,  0,  0, 21, 10,  0,  0,  0,  0,  0, 22, 22, 42,  3,  2, 29,
	47,  0,  0,  0,  0,  0, 27, 46, 48, 24, 22, 36, 34,  8, 10, 21,
	29, 22, 10, 22, 22, 41, 19, 35, 21, 29, 49, 34, 16, 41,  0, 22
};

static cc_result Lvl_ReadCustomBlocks(struct Stream* stream) {	
	cc_uint8 chunk[LVL_CHUNKSIZE * LVL_CHUNKSIZE * LVL_CHUNKSIZE];
	cc_uint8 hasCustom;
	int baseIndex, index, xx, yy, zz;
	cc_result res;
	int x, y, z, i;

	/* skip bounds checks when we know chunk is entirely inside map */
	int adjWidth  = World.Width  & ~0x0F;
	int adjHeight = World.Height & ~0x0F;
	int adjLength = World.Length & ~0x0F;

	for (y = 0; y < World.Height; y += LVL_CHUNKSIZE) {
		for (z = 0; z < World.Length; z += LVL_CHUNKSIZE) {
			for (x = 0; x < World.Width; x += LVL_CHUNKSIZE) {

				if ((res = stream->ReadU8(stream, &hasCustom))) return res;
				if (hasCustom != 1) continue;
				if ((res = Stream_Read(stream, chunk, sizeof(chunk)))) return res;
				baseIndex = World_Pack(x, y, z);

				if ((x + LVL_CHUNKSIZE) <= adjWidth && (y + LVL_CHUNKSIZE) <= adjHeight && (z + LVL_CHUNKSIZE) <= adjLength) {
					for (i = 0; i < sizeof(chunk); i++) {
						xx = i & 0xF; yy = (i >> 8) & 0xF; zz = (i >> 4) & 0xF;

						index = baseIndex + World_Pack(xx, yy, zz);
						World.Blocks[index] = World.Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World.Blocks[index];
					}
				} else {
					for (i = 0; i < sizeof(chunk); i++) {
						xx = i & 0xF; yy = (i >> 8) & 0xF; zz = (i >> 4) & 0xF;
						if ((x + xx) >= World.Width || (y + yy) >= World.Height || (z + zz) >= World.Length) continue;

						index = baseIndex + World_Pack(xx, yy, zz);
						World.Blocks[index] = World.Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World.Blocks[index];
					}
				}
			}
		}
	}
	return 0;
}

cc_result Lvl_Load(struct Stream* stream) {
	cc_uint8 header[18];
	cc_uint8* blocks;
	cc_uint8 section;
	cc_result res;
	int i;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream2(&compStream, &state, stream);
	
	if ((res = Map_SkipGZipHeader(stream)))                       return res;
	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;
	if (Stream_GetU16_LE(&header[0]) != 1874) return LVL_ERR_VERSION;

	World.Width  = Stream_GetU16_LE(&header[2]);
	World.Length = Stream_GetU16_LE(&header[4]);
	World.Height = Stream_GetU16_LE(&header[6]);

	p->Spawn.X = Stream_GetU16_LE(&header[8]);
	p->Spawn.Z = Stream_GetU16_LE(&header[10]);
	p->Spawn.Y = Stream_GetU16_LE(&header[12]);
	p->SpawnYaw   = Math_Packed2Deg(header[14]);
	p->SpawnPitch = Math_Packed2Deg(header[15]);
	/* (2) pervisit, perbuild permissions */

	if ((res = Map_ReadBlocks(&compStream))) return res;
	blocks = World.Blocks;
	/* Bulk convert 4 blocks at once */
	for (i = 0; i < (World.Volume & ~3); i += 4) {
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
	}
	for (; i < World.Volume; i++) {
		*blocks = Lvl_table[*blocks]; blocks++;
	}

	/* 0xBD section type is not present in older .lvl files */
	res = compStream.ReadU8(&compStream, &section);
	if (res == ERR_END_OF_STREAM) return 0;

	if (res) return res;
	/* Unrecognised section type, stop reading */
	if (section != 0xBD) return 0;

	res = Lvl_ReadCustomBlocks(&compStream);
	/* At least one map out there has a corrupted 0xBD section */
	if (res == ERR_END_OF_STREAM) {
		Chat_AddRaw("&cEnd of stream reading .lvl custom blocks section");
		Chat_AddRaw("&c  Some blocks may therefore appear incorrectly");
		res = 0;
	}
	return res;
}


/*########################################################################################################################*
*----------------------------------------------------fCraft map format----------------------------------------------------*
*#########################################################################################################################*/
/* fCraft* format is a binary map format. All metadata is discarded.
	U32 "Identifier" (must be FC2AF40)
	U8  "Revision"   (only '13' supported)
	U16 "Width",  "Height", "Length"
	U32 "SpawnX", "SpawnY", "SpawnZ"
	U8  "Yaw", "Pitch"
	U32 "DateModified", "DateCreated" (ignored)
	U8* "UUID"
	U8  "Layers"     (only maps with 1 layer supported)
	U8* "LayersInfo" (ignored, assumes only layer is map blocks)
	U32 "MetaCount"
	METADATA { STR "Group", "Key", "Value" }
	U8* "Blocks"
}*/
static cc_result Fcm_ReadString(struct Stream* stream) {
	cc_uint8 data[2];
	int len;
	cc_result res;

	if ((res = Stream_Read(stream, data, sizeof(data)))) return res;
	len = Stream_GetU16_LE(data);

	return stream->Skip(stream, len);
}

cc_result Fcm_Load(struct Stream* stream) {
	cc_uint8 header[79];	
	cc_result res;
	int i, count;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream2(&compStream, &state, stream);

	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;
	if (Stream_GetU32_LE(&header[0]) != 0x0FC2AF40UL)        return FCM_ERR_IDENTIFIER;
	if (header[4] != 13) return FCM_ERR_REVISION;
	
	World.Width  = Stream_GetU16_LE(&header[5]);
	World.Height = Stream_GetU16_LE(&header[7]);
	World.Length = Stream_GetU16_LE(&header[9]);
	
	p->Spawn.X = ((int)Stream_GetU32_LE(&header[11])) / 32.0f;
	p->Spawn.Y = ((int)Stream_GetU32_LE(&header[15])) / 32.0f;
	p->Spawn.Z = ((int)Stream_GetU32_LE(&header[19])) / 32.0f;
	p->SpawnYaw   = Math_Packed2Deg(header[23]);
	p->SpawnPitch = Math_Packed2Deg(header[24]);

	/* header[25] (4) date modified */
	/* header[29] (4) date created */
	Mem_Copy(&World.Uuid, &header[33], WORLD_UUID_LEN);
	/* header[49] (26) layer index */
	count = (int)Stream_GetU32_LE(&header[75]);

	/* header isn't compressed, rest of data is though */
	for (i = 0; i < count; i++) {
		if ((res = Fcm_ReadString(&compStream))) return res; /* Group */
		if ((res = Fcm_ReadString(&compStream))) return res; /* Key   */
		if ((res = Fcm_ReadString(&compStream))) return res; /* Value */
	}

	return Map_ReadBlocks(&compStream);
}


/*########################################################################################################################*
*---------------------------------------------------------NBTFile---------------------------------------------------------*
*#########################################################################################################################*/
enum NbtTagType { 
	NBT_END, NBT_I8,  NBT_I16, NBT_I32,  NBT_I64, NBT_F32, 
	NBT_R64, NBT_I8S, NBT_STR, NBT_LIST, NBT_DICT
};

#define NBT_SMALL_SIZE  STRING_SIZE
#define NBT_STRING_SIZE STRING_SIZE
#define NbtTag_IsSmall(tag) ((tag)->dataSize <= NBT_SMALL_SIZE)
struct NbtTag;

struct NbtTag {
	struct NbtTag* parent;
	cc_uint8  type;
	cc_string name;
	cc_uint32 dataSize; /* size of data for arrays */

	union {
		cc_uint8  u8;
		cc_int16  i16;
		cc_uint16 u16;
		cc_uint32 u32;
		float     f32;
		cc_uint8  small[NBT_SMALL_SIZE];
		cc_uint8* big; /* malloc for big byte arrays */
		struct { cc_string text; char buffer[NBT_STRING_SIZE]; } str;
	} value;
	char _nameBuffer[NBT_STRING_SIZE];
	cc_result result;
};

static cc_uint8 NbtTag_U8(struct NbtTag* tag) {
	if (tag->type != NBT_I8) Logger_Abort("Expected I8 NBT tag");
	return tag->value.u8;
}

static cc_int16 NbtTag_I16(struct NbtTag* tag) {
	if (tag->type != NBT_I16) Logger_Abort("Expected I16 NBT tag");
	return tag->value.i16;
}

static cc_uint16 NbtTag_U16(struct NbtTag* tag) {
	if (tag->type != NBT_I16) Logger_Abort("Expected I16 NBT tag");
	return tag->value.u16;
}

static float NbtTag_F32(struct NbtTag* tag) {
	if (tag->type != NBT_F32) Logger_Abort("Expected F32 NBT tag");
	return tag->value.f32;
}

static cc_uint8* NbtTag_U8_Array(struct NbtTag* tag, int minSize) {
	if (tag->type != NBT_I8S) Logger_Abort("Expected I8_Array NBT tag");
	if (tag->dataSize < minSize) Logger_Abort("I8_Array NBT tag too small");

	return NbtTag_IsSmall(tag) ? tag->value.small : tag->value.big;
}

static cc_string NbtTag_String(struct NbtTag* tag) {
	if (tag->type != NBT_STR) Logger_Abort("Expected String NBT tag");
	return tag->value.str.text;
}

static cc_result Nbt_ReadString(struct Stream* stream, cc_string* str) {
	cc_uint8 buffer[NBT_STRING_SIZE * 4];
	int len;
	cc_result res;

	if ((res = Stream_Read(stream, buffer, 2)))   return res;
	len = Stream_GetU16_BE(buffer);

	if (len > Array_Elems(buffer)) return CW_ERR_STRING_LEN;
	if ((res = Stream_Read(stream, buffer, len))) return res;

	String_AppendUtf8(str, buffer, len);
	return 0;
}

typedef void (*Nbt_Callback)(struct NbtTag* tag);
static cc_result Nbt_ReadTag(cc_uint8 typeId, cc_bool readTagName, struct Stream* stream, struct NbtTag* parent, Nbt_Callback callback) {
	struct NbtTag tag;
	cc_uint8 childType;
	cc_uint8 tmp[5];	
	cc_result res;
	cc_uint32 i, count;
	
	if (typeId == NBT_END) return 0;
	tag.type   = typeId; 
	tag.parent = parent;
	tag.dataSize = 0;
	String_InitArray(tag.name, tag._nameBuffer);

	if (readTagName) {
		res = Nbt_ReadString(stream, &tag.name);
		if (res) return res;
	}

	switch (typeId) {
	case NBT_I8:
		res = stream->ReadU8(stream, &tag.value.u8);
		break;
	case NBT_I16:
		res = Stream_Read(stream, tmp, 2);
		tag.value.u16 = Stream_GetU16_BE(tmp);
		break;
	case NBT_I32:
	case NBT_F32:
		res = Stream_ReadU32_BE(stream, &tag.value.u32);
		break;
	case NBT_I64:
	case NBT_R64:
		res = stream->Skip(stream, 8);
		break; /* (8) data */

	case NBT_I8S:
		if ((res = Stream_ReadU32_BE(stream, &tag.dataSize))) break;

		if (NbtTag_IsSmall(&tag)) {
			res = Stream_Read(stream, tag.value.small, tag.dataSize);
		} else {
			tag.value.big = (cc_uint8*)Mem_TryAlloc(tag.dataSize, 1);
			if (!tag.value.big) return ERR_OUT_OF_MEMORY;

			res = Stream_Read(stream, tag.value.big, tag.dataSize);
			if (res) Mem_Free(tag.value.big);
		}
		break;
	case NBT_STR:
		String_InitArray(tag.value.str.text, tag.value.str.buffer);
		res = Nbt_ReadString(stream, &tag.value.str.text);
		break;

	case NBT_LIST:
		if ((res = Stream_Read(stream, tmp, 5))) break;
		childType = tmp[0];
		count = Stream_GetU32_BE(&tmp[1]);

		for (i = 0; i < count; i++) {
			res = Nbt_ReadTag(childType, false, stream, &tag, callback);
			if (res) break;
		}
		break;

	case NBT_DICT:
		for (;;) {
			if ((res = stream->ReadU8(stream, &childType))) break;
			if (childType == NBT_END) break;

			res = Nbt_ReadTag(childType, true, stream, &tag, callback);
			if (res) break;
		}
		break;

	default: return NBT_ERR_UNKNOWN;
	}

	if (res) return res;
	tag.result = 0;
	callback(&tag);
	/* NOTE: callback must set DataBig to NULL, if doesn't want it to be freed */
	if (!NbtTag_IsSmall(&tag)) Mem_Free(tag.value.big);
	return tag.result;
}
#define IsTag(tag, tagName) (String_CaselessEqualsConst(&tag->name, tagName))

/*########################################################################################################################*
*--------------------------------------------------ClassicWorld format----------------------------------------------------*
*#########################################################################################################################*/
/* ClassicWorld is a NBT tag based map format. Tags not listed below are discarded.
COMPOUND "ClassicWorld" {
	U8* "UUID"
	U16 "X", "Y", "Z"
	COMPOUND "Spawn" {
		I16 "X", "Y", "Z"
		U8  "H", "P"
	}
	U8* "BlockArray"  (lower 8 bits, required)
	U8* "BlockArray2" (upper 8 bits, optional)
	COMPOUND "Metadata" {
		COMPOUND "CPE" {
			COMPOUND "ClickDistance"  { U16 "Reach" }
			COMPOUND "EnvWeatherType" { U8 "WeatherType" }
			COMPOUND "EnvMapAppearance" {
				U8 "SideBlock", "EdgeBlock"
				I16 "SidesLevel"
				STR "TextureURL"
			}
			COMPOUND "EnvColors" {
				COMPOUND "Sky"      { U16 "R", "G", "B" }
				COMPOUND "Cloud"    { U16 "R", "G", "B" }
				COMPOUND "Fog"      { U16 "R", "G", "B" }
				COMPOUND "Sunlight" { U16 "R", "G", "B" }
				COMPOUND "Ambient"  { U16 "R", "G", "B" }
			}
			COMPOUND "BlockDefinitions" {
				COMPOUND "Block_XYZ" { (name must start with 'Block')
					U8  "ID", U16 "ID2"
					STR "Name"
					F32 "Speed"
					U8  "CollideType", "BlockDraw"					
					U8  "TransmitsLight", "FullBright"
					U8  "Shape"	, "WalkSound"	
					U8* "Textures", "Fog", "Coords"
				}
			}
		}
	}
}*/
static BlockRaw* Cw_GetBlocks(struct NbtTag* tag) {
	BlockRaw* ptr;
	if (NbtTag_IsSmall(tag)) {
		ptr = (BlockRaw*)Mem_Alloc(tag->dataSize, 1, ".cw map blocks");
		Mem_Copy(ptr, tag->value.small, tag->dataSize);
	} else {
		ptr = tag->value.big;
		tag->value.big = NULL; /* So Nbt_ReadTag doesn't call Mem_Free on World.Blocks */
	}
	return ptr;
}

static void Cw_Callback_1(struct NbtTag* tag) {
	if (IsTag(tag, "X")) { World.Width  = NbtTag_U16(tag); return; }
	if (IsTag(tag, "Y")) { World.Height = NbtTag_U16(tag); return; }
	if (IsTag(tag, "Z")) { World.Length = NbtTag_U16(tag); return; }

	if (IsTag(tag, "UUID")) {
		if (tag->dataSize != WORLD_UUID_LEN) {
			tag->result = CW_ERR_UUID_LEN;
		} else {
			Mem_Copy(World.Uuid, tag->value.small, WORLD_UUID_LEN);
		}
		return;
	}

	if (IsTag(tag, "BlockArray")) {
		World.Volume = tag->dataSize;
		World.Blocks = Cw_GetBlocks(tag);
	}
#ifdef EXTENDED_BLOCKS
	if (IsTag(tag, "BlockArray2")) World_SetMapUpper(Cw_GetBlocks(tag));
#endif
}

static void Cw_Callback_2(struct NbtTag* tag) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!IsTag(tag->parent, "Spawn")) return;
	
	if (IsTag(tag, "X")) { p->Spawn.X = NbtTag_I16(tag); return; }
	if (IsTag(tag, "Y")) { p->Spawn.Y = NbtTag_I16(tag); return; }
	if (IsTag(tag, "Z")) { p->Spawn.Z = NbtTag_I16(tag); return; }
	if (IsTag(tag, "H")) { p->SpawnYaw   = Math_Packed2Deg(NbtTag_U8(tag)); return; }
	if (IsTag(tag, "P")) { p->SpawnPitch = Math_Packed2Deg(NbtTag_U8(tag)); return; }
}

static BlockID cw_curID;
static int cw_colR, cw_colG, cw_colB;
static PackedCol Cw_ParseColor(PackedCol defValue) {
	int r = cw_colR, g = cw_colG, b = cw_colB;
	if (r > 255 || g > 255 || b > 255) return defValue;
	return PackedCol_Make(r, g, b, 255);
}

static void Cw_Callback_4(struct NbtTag* tag) {
	BlockID id = cw_curID;
	struct LocalPlayer* p = &LocalPlayer_Instance;

	if (!IsTag(tag->parent->parent, "CPE")) return;
	if (!IsTag(tag->parent->parent->parent, "Metadata")) return;

	if (IsTag(tag->parent, "ClickDistance")) {
		if (IsTag(tag, "Distance")) { p->ReachDistance = NbtTag_U16(tag) / 32.0f; return; }
	}
	if (IsTag(tag->parent, "EnvWeatherType")) {
		if (IsTag(tag, "WeatherType")) { Env.Weather = NbtTag_U8(tag); return; }
	}

	if (IsTag(tag->parent, "EnvMapAppearance")) {
		if (IsTag(tag, "SideBlock")) { Env.SidesBlock = NbtTag_U8(tag);  return; }
		if (IsTag(tag, "EdgeBlock")) { Env.EdgeBlock  = NbtTag_U8(tag);  return; }
		if (IsTag(tag, "SideLevel")) { Env.EdgeHeight = NbtTag_I16(tag); return; }

		if (IsTag(tag, "TextureURL")) {
			cc_string url = NbtTag_String(tag);
			if (url.length) Server_RetrieveTexturePack(&url);
			return;
		}
	}

	/* Callback for compound tag is called after all its children have been processed */
	if (IsTag(tag->parent, "EnvColors")) {
		if (IsTag(tag, "Sky")) {
			Env.SkyCol    = Cw_ParseColor(ENV_DEFAULT_SKY_COLOR); return;
		} else if (IsTag(tag, "Cloud")) {
			Env.CloudsCol = Cw_ParseColor(ENV_DEFAULT_CLOUDS_COLOR); return;
		} else if (IsTag(tag, "Fog")) {
			Env.FogCol    = Cw_ParseColor(ENV_DEFAULT_FOG_COLOR); return;
		} else if (IsTag(tag, "Sunlight")) {
			Env_SetSunCol(Cw_ParseColor(ENV_DEFAULT_SUN_COLOR)); return;
		} else if (IsTag(tag, "Ambient")) {
			Env_SetShadowCol(Cw_ParseColor(ENV_DEFAULT_SHADOW_COLOR)); return;
		}
	}

	if (IsTag(tag->parent, "BlockDefinitions") && Game_AllowCustomBlocks) {
		static const cc_string blockStr = String_FromConst("Block");
		if (!String_CaselessStarts(&tag->name, &blockStr)) return;	

		/* hack for sprite draw (can't rely on order of tags when reading) */
		if (Blocks.SpriteOffset[id] == 0) {
			Blocks.SpriteOffset[id] = Blocks.Draw[id];
			Blocks.Draw[id] = DRAW_SPRITE;
		} else {
			Blocks.SpriteOffset[id] = 0;
		}

		Block_DefineCustom(id);
		Blocks.CanPlace[id]  = true;
		Blocks.CanDelete[id] = true;
		Event_RaiseVoid(&BlockEvents.PermissionsChanged);

		cw_curID = 0;
	}
}

static void Cw_Callback_5(struct NbtTag* tag) {
	BlockID id = cw_curID;
	cc_uint8* arr;
	cc_uint8 sound;

	if (!IsTag(tag->parent->parent->parent, "CPE")) return;
	if (!IsTag(tag->parent->parent->parent->parent, "Metadata")) return;

	if (IsTag(tag->parent->parent, "EnvColors")) {
		if (IsTag(tag, "R")) { cw_colR = NbtTag_U16(tag); return; }
		if (IsTag(tag, "G")) { cw_colG = NbtTag_U16(tag); return; }
		if (IsTag(tag, "B")) { cw_colB = NbtTag_U16(tag); return; }
	}

	if (IsTag(tag->parent->parent, "BlockDefinitions") && Game_AllowCustomBlocks) {
		if (IsTag(tag, "ID"))             { cw_curID = NbtTag_U8(tag);  return; }
		if (IsTag(tag, "ID2"))            { cw_curID = NbtTag_U16(tag); return; }
		if (IsTag(tag, "CollideType"))    { Block_SetCollide(id, NbtTag_U8(tag)); return; }
		if (IsTag(tag, "Speed"))          { Blocks.SpeedMultiplier[id] = NbtTag_F32(tag); return; }
		if (IsTag(tag, "TransmitsLight")) { Blocks.BlocksLight[id] = NbtTag_U8(tag) == 0; return; }
		if (IsTag(tag, "FullBright"))     { Blocks.FullBright[id] = NbtTag_U8(tag) != 0; return; }
		if (IsTag(tag, "BlockDraw"))      { Blocks.Draw[id] = NbtTag_U8(tag); return; }
		if (IsTag(tag, "Shape"))          { Blocks.SpriteOffset[id] = NbtTag_U8(tag); return; }

		if (IsTag(tag, "Name")) {
			cc_string name = NbtTag_String(tag);
			Block_SetName(id, &name);
			return;
		}

		if (IsTag(tag, "Textures")) {
			arr = NbtTag_U8_Array(tag, 6);
			Block_Tex(id, FACE_YMAX) = arr[0]; Block_Tex(id, FACE_YMIN) = arr[1];
			Block_Tex(id, FACE_XMIN) = arr[2]; Block_Tex(id, FACE_XMAX) = arr[3];
			Block_Tex(id, FACE_ZMIN) = arr[4]; Block_Tex(id, FACE_ZMAX) = arr[5];

			/* hacky way of storing upper 8 bits */
			if (tag->dataSize >= 12) {
				Block_Tex(id, FACE_YMAX) |= arr[6]  << 8; Block_Tex(id, FACE_YMIN) |= arr[7]  << 8;
				Block_Tex(id, FACE_XMIN) |= arr[8]  << 8; Block_Tex(id, FACE_XMAX) |= arr[9]  << 8;
				Block_Tex(id, FACE_ZMIN) |= arr[10] << 8; Block_Tex(id, FACE_ZMAX) |= arr[11] << 8;
			}
			return;
		}
		
		if (IsTag(tag, "WalkSound")) {
			sound = NbtTag_U8(tag);
			Blocks.DigSounds[id]  = sound;
			Blocks.StepSounds[id] = sound;
			if (sound == SOUND_GLASS) Blocks.StepSounds[id] = SOUND_STONE;
			return;
		}

		if (IsTag(tag, "Fog")) {
			arr = NbtTag_U8_Array(tag, 4);
			Blocks.FogDensity[id] = (arr[0] + 1) / 128.0f;
			/* Fix for older ClassicalSharp versions which saved wrong fog density value */
			if (arr[0] == 0xFF) Blocks.FogDensity[id] = 0.0f;
			Blocks.FogCol[id] = PackedCol_Make(arr[1], arr[2], arr[3], 255);
			return;
		}

		if (IsTag(tag, "Coords")) {
			arr = NbtTag_U8_Array(tag, 6);
			Blocks.MinBB[id].X = (cc_int8)arr[0] / 16.0f; Blocks.MaxBB[id].X = (cc_int8)arr[3] / 16.0f;
			Blocks.MinBB[id].Y = (cc_int8)arr[1] / 16.0f; Blocks.MaxBB[id].Y = (cc_int8)arr[4] / 16.0f;
			Blocks.MinBB[id].Z = (cc_int8)arr[2] / 16.0f; Blocks.MaxBB[id].Z = (cc_int8)arr[5] / 16.0f;
			return;
		}
	}
}

static void Cw_Callback(struct NbtTag* tag) {
	struct NbtTag* tmp = tag->parent;
	int depth = 0;
	while (tmp) { depth++; tmp = tmp->parent; }

	switch (depth) {
	case 1: Cw_Callback_1(tag); return;
	case 2: Cw_Callback_2(tag); return;
	case 4: Cw_Callback_4(tag); return;
	case 5: Cw_Callback_5(tag); return;
	}
	/* ClassicWorld -> Metadata -> CPE -> ExtName -> [values]
	        0             1         2        3          4   */
}

cc_result Cw_Load(struct Stream* stream) {
	struct Stream compStream;
	struct InflateState state;
	cc_result res;
	cc_uint8 tag;

	Inflate_MakeStream2(&compStream, &state, stream);
	if ((res = Map_SkipGZipHeader(stream))) return res;
	if ((res = compStream.ReadU8(&compStream, &tag))) return res;

	if (tag != NBT_DICT) return CW_ERR_ROOT_TAG;
	return Nbt_ReadTag(NBT_DICT, true, &compStream, NULL, Cw_Callback);
}


/*########################################################################################################################*
*-----------------------------------------------Java serialisation format-------------------------------------------------*
*#########################################################################################################################*/
/* Rather than bothering following this, I skip a lot of the java serialisation format
     Stream              BlockData        BlockDataTiny      BlockDataLong
|--------------|     |---------------|  |---------------|  |---------------| 
| U16 Magic    |     |>BlockDataTiny |  | TC_BLOCKDATA  |  | TC_BLOCKLONG  |
| U16 Version  |     |>BlockDataLong |  | U8 Size       |  | U32 Size      |
| Content[var] |     |_______________|  | U8 Data[size] |  | U8 Data[size] |
|______________|                        |_______________|  |_______________|

    Content
|--------------| |--------------| 
| >BlockData   | | >NewString   |
| >Object      | | >TC_RESET    |
|______________| | >TC_NULL     |
| >PrevObject     |
| >NewClass     |
| >NewEnum     |

}*/
enum JTypeCode {
	TC_NULL = 0x70, TC_REFERENCE = 0x71, TC_CLASSDESC = 0x72, TC_OBJECT = 0x73, 
	TC_STRING = 0x74, TC_ARRAY = 0x75, TC_BLOCKDATA = 0x77, TC_ENDBLOCKDATA = 0x78
};
enum JFieldType {
	JFIELD_I8 = 'B', JFIELD_F64 = 'D', JFIELD_F32 = 'F', JFIELD_I32 = 'I', JFIELD_I64 = 'J',
	JFIELD_BOOL = 'Z', JFIELD_ARRAY = '[', JFIELD_OBJECT = 'L'
};

#define JNAME_SIZE 48
#define SC_WRITE_METHOD 0x01
#define SC_SERIALIZABLE 0x02

static cc_uint32 reference_id;
#define Java_AddReference() reference_id++;

union JValue {
	cc_uint8  U8;
	cc_int32  I32;
	cc_uint32 U32;
	float     F32;
	struct { cc_uint8* Ptr; cc_uint32 Size; } Array;
};

struct JFieldDesc {
	cc_uint8 Type;
	cc_uint8 FieldName[JNAME_SIZE];
	union JValue Value; 
	/* "Value" field here is not accurate to how java deserialising actually works, */
	/*  but easier to store here since only care about Level class values anyways */
};

struct JClassDesc;
struct JClassDesc {
	cc_uint8 ClassName[JNAME_SIZE];
	cc_uint8 Flags;
	int FieldsCount;
	struct JFieldDesc Fields[38];
	cc_uint32 Reference;
	struct JClassDesc* SuperClass;
	struct JClassDesc* tmp;
};

struct JArray {
	struct JClassDesc* Desc;
	cc_uint8* Data; /* for byte arrays */
	cc_uint32 Size; /* for byte arrays */
};

struct JUnion {
	cc_uint8 Type;
	union {
		cc_uint8 String[JNAME_SIZE]; /* TC_STRING */
		struct JClassDesc* Object;   /* TC_OBJECT */
		struct JArray      Array;    /* TC_ARRAY */
	} Value;
};

static cc_result Java_ReadString(struct Stream* stream, cc_uint8* buffer) {
	int len;
	cc_result res;

	if ((res = Stream_Read(stream, buffer, 2))) return res;
	len = Stream_GetU16_BE(buffer);

	Mem_Set(buffer, 0, JNAME_SIZE);
	if (len > JNAME_SIZE) return JAVA_ERR_JSTRING_LEN;
	return Stream_Read(stream, buffer, len);
}


static cc_result Java_ReadObject(struct Stream* stream,     struct JUnion* object);
static cc_result Java_ReadObjectData(struct Stream* stream, struct JUnion* object);
static cc_result Java_SkipAnnotation(struct Stream* stream) {
	cc_uint8 typeCode, count;
	struct JUnion object;
	cc_result res;

	for (;;)
	{
		if ((res = stream->ReadU8(stream, &typeCode))) return res;

		switch (typeCode)
		{
		case TC_BLOCKDATA:
			if ((res = stream->ReadU8(stream, &count))) return res;
			if ((res = stream->Skip(stream, count)))    return res;
			break;
		case TC_ENDBLOCKDATA: 
			return 0;
		default:
			object.Type = typeCode;
			if ((res = Java_ReadObjectData(stream, &object))) return res;
			break;
		}
	}
	return 0;
}


/* Most .dat maps only use at most 16 different class types */
/*  However some survival test maps can use up to 30 */
#define CLASS_CAPACITY 30
static struct JClassDesc* class_cache;
static int class_count;
static cc_result Java_ReadClassDesc(struct Stream* stream, struct JClassDesc** desc);

static cc_result Java_ReadFieldDesc(struct Stream* stream, struct JFieldDesc* desc) {
	struct JUnion className;
	cc_result res;

	if ((res = stream->ReadU8(stream, &desc->Type)))      return res;
	if ((res = Java_ReadString(stream, desc->FieldName))) return res;

	if (desc->Type == JFIELD_ARRAY || desc->Type == JFIELD_OBJECT) {		
		return Java_ReadObject(stream, &className);
	}
	return 0;
}

static cc_result Java_ReadNewClassDesc(struct Stream* stream, struct JClassDesc* desc) {
	cc_uint8 count[2];
	cc_result res;
	int i;

	if ((res = Java_ReadString(stream, desc->ClassName))) return res;
	if ((res = stream->Skip(stream, 8)))                  return res; /* serial version UID */
	if ((res = stream->ReadU8(stream, &desc->Flags)))     return res;

	desc->Reference = reference_id;
	Java_AddReference();

	if ((res = Stream_Read(stream, count, 2))) return res;
	desc->FieldsCount = Stream_GetU16_BE(count);
	if (desc->FieldsCount > Array_Elems(desc->Fields)) return JAVA_ERR_JCLASS_FIELDS;
	
	for (i = 0; i < desc->FieldsCount; i++) {
		if ((res = Java_ReadFieldDesc(stream, &desc->Fields[i]))) return res;
	}

	if ((res = Java_SkipAnnotation(stream))) return res;
	return Java_ReadClassDesc(stream, &desc->SuperClass);
}

static cc_result Java_ReadClassDesc(struct Stream* stream, struct JClassDesc** desc) {
	cc_uint8 typeCode;
	cc_uint32 reference;
	cc_result res;
	int i;
	if ((res = stream->ReadU8(stream, &typeCode))) return res;

	switch (typeCode)
	{
		case TC_NULL:
			*desc = NULL;
			return 0;

		case TC_REFERENCE:
			if ((res = Stream_ReadU32_BE(stream, &reference))) return res;

			/* Use a previously defined ClassDescriptor */
			for (i = 0; i < class_count; i++) 
			{
				if (class_cache[i].Reference != reference) continue;
				
				*desc = &class_cache[i];
				return 0;
			}
			return JAVA_ERR_JCLASS_REFERENCE;

		case TC_CLASSDESC:
			if (class_count >= CLASS_CAPACITY) return JAVA_ERR_JCLASSES_COUNT;

			*desc = &class_cache[class_count++];
			return Java_ReadNewClassDesc(stream, *desc);
	}
	return JAVA_ERR_JCLASS_TYPE;
}


static cc_result Java_ReadValue(struct Stream* stream, cc_uint8 type, union JValue* value) {
	struct JUnion obj;
	cc_result res;

	switch (type) {
	case JFIELD_I8:
	case JFIELD_BOOL:
		return stream->ReadU8(stream, &value->U8);
	case JFIELD_F32:
	case JFIELD_I32:
		return Stream_ReadU32_BE(stream, &value->U32);
	case JFIELD_F64:
	case JFIELD_I64:
		return stream->Skip(stream, 8); /* (8) data */
	case JFIELD_OBJECT:
		return Java_ReadObject(stream, &obj);

	case JFIELD_ARRAY:
		if ((res = Java_ReadObject(stream, &obj))) return res;
		value->Array.Size = 0;
		value->Array.Ptr  = NULL;

		/* Array is a byte array */
		/* NOTE: This can technically leak memory if this array is discarded, however */
		/*  so far the only observed byte array in .dat files is the map blocks anyways */
		if (obj.Type == TC_ARRAY && obj.Value.Array.Desc->ClassName[1] == JFIELD_I8) {
			value->Array.Size = obj.Value.Array.Size;
			value->Array.Ptr  = obj.Value.Array.Data;
		}
		return 0;
	}
	return JAVA_ERR_JVALUE_TYPE;
}

static cc_result Java_ReadClassData(struct Stream* stream, struct JClassDesc* desc) {
	struct JFieldDesc* field;
	cc_result res;
	int i;

	if (!(desc->Flags & SC_SERIALIZABLE))
		return JAVA_ERR_JOBJECT_FLAGS;

	for (i = 0; i < desc->FieldsCount; i++) 
	{
		field = &desc->Fields[i];
		if ((res = Java_ReadValue(stream, field->Type, &field->Value))) return res;
	}

	if (desc->Flags & SC_WRITE_METHOD)
		return Java_SkipAnnotation(stream);
	return 0;
}

static cc_result Java_ReadNewString(struct Stream* stream, struct JUnion* object) {
	Java_AddReference();
	return Java_ReadString(stream, object->Value.String);
}

static cc_result Java_ReadNewObject(struct Stream* stream, struct JUnion* object) {
	struct JClassDesc* head;
	cc_result res;
	if ((res = Java_ReadClassDesc(stream, &object->Value.Object))) return res;
	Java_AddReference();

	/* Linked list of classes, with most superclass fist as head */
	head = object->Value.Object; head->tmp = NULL;
	while (head->SuperClass) {
		head->SuperClass->tmp = head;
		head = head->SuperClass;
	}

	/* Class data is read with most superclass first */
	while (head) {
		if ((res = Java_ReadClassData(stream, head))) return res;
		head = head->tmp;
	}
	return 0;
}

static cc_result Java_ReadNewArray(struct Stream* stream, struct JUnion* object) {
	struct JArray* array = &object->Value.Array;
	union JValue value;
	cc_uint32 count;
	cc_uint8 type;
	cc_result res;
	int i;

	if ((res = Java_ReadClassDesc(stream, &array->Desc))) return res;
	if ((res = Stream_ReadU32_BE(stream, &count)))        return res;
	type = array->Desc->ClassName[1];
	Java_AddReference();

	if (type != JFIELD_I8) {
		/* Not a byte array, so just discard the unnecessary values */
		for (i = 0; i < count; i++)
		{
			if ((res = Java_ReadValue(stream, type, &value))) return res;
		}
		return 0;
	}

	array->Size = count;
	array->Data = (cc_uint8*)Mem_TryAlloc(count, 1);

	if (!array->Data) return ERR_OUT_OF_MEMORY;
	res = Stream_Read(stream, array->Data, count);
	if (res) { Mem_Free(array->Data); }
	return res;
}

static cc_result Java_ReadObjectData(struct Stream* stream, struct JUnion* object) {
	cc_uint32 reference;
	switch (object->Type) 
	{
		case TC_STRING:    return Java_ReadNewString(stream, object);
		case TC_NULL:      return 0;
		case TC_REFERENCE: return Stream_ReadU32_BE(stream, &reference);
		case TC_OBJECT:    return Java_ReadNewObject(stream, object);
		case TC_ARRAY:     return Java_ReadNewArray(stream,  object);
	}
	return JAVA_ERR_INVALID_TYPECODE;
}

static cc_result Java_ReadObject(struct Stream* stream, struct JUnion* object) {
	cc_result res;
	if ((res = stream->ReadU8(stream, &object->Type))) return res;
	return Java_ReadObjectData(stream, object);
}

static int Java_I32(struct JFieldDesc* field) {
	if (field->Type != JFIELD_I32) Logger_Abort("Field type must be Int32");
	return field->Value.I32;
}


/*########################################################################################################################*
*-------------------------------------------------Minecraft .dat format---------------------------------------------------*
*#########################################################################################################################*/
/* Minecraft Classic used 3 different GZIP compressed binary map formats throughout its various versions
Preclassic - Classic 0.12:
    U8* "Blocks"     (256x64x256 array)
Classic 0.13:
	U32 "Identifier" (must be 0x271BB788)
	U8  "Version"    (must be 1)
	STR "Name"       (ignored)
	STR "Author"     (ignored)
	U64 "Creation"   (ignored)
	U16 "Width"
	U16 "Length"
	U16 "Height"
	U8* "Blocks"
Classic 0.15 to Classic 0.30:
	U32 "Identifier" (must be 0x271BB788)
	U8  "Version"    (must be 2)
	VAR "Level"      (Java serialised level object instance)
}*/

static void UseClassic013Env(void) {
	/* Similiar env to how it appears in 0.13 classic client */
	Env.CloudsHeight = -30000;
	Env.SkyCol       = PackedCol_Make(0x7F, 0xCC, 0xFF, 0xFF);
	Env.FogCol       = PackedCol_Make(0x7F, 0xCC, 0xFF, 0xFF);
}

static cc_result Dat_LoadFormat0(struct Stream* stream) {
	UseClassic013Env();
	/* Similiar env to how it appears in preclassic client */
	Env.EdgeBlock  = BLOCK_AIR;
	Env.SidesBlock = BLOCK_AIR;

	/* Map 'format' is just the 256x64x256 blocks of the level */
	World.Width  = 256;
	World.Height =  64;
	World.Length = 256;

	#define PC_VOLUME (256 * 64 * 256)
	World.Volume = PC_VOLUME;
	World.Blocks = (BlockRaw*)Mem_TryAlloc(PC_VOLUME, 1);
	if (!World.Blocks) return ERR_OUT_OF_MEMORY;

	/* First 5 bytes already read earlier as .dat header */
	Mem_Set(World.Blocks, BLOCK_STONE, 5);
	return Stream_Read(stream, World.Blocks + 5, PC_VOLUME - 5);
}

static cc_result Dat_LoadFormat1(struct Stream* stream) {
	cc_uint8 level_name[JNAME_SIZE];
	cc_uint8 level_author[JNAME_SIZE];
	cc_uint8 header[8 + 2 + 2 + 2];
	cc_result res;

	UseClassic013Env();
	if ((res = Java_ReadString(stream,   level_name))) return res;
	if ((res = Java_ReadString(stream, level_author))) return res;
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;
	
	/* bytes 0-8 = created timestamp (currentTimeMillis) */
	World.Width  = Stream_GetU16_BE(header +  8);
	World.Length = Stream_GetU16_BE(header + 10);
	World.Height = Stream_GetU16_BE(header + 12);
	return Map_ReadBlocks(stream);
}

static cc_result Dat_LoadFormat2(struct Stream* stream) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct JClassDesc classes[CLASS_CAPACITY];
	cc_uint8 header[2 + 2];
	struct JUnion obj;
	struct JClassDesc* desc;
	struct JFieldDesc* field;
	cc_string fieldName;
	cc_result res;
	int i;
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;

	/* Reset state for Java Serialisation */
	class_cache  = classes;
	class_count  = 0;
	reference_id = 0x7E0000;

	/* Java seralisation headers */
	if (Stream_GetU16_BE(header + 0) != 0xACED) return DAT_ERR_JIDENTIFIER;
	if (Stream_GetU16_BE(header + 2) != 0x0005) return DAT_ERR_JVERSION;

	if ((res = Java_ReadObject(stream, &obj)))  return res;
	if (obj.Type != TC_OBJECT)                  return DAT_ERR_ROOT_OBJECT;
	desc = obj.Value.Object;

	for (i = 0; i < desc->FieldsCount; i++) 
	{
		field     = &desc->Fields[i];
		fieldName = String_FromRaw((char*)field->FieldName, JNAME_SIZE);

		if (String_CaselessEqualsConst(&fieldName, "width")) {
			World.Width  = Java_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "height")) {
			World.Length = Java_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "depth")) {
			World.Height = Java_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "blocks")) {
			if (field->Type != JFIELD_ARRAY) Logger_Abort("Blocks field must be Array");
			World.Blocks = field->Value.Array.Ptr;
			World.Volume = field->Value.Array.Size;
		} else if (String_CaselessEqualsConst(&fieldName, "xSpawn")) {
			p->Spawn.X = (float)Java_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "ySpawn")) {
			p->Spawn.Y = (float)Java_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "zSpawn")) {
			p->Spawn.Z = (float)Java_I32(field);
		}
	}
	return 0;
}

cc_result Dat_Load(struct Stream* stream) {
	cc_uint8 header[4 + 1];
	cc_uint32 signature;
	cc_result res;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream2(&compStream, &state, stream);
	if ((res = Map_SkipGZipHeader(stream)))                       return res;
	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;

	signature = Stream_GetU32_BE(header + 0);
	switch (signature)
	{
		/* Classic map format signature */
	case 0x271BB788: break;
		/* Not an actual signature, but 99% of preclassic */
		/*  to classic 0.12 maps start with these 4 bytes */
	case 0x01010101: return Dat_LoadFormat0(&compStream);
		/* Bogus .dat file */
	default:         return DAT_ERR_IDENTIFIER;
	}

	/* Format version */
	switch (header[4])
	{
		/* Format version 1 = classic 0.13 */
	case 0x01: return Dat_LoadFormat1(&compStream);
		/* Format version 2 = classic 0.15 to 0.30 */
	case 0x02: return Dat_LoadFormat2(&compStream);
		/* Bogus .dat file */
	default:   return DAT_ERR_VERSION;
	}
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld export----------------------------------------------------*
*#########################################################################################################################*/
#define CW_META_RGB NBT_I16,0,1,'R',0,0,  NBT_I16,0,1,'G',0,0,  NBT_I16,0,1,'B',0,0,

static int Cw_WriteEndString(cc_uint8* data, const cc_string* text) {
	cc_uint8* cur = data + 2;
	int i, wrote, len = 0;

	for (i = 0; i < text->length; i++) {
		wrote = Convert_CP437ToUtf8(text->buffer[i], cur);
		len += wrote; cur += wrote;
	}

	Stream_SetU16_BE(data, len);
	*cur = NBT_END;
	return len + 1;
}

static cc_uint8 cw_begin[131] = {
NBT_DICT, 0,12, 'C','l','a','s','s','i','c','W','o','r','l','d',
	NBT_I8,   0,13, 'F','o','r','m','a','t','V','e','r','s','i','o','n', 1,
	NBT_I8S,  0,4,  'U','U','I','D', 0,0,0,16, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	NBT_I16,  0,1,  'X', 0,0,
	NBT_I16,  0,1,  'Y', 0,0,
	NBT_I16,  0,1,  'Z', 0,0,
	NBT_DICT, 0,5,  'S','p','a','w','n',
		NBT_I16,  0,1, 'X', 0,0,
		NBT_I16,  0,1, 'Y', 0,0,
		NBT_I16,  0,1, 'Z', 0,0,
		NBT_I8,   0,1, 'H', 0,
		NBT_I8,   0,1, 'P', 0,
	NBT_END,
	NBT_I8S,  0,10, 'B','l','o','c','k','A','r','r','a','y', 0,0,0,0,
};
static cc_uint8 cw_map2[18] = {
	NBT_I8S,  0,11, 'B','l','o','c','k','A','r','r','a','y','2', 0,0,0,0,
};
static cc_uint8 cw_meta_cpe[303] = {
	NBT_DICT, 0,8,  'M','e','t','a','d','a','t','a',
		NBT_DICT, 0,3, 'C','P','E',
			NBT_DICT, 0,13, 'C','l','i','c','k','D','i','s','t','a','n','c','e',
				NBT_I16,  0,8,  'D','i','s','t','a','n','c','e', 0,0,
			NBT_END,
			NBT_DICT, 0,14, 'E','n','v','W','e','a','t','h','e','r','T','y','p','e',
				NBT_I8,   0,11, 'W','e','a','t','h','e','r','T','y','p','e', 0,
			NBT_END,
			NBT_DICT, 0,9,  'E','n','v','C','o','l','o','r','s',
				NBT_DICT, 0,3, 'S','k','y',                     CW_META_RGB
				NBT_END,
				NBT_DICT, 0,5, 'C','l','o','u','d',             CW_META_RGB
				NBT_END,
				NBT_DICT, 0,3, 'F','o','g',                     CW_META_RGB
				NBT_END,
				NBT_DICT, 0,7, 'A','m','b','i','e','n','t',     CW_META_RGB
				NBT_END,
				NBT_DICT, 0,8, 'S','u','n','l','i','g','h','t', CW_META_RGB
				NBT_END,
			NBT_END,
			NBT_DICT, 0,16, 'E','n','v','M','a','p','A','p','p','e','a','r','a','n','c','e',
				NBT_I8,   0,9,  'S','i','d','e','B','l','o','c','k',     0,
				NBT_I8,   0,9,  'E','d','g','e','B','l','o','c','k',     0,
				NBT_I16,  0,9,  'S','i','d','e','L','e','v','e','l',     0,0,
				NBT_STR,  0,10, 'T','e','x','t','u','r','e','U','R','L', 0,0,
};
static cc_uint8 cw_meta_defs[19] = {
			NBT_DICT, 0,16, 'B','l','o','c','k','D','e','f','i','n','i','t','i','o','n','s',
};
static cc_uint8 cw_meta_def[189] = {
				NBT_DICT, 0,9,  'B','l','o','c','k','\0','\0','\0','\0',
					NBT_I8,  0,2,  'I','D',                              0,
					/* It would be have been better to just change ID to be a I16 */
					/* Unfortunately this isn't backwards compatible with ClassicalSharp */
					NBT_I16, 0,3,  'I','D','2',                          0,0,
					NBT_I8,  0,11, 'C','o','l','l','i','d','e','T','y','p','e', 0,
					NBT_F32, 0,5,  'S','p','e','e','d',                  0,0,0,0,
					/* Ugly hack for supporting texture IDs over 255 */
					/* First 6 elements are lower 8 bits, next 6 are upper 8 bits */
					NBT_I8S, 0,8,  'T','e','x','t','u','r','e','s',      0,0,0,12, 0,0,0,0,0,0, 0,0,0,0,0,0,
					NBT_I8,  0,14, 'T','r','a','n','s','m','i','t','s','L','i','g','h','t', 0,
					NBT_I8,  0,9,  'W','a','l','k','S','o','u','n','d',  0,
					NBT_I8,  0,10, 'F','u','l','l','B','r','i','g','h','t', 0,
					NBT_I8,  0,5,  'S','h','a','p','e',                  0,
					NBT_I8,  0,9,  'B','l','o','c','k','D','r','a','w',  0,
					NBT_I8S, 0,3,  'F','o','g',                          0,0,0,4, 0,0,0,0,
					NBT_I8S, 0,6,  'C','o','o','r','d','s',              0,0,0,6, 0,0,0,0,0,0,
					NBT_STR, 0,4,  'N','a','m','e',                      0,0,
};
static cc_uint8 cw_end[4] = {
			NBT_END,
		NBT_END,
	NBT_END,
NBT_END,
};


static cc_result Cw_WriteBockDef(struct Stream* stream, int b) {
	cc_uint8 tmp[512];
	cc_string name;
	int len;

	cc_bool sprite = Blocks.Draw[b] == DRAW_SPRITE;
	union IntAndFloat speed;
	TextureLoc tex;
	cc_uint8 fog;
	PackedCol col;
	Vec3 minBB, maxBB;	

	Mem_Copy(tmp, cw_meta_def, sizeof(cw_meta_def));
	{
		/* Hacky unique tag name for each by using hex of block */
		name = String_Init((char*)&tmp[8], 0, 4);
		String_AppendHex(&name, b >> 8);
		String_AppendHex(&name, b);

		tmp[17] = b;
		Stream_SetU16_BE(&tmp[24], b);

		tmp[40] = Blocks.Collide[b];
		speed.f = Blocks.SpeedMultiplier[b];
		Stream_SetU32_BE(&tmp[49], speed.u);

		/* Originally only up to 256 textures were supported, which used up 6 bytes total */
		/* Later, support for more textures was added, which requires 2 bytes per texture */
		/*  For backwards compatibility, the lower byte of each texture is */
		/*  written into first 6 bytes, then higher byte into next 6 bytes */
		tex = Block_Tex(b, FACE_YMAX); tmp[68] = (cc_uint8)tex; tmp[74] = (cc_uint8)(tex >> 8);
		tex = Block_Tex(b, FACE_YMIN); tmp[69] = (cc_uint8)tex; tmp[75] = (cc_uint8)(tex >> 8);
		tex = Block_Tex(b, FACE_XMIN); tmp[70] = (cc_uint8)tex; tmp[76] = (cc_uint8)(tex >> 8);
		tex = Block_Tex(b, FACE_XMAX); tmp[71] = (cc_uint8)tex; tmp[77] = (cc_uint8)(tex >> 8);
		tex = Block_Tex(b, FACE_ZMIN); tmp[72] = (cc_uint8)tex; tmp[78] = (cc_uint8)(tex >> 8);
		tex = Block_Tex(b, FACE_ZMAX); tmp[73] = (cc_uint8)tex; tmp[79] = (cc_uint8)(tex >> 8);

		tmp[97]  = Blocks.BlocksLight[b] ? 0 : 1;
		tmp[110] = Blocks.DigSounds[b];
		tmp[124] = Blocks.FullBright[b] ? 1 : 0;
		tmp[133] = sprite ? 0 : (cc_uint8)(Blocks.MaxBB[b].Y * 16);
		tmp[146] = sprite ? Blocks.SpriteOffset[b] : Blocks.Draw[b];

		fog = (cc_uint8)(128 * Blocks.FogDensity[b] - 1);
		col = Blocks.FogCol[b];
		tmp[157] = Blocks.FogDensity[b] ? fog : 0;
		tmp[158] = PackedCol_R(col); tmp[159] = PackedCol_G(col); tmp[160] = PackedCol_B(col);

		minBB = Blocks.MinBB[b]; maxBB = Blocks.MaxBB[b];
		tmp[174] = (cc_uint8)(minBB.X * 16); tmp[175] = (cc_uint8)(minBB.Y * 16); tmp[176] = (cc_uint8)(minBB.Z * 16);
		tmp[177] = (cc_uint8)(maxBB.X * 16); tmp[178] = (cc_uint8)(maxBB.Y * 16); tmp[179] = (cc_uint8)(maxBB.Z * 16);
	}

	name = Block_UNSAFE_GetName(b);
	len  = Cw_WriteEndString(&tmp[187], &name);
	return Stream_Write(stream, tmp, sizeof(cw_meta_def) + len);
}

cc_result Cw_Save(struct Stream* stream) {
	cc_uint8 tmp[768];
	PackedCol col;
	struct LocalPlayer* p = &LocalPlayer_Instance;
	cc_result res;
	int b, len;

	Mem_Copy(tmp, cw_begin, sizeof(cw_begin));
	{
		Mem_Copy(&tmp[43], World.Uuid, WORLD_UUID_LEN);
		Stream_SetU16_BE(&tmp[63], World.Width);
		Stream_SetU16_BE(&tmp[69], World.Height);
		Stream_SetU16_BE(&tmp[75], World.Length);
		Stream_SetU32_BE(&tmp[127], World.Volume);
		
		/* TODO: Maybe keep real spawn too? */
		Stream_SetU16_BE(&tmp[89],  (cc_uint16)p->Base.Position.X);
		Stream_SetU16_BE(&tmp[95],  (cc_uint16)p->Base.Position.Y);
		Stream_SetU16_BE(&tmp[101], (cc_uint16)p->Base.Position.Z);
		tmp[107] = Math_Deg2Packed(p->SpawnYaw);
		tmp[112] = Math_Deg2Packed(p->SpawnPitch);
	}
	if ((res = Stream_Write(stream, tmp,      sizeof(cw_begin)))) return res;
	if ((res = Stream_Write(stream, World.Blocks, World.Volume))) return res;

#ifdef EXTENDED_BLOCKS
	if (World.Blocks != World.Blocks2) {
		Mem_Copy(tmp, cw_map2, sizeof(cw_map2));
		Stream_SetU32_BE(&tmp[14], World.Volume);

		if ((res = Stream_Write(stream, tmp,        sizeof(cw_map2)))) return res;
		if ((res = Stream_Write(stream, World.Blocks2, World.Volume))) return res;
	}
#endif

	Mem_Copy(tmp, cw_meta_cpe, sizeof(cw_meta_cpe));
	{
		Stream_SetU16_BE(&tmp[44], (cc_uint16)(LocalPlayer_Instance.ReachDistance * 32));
		tmp[78] = Env.Weather;

		col = Env.SkyCol;    tmp[103] = PackedCol_R(col); tmp[109] = PackedCol_G(col); tmp[115] = PackedCol_B(col);
		col = Env.CloudsCol; tmp[130] = PackedCol_R(col); tmp[136] = PackedCol_G(col); tmp[142] = PackedCol_B(col);
		col = Env.FogCol;    tmp[155] = PackedCol_R(col); tmp[161] = PackedCol_G(col); tmp[167] = PackedCol_B(col);
		col = Env.ShadowCol; tmp[184] = PackedCol_R(col); tmp[190] = PackedCol_G(col); tmp[196] = PackedCol_B(col);
		col = Env.SunCol;    tmp[214] = PackedCol_R(col); tmp[220] = PackedCol_G(col); tmp[226] = PackedCol_B(col);

		tmp[260] = (BlockRaw)Env.SidesBlock;
		tmp[273] = (BlockRaw)Env.EdgeBlock;
		Stream_SetU16_BE(&tmp[286], Env.EdgeHeight);
	}
	len = Cw_WriteEndString(&tmp[301], &TexturePack_Url);
	if ((res = Stream_Write(stream, tmp, sizeof(cw_meta_cpe) + len))) return res;

	if ((res = Stream_Write(stream, cw_meta_defs, sizeof(cw_meta_defs)))) return res;
	/* Write block definitions in reverse order so that software that only reads byte 'ID' */
	/* still loads correct first 256 block defs when saving a map with over 256 block defs */
	for (b = BLOCK_MAX_DEFINED; b >= 1; b--) {
		if (!Block_IsCustomDefined(b)) continue;
		if ((res = Cw_WriteBockDef(stream, b))) return res;
	}
	return Stream_Write(stream, cw_end, sizeof(cw_end));
}


/*########################################################################################################################*
*---------------------------------------------------Schematic export------------------------------------------------------*
*#########################################################################################################################*/

static cc_uint8 sc_begin[78] = {
NBT_DICT, 0,9, 'S','c','h','e','m','a','t','i','c',
	NBT_STR,  0,9,  'M','a','t','e','r','i','a','l','s', 0,7, 'C','l','a','s','s','i','c',
	NBT_I16,  0,5,  'W','i','d','t','h',                 0,0,
	NBT_I16,  0,6,  'H','e','i','g','h','t',             0,0,
	NBT_I16,  0,6,  'L','e','n','g','t','h',             0,0,
	NBT_I8S,  0,6,  'B','l','o','c','k','s',             0,0,0,0,
};
static cc_uint8 sc_data[11] = {
	NBT_I8S,  0,4,  'D','a','t','a',                     0,0,0,0,
};
static cc_uint8 sc_end[37] = {
	NBT_LIST, 0,8,  'E','n','t','i','t','i','e','s',                 NBT_DICT, 0,0,0,0,
	NBT_LIST, 0,12, 'T','i','l','e','E','n','t','i','t','i','e','s', NBT_DICT, 0,0,0,0,
NBT_END,
};

cc_result Schematic_Save(struct Stream* stream) {
	cc_uint8 tmp[256], chunk[8192] = { 0 };
	cc_result res;
	int i;

	Mem_Copy(tmp, sc_begin, sizeof(sc_begin));
	{
		Stream_SetU16_BE(&tmp[41], World.Width);
		Stream_SetU16_BE(&tmp[52], World.Height);
		Stream_SetU16_BE(&tmp[63], World.Length);
		Stream_SetU32_BE(&tmp[74], World.Volume);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(sc_begin)))) return res;
	if ((res = Stream_Write(stream, World.Blocks, World.Volume))) return res;

	Mem_Copy(tmp, sc_data, sizeof(sc_data));
	{
		Stream_SetU32_BE(&tmp[7], World.Volume);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(sc_data)))) return res;

	for (i = 0; i < World.Volume; i += sizeof(chunk)) {
		int count = World.Volume - i; count = min(count, sizeof(chunk));
		if ((res = Stream_Write(stream, chunk, count))) return res;
	}
	return Stream_Write(stream, sc_end, sizeof(sc_end));
}
