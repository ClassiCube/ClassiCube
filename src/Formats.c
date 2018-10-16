#include "Formats.h"
#include "World.h"
#include "Deflate.h"
#include "Block.h"
#include "Entity.h"
#include "Platform.h"
#include "ExtMath.h"
#include "ErrorHandler.h"
#include "Game.h"
#include "ServerConnection.h"
#include "Event.h"
#include "Funcs.h"
#include "Errors.h"
#include "Stream.h"

static ReturnCode Map_ReadBlocks(struct Stream* stream) {
	World_BlocksSize = World_Width * World_Length * World_Height;
	World_Blocks     = Mem_Alloc(World_BlocksSize, 1, "map blocks");
#ifdef EXTENDED_BLOCKS
	World_Blocks2    = World_Blocks;
#endif
	return Stream_Read(stream, World_Blocks, World_BlocksSize);
}

static ReturnCode Map_SkipGZipHeader(struct Stream* stream) {
	struct GZipHeader gzHeader;
	ReturnCode res;
	GZipHeader_Init(&gzHeader);

	while (!gzHeader.Done) {
		if ((res = GZipHeader_Read(stream, &gzHeader))) return res;
	}
	return 0;
}

NOINLINE_ IMapImporter Map_FindImporter(const String* path) {
	static String cw  = String_FromConst(".cw"),  lvl = String_FromConst(".lvl");
	static String fcm = String_FromConst(".fcm"), dat = String_FromConst(".dat");

	if (String_CaselessEnds(path, &cw))  return Cw_Load;
	if (String_CaselessEnds(path, &lvl)) return Lvl_Load;
	if (String_CaselessEnds(path, &fcm)) return Fcm_Load;
	if (String_CaselessEnds(path, &dat)) return Dat_Load;

	return NULL;
}


/*########################################################################################################################*
*--------------------------------------------------MCSharp level Format---------------------------------------------------*
*#########################################################################################################################*/
#define LVL_CUSTOMTILE 163
#define LVL_CHUNKSIZE 16
const uint8_t Lvl_table[256] = {
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

static ReturnCode Lvl_ReadCustomBlocks(struct Stream* stream) {	
	uint8_t chunk[LVL_CHUNKSIZE * LVL_CHUNKSIZE * LVL_CHUNKSIZE];
	uint8_t hasCustom;
	int baseIndex, index, xx, yy, zz;
	ReturnCode res;
	int x, y, z, i;

	/* skip bounds checks when we know chunk is entirely inside map */
	int adjWidth  = World_Width  & ~0x0F;
	int adjHeight = World_Height & ~0x0F;
	int adjLength = World_Length & ~0x0F;

	for (y = 0; y < World_Height; y += LVL_CHUNKSIZE) {
		for (z = 0; z < World_Length; z += LVL_CHUNKSIZE) {
			for (x = 0; x < World_Width; x += LVL_CHUNKSIZE) {

				if ((res = stream->ReadU8(stream, &hasCustom))) return res;
				if (hasCustom != 1) continue;
				if ((res = Stream_Read(stream, chunk, sizeof(chunk)))) return res;
				baseIndex = World_Pack(x, y, z);

				if ((x + LVL_CHUNKSIZE) <= adjWidth && (y + LVL_CHUNKSIZE) <= adjHeight && (z + LVL_CHUNKSIZE) <= adjLength) {
					for (i = 0; i < sizeof(chunk); i++) {
						xx = i & 0xF; yy = (i >> 8) & 0xF; zz = (i >> 4) & 0xF;

						index = baseIndex + World_Pack(xx, yy, zz);
						World_Blocks[index] = World_Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World_Blocks[index];
					}
				} else {
					for (i = 0; i < sizeof(chunk); i++) {
						xx = i & 0xF; yy = (i >> 8) & 0xF; zz = (i >> 4) & 0xF;
						if ((x + xx) >= World_Width || (y + yy) >= World_Height || (z + zz) >= World_Length) continue;

						index = baseIndex + World_Pack(xx, yy, zz);
						World_Blocks[index] = World_Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World_Blocks[index];
					}
				}			
			}
		}
	}
	return 0;
}

ReturnCode Lvl_Load(struct Stream* stream) {
	uint8_t header[18];		
	uint8_t* blocks;
	uint8_t section;
	ReturnCode res;
	int i;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);
	
	if ((res = Map_SkipGZipHeader(stream)))                       return res;
	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;
	if (Stream_GetU16_LE(&header[0]) != 1874) return LVL_ERR_VERSION;

	World_Width  = Stream_GetU16_LE(&header[2]);
	World_Length = Stream_GetU16_LE(&header[4]);
	World_Height = Stream_GetU16_LE(&header[6]);

	p->Spawn.X = Stream_GetU16_LE(&header[8]);
	p->Spawn.Z = Stream_GetU16_LE(&header[10]);
	p->Spawn.Y = Stream_GetU16_LE(&header[12]);
	p->SpawnRotY  = Math_Packed2Deg(header[14]);
	p->SpawnHeadX = Math_Packed2Deg(header[15]);
	/* (2) pervisit, perbuild permissions */

	if ((res = Map_ReadBlocks(&compStream))) return res;
	blocks = World_Blocks;
	/* Bulk convert 4 blocks at once */
	for (i = 0; i < (World_BlocksSize & ~3); i += 4) {
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
		*blocks = Lvl_table[*blocks]; blocks++;
	}
	for (; i < World_BlocksSize; i++) {
		*blocks = Lvl_table[*blocks]; blocks++;
	}

	/* 0xBD section type is not present in older .lvl files */
	res = compStream.ReadU8(&compStream, &section);
	if (res == ERR_END_OF_STREAM) return 0;

	if (res) return res;
	return section == 0xBD ? Lvl_ReadCustomBlocks(&compStream) : 0;
}


/*########################################################################################################################*
*----------------------------------------------------fCraft map format----------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Fcm_ReadString(struct Stream* stream) {
	uint8_t data[2];
	int len;
	ReturnCode res;

	if ((res = Stream_Read(stream, data, sizeof(data)))) return res;
	len = Stream_GetU16_LE(data);

	return stream->Skip(stream, len);
}

ReturnCode Fcm_Load(struct Stream* stream) {
	uint8_t header[79];	
	ReturnCode res;
	int i, count;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;
	if (Stream_GetU32_LE(&header[0]) != 0x0FC2AF40UL)        return FCM_ERR_IDENTIFIER;
	if (header[4] != 13) return FCM_ERR_REVISION;
	
	World_Width  = Stream_GetU16_LE(&header[5]);
	World_Height = Stream_GetU16_LE(&header[7]);
	World_Length = Stream_GetU16_LE(&header[9]);
	
	p->Spawn.X = ((int)Stream_GetU32_LE(&header[11])) / 32.0f;
	p->Spawn.Y = ((int)Stream_GetU32_LE(&header[15])) / 32.0f;
	p->Spawn.Z = ((int)Stream_GetU32_LE(&header[19])) / 32.0f;
	p->SpawnRotY  = Math_Packed2Deg(header[23]);
	p->SpawnHeadX = Math_Packed2Deg(header[24]);

	/* header[25] (4) date modified */
	/* header[29] (4) date created */
	Mem_Copy(&World_Uuid, &header[33], sizeof(World_Uuid));
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
	NBT_END, NBT_I8,  NBT_I16, NBT_I32,  NBT_I64,  NBT_F32, 
	NBT_R64, NBT_I8S, NBT_STR, NBT_LIST, NBT_DICT, NBT_I32S
};

#define NBT_SMALL_SIZE STRING_SIZE
struct NbtTag;
struct NbtTag {
	struct NbtTag* Parent;
	uint8_t  TagID;
	char     NameBuffer[NBT_SMALL_SIZE];
	uint32_t NameSize;
	uint32_t DataSize; /* size of data for arrays */

	union {
		uint8_t  Value_U8;
		int16_t  Value_I16;
		uint16_t Value_U16;
		uint32_t Value_U32;
		float    Value_F32;
		uint8_t  DataSmall[NBT_SMALL_SIZE];
		uint8_t* DataBig; /* malloc for big byte arrays */
	};
};

static uint8_t NbtTag_U8(struct NbtTag* tag) {
	if (tag->TagID != NBT_I8) ErrorHandler_Fail("Expected I8 NBT tag");
	return tag->Value_U8;
}

static int16_t NbtTag_I16(struct NbtTag* tag) {
	if (tag->TagID != NBT_I16) ErrorHandler_Fail("Expected I16 NBT tag");
	return tag->Value_I16;
}

static uint16_t NbtTag_U16(struct NbtTag* tag) {
	if (tag->TagID != NBT_I16) ErrorHandler_Fail("Expected I16 NBT tag");
	return tag->Value_U16;
}

static float NbtTag_F32(struct NbtTag* tag) {
	if (tag->TagID != NBT_F32) ErrorHandler_Fail("Expected F32 NBT tag");
	return tag->Value_F32;
}

static uint8_t* NbtTag_U8_Array(struct NbtTag* tag, int minSize) {
	if (tag->TagID != NBT_I8S) ErrorHandler_Fail("Expected I8_Array NBT tag");
	if (tag->DataSize < minSize) ErrorHandler_Fail("I8_Array NBT tag too small");

	return tag->DataSize <= NBT_SMALL_SIZE ? tag->DataSmall : tag->DataBig;
}

static String NbtTag_String(struct NbtTag* tag) {
	if (tag->TagID != NBT_STR) ErrorHandler_Fail("Expected String NBT tag");
	return String_Init(tag->DataSmall, tag->DataSize, tag->DataSize);
}

static ReturnCode Nbt_ReadString(struct Stream* stream, char* strBuffer, uint32_t* strLen) {
	uint8_t buffer[NBT_SMALL_SIZE * 4];
	int len;
	String str;
	ReturnCode res;

	if ((res = Stream_Read(stream, buffer, 2)))   return res;
	len = Stream_GetU16_BE(buffer);

	if (len > Array_Elems(buffer)) return CW_ERR_STRING_LEN;
	if ((res = Stream_Read(stream, buffer, len))) return res;

	str = String_Init(strBuffer, 0, NBT_SMALL_SIZE);
	String_DecodeUtf8(&str, buffer, len);
	*strLen = str.length; return 0;
}

typedef void (*Nbt_Callback)(struct NbtTag* tag);
static ReturnCode Nbt_ReadTag(uint8_t typeId, bool readTagName, struct Stream* stream, struct NbtTag* parent, Nbt_Callback callback) {
	struct NbtTag tag;
	uint8_t childType;
	uint8_t tmp[5];	
	ReturnCode res;
	uint32_t i, count;
	
	if (typeId == NBT_END) return 0;
	tag.TagID = typeId; tag.Parent = parent;
	tag.NameSize = 0;   tag.DataSize = 0;

	if (readTagName) {
		res = Nbt_ReadString(stream, tag.NameBuffer, &tag.NameSize);
		if (res) return res;
	}

	switch (typeId) {
	case NBT_I8:
		res = stream->ReadU8(stream, &tag.Value_U8);
		break;
	case NBT_I16:
		res = Stream_Read(stream, tmp, 2);
		tag.Value_I16 = Stream_GetU16_BE(tmp);
		break;
	case NBT_I32:
	case NBT_F32:
		res = Stream_ReadU32_BE(stream, &tag.Value_U32);
		break;
	case NBT_I64:
	case NBT_R64:
		res = stream->Skip(stream, 8);
		break; /* (8) data */

	case NBT_I8S:
		if ((res = Stream_ReadU32_BE(stream, &tag.DataSize))) break;

		if (tag.DataSize <= NBT_SMALL_SIZE) {
			res = Stream_Read(stream, tag.DataSmall, tag.DataSize);
		} else {
			tag.DataBig = Mem_Alloc(tag.DataSize, 1, "NBT data");
			res = Stream_Read(stream, tag.DataBig, tag.DataSize);
			if (res) { Mem_Free(tag.DataBig); }
		}
		break;
	case NBT_STR:
		res = Nbt_ReadString(stream, tag.DataSmall, &tag.DataSize);
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

	case NBT_I32S: return NBT_ERR_INT32S;
	default:       return NBT_ERR_UNKNOWN;
	}

	if (res) return res;
	callback(&tag);
	/* callback sets DataBig to NULL, if doesn't want it to be freed */
	if (tag.DataSize > NBT_SMALL_SIZE) Mem_Free(tag.DataBig);
	return 0;
}

static bool IsTag(struct NbtTag* tag, const char* tagName) {
	String name = { tag->NameBuffer, tag->NameSize, tag->NameSize };
	return String_CaselessEqualsConst(&name, tagName);
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld format----------------------------------------------------*
*#########################################################################################################################*/
static void Cw_Callback_1(struct NbtTag* tag) {
	if (IsTag(tag, "X")) { World_Width  = NbtTag_U16(tag); return; }
	if (IsTag(tag, "Y")) { World_Height = NbtTag_U16(tag); return; }
	if (IsTag(tag, "Z")) { World_Length = NbtTag_U16(tag); return; }

	if (IsTag(tag, "UUID")) {
		if (tag->DataSize != sizeof(World_Uuid)) ErrorHandler_Fail("Map UUID must be 16 bytes");
		Mem_Copy(World_Uuid, tag->DataSmall, sizeof(World_Uuid));
		return;
	}

	if (IsTag(tag, "BlockArray")) {
		World_BlocksSize = tag->DataSize;
		if (tag->DataSize < NBT_SMALL_SIZE) {
			World_Blocks = Mem_Alloc(World_BlocksSize, 1, ".cw map blocks");
			Mem_Copy(World_Blocks, tag->DataSmall, tag->DataSize);
		} else {
			World_Blocks = tag->DataBig;
			tag->DataBig = NULL; /* So Nbt_ReadTag doesn't call Mem_Free on World_Blocks */
		}
#ifdef EXTENDED_BLOCKS
		World_Blocks2 = World_Blocks;
#endif
	}
}

static void Cw_Callback_2(struct NbtTag* tag) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!IsTag(tag->Parent, "Spawn")) return;
	
	if (IsTag(tag, "X")) { p->Spawn.X = NbtTag_I16(tag); return; }
	if (IsTag(tag, "Y")) { p->Spawn.Y = NbtTag_I16(tag); return; }
	if (IsTag(tag, "Z")) { p->Spawn.Z = NbtTag_I16(tag); return; }
	if (IsTag(tag, "H")) { p->SpawnRotY  = Math_Deg2Packed(NbtTag_U8(tag)); return; }
	if (IsTag(tag, "P")) { p->SpawnHeadX = Math_Deg2Packed(NbtTag_U8(tag)); return; }
}

BlockID cw_curID;
int cw_colR, cw_colG, cw_colB;
static PackedCol Cw_ParseCol(PackedCol defValue) {
	int r = cw_colR, g = cw_colG, b = cw_colB;
	if (r > 255 || g > 255 || b > 255) return defValue;

	PackedCol col = PACKEDCOL_CONST((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);
	return col;		
}

static void Cw_Callback_4(struct NbtTag* tag) {
	struct LocalPlayer* p = &LocalPlayer_Instance;
	if (!IsTag(tag->Parent->Parent, "CPE")) return;
	if (!IsTag(tag->Parent->Parent->Parent, "Metadata")) return;

	if (IsTag(tag->Parent, "ClickDistance")) {
		if (IsTag(tag, "Distance")) { p->ReachDistance = NbtTag_U16(tag) / 32.0f; return; }
	}
	if (IsTag(tag->Parent, "EnvWeatherType")) {
		if (IsTag(tag, "WeatherType")) { Env_SetWeather(NbtTag_U8(tag)); return; }
	}

	if (IsTag(tag->Parent, "EnvMapAppearance")) {
		if (IsTag(tag, "SideBlock")) { Env_SetSidesBlock(NbtTag_U8(tag));  return; }
		if (IsTag(tag, "EdgeBlock")) { Env_SetEdgeBlock(NbtTag_U8(tag));   return; }
		if (IsTag(tag, "SideLevel")) { Env_SetEdgeHeight(NbtTag_I16(tag)); return; }

		if (IsTag(tag, "TextureURL")) {
			String url = NbtTag_String(tag);
			if (Game_AllowServerTextures && url.length) {
				ServerConnection_RetrieveTexturePack(&url);
			}
			return;
		}
	}

	/* Callback for compound tag is called after all its children have been processed */
	if (IsTag(tag->Parent, "EnvColors")) {
		if (IsTag(tag, "Sky")) {
			Env_SetSkyCol(Cw_ParseCol(Env_DefaultSkyCol)); return;
		} else if (IsTag(tag, "Cloud")) {
			Env_SetCloudsCol(Cw_ParseCol(Env_DefaultCloudsCol)); return;
		} else if (IsTag(tag, "Fog")) {
			Env_SetFogCol(Cw_ParseCol(Env_DefaultFogCol)); return;
		} else if (IsTag(tag, "Sunlight")) {
			Env_SetSunCol(Cw_ParseCol(Env_DefaultSunCol)); return;
		} else if (IsTag(tag, "Ambient")) {
			Env_SetShadowCol(Cw_ParseCol(Env_DefaultShadowCol)); return;
		}
	}

	if (IsTag(tag->Parent, "BlockDefinitions")) {
		String tagName = { tag->NameBuffer, tag->NameSize, tag->NameSize };
		String blockStr = String_FromConst("Block");
		if (!String_CaselessStarts(&tagName, &blockStr)) return;
		BlockID id = cw_curID;

		/* hack for sprite draw (can't rely on order of tags when reading) */
		if (Block_SpriteOffset[id] == 0) {
			Block_SpriteOffset[id] = Block_Draw[id];
			Block_Draw[id] = DRAW_SPRITE;
		} else {
			Block_SpriteOffset[id] = 0;
		}

		Block_DefineCustom(id);
		Block_CanPlace[id]  = true;
		Block_CanDelete[id] = true;
		Event_RaiseVoid(&BlockEvents_PermissionsChanged);

		cw_curID = 0;
	}
}

static void Cw_Callback_5(struct NbtTag* tag) {
	BlockID id = cw_curID;
	uint8_t* arr;
	uint8_t sound;

	if (!IsTag(tag->Parent->Parent->Parent, "CPE")) return;
	if (!IsTag(tag->Parent->Parent->Parent->Parent, "Metadata")) return;

	if (IsTag(tag->Parent->Parent, "EnvColors")) {
		if (IsTag(tag, "R")) { cw_colR = NbtTag_U16(tag); return; }
		if (IsTag(tag, "G")) { cw_colG = NbtTag_U16(tag); return; }
		if (IsTag(tag, "B")) { cw_colB = NbtTag_U16(tag); return; }
	}

	if (IsTag(tag->Parent->Parent, "BlockDefinitions") && Game_AllowCustomBlocks) {
		if (IsTag(tag, "ID"))             { cw_curID = NbtTag_U8(tag); return; }
		if (IsTag(tag, "CollideType"))    { Block_SetCollide(id, NbtTag_U8(tag)); return; }
		if (IsTag(tag, "Speed"))          { Block_SpeedMultiplier[id] = NbtTag_F32(tag); return; }
		if (IsTag(tag, "TransmitsLight")) { Block_BlocksLight[id] = NbtTag_U8(tag) == 0; return; }
		if (IsTag(tag, "FullBright"))     { Block_FullBright[id] = NbtTag_U8(tag) != 0; return; }
		if (IsTag(tag, "BlockDraw"))      { Block_Draw[id] = NbtTag_U8(tag); return; }
		if (IsTag(tag, "Shape"))          { Block_SpriteOffset[id] = NbtTag_U8(tag); return; }

		if (IsTag(tag, "Name")) {
			String name = NbtTag_String(tag);
			Block_SetName(id, &name);
			return;
		}

		if (IsTag(tag, "Textures")) {
			arr = NbtTag_U8_Array(tag, 6);
			Block_SetTex(arr[0], FACE_YMAX, id);
			Block_SetTex(arr[1], FACE_YMIN, id);
			Block_SetTex(arr[2], FACE_XMIN, id);
			Block_SetTex(arr[3], FACE_XMAX, id);
			Block_SetTex(arr[4], FACE_ZMIN, id);
			Block_SetTex(arr[5], FACE_ZMAX, id);
			return;
		}
		
		if (IsTag(tag, "WalkSound")) {
			sound = NbtTag_U8(tag);
			Block_DigSounds[id]  = sound;
			Block_StepSounds[id] = sound;
			if (sound == SOUND_GLASS) Block_StepSounds[id] = SOUND_STONE;
			return;
		}

		if (IsTag(tag, "Fog")) {
			arr = NbtTag_U8_Array(tag, 4);
			Block_FogDensity[id] = (arr[0] + 1) / 128.0f;
			/* Fix for older ClassicalSharp versions which saved wrong fog density value */
			if (arr[0] == 0xFF) Block_FogDensity[id] = 0.0f;
 
			Block_FogCol[id].R = arr[1];
			Block_FogCol[id].G = arr[2];
			Block_FogCol[id].B = arr[3];
			Block_FogCol[id].A = 255;
			return;
		}

		if (IsTag(tag, "Coords")) {
			arr = NbtTag_U8_Array(tag, 6);
			Block_MinBB[id].X = arr[0] / 16.0f; Block_MaxBB[id].X = arr[3] / 16.0f;
			Block_MinBB[id].Y = arr[1] / 16.0f; Block_MaxBB[id].Y = arr[4] / 16.0f;
			Block_MinBB[id].Z = arr[2] / 16.0f; Block_MaxBB[id].Z = arr[5] / 16.0f;
			return;
		}
	}
}

static void Cw_Callback(struct NbtTag* tag) {
	int depth = 0;
	struct NbtTag* tmp = tag->Parent;
	while (tmp) { depth++; tmp = tmp->Parent; }

	switch (depth) {
	case 1: Cw_Callback_1(tag); return;
	case 2: Cw_Callback_2(tag); return;
	case 4: Cw_Callback_4(tag); return;
	case 5: Cw_Callback_5(tag); return;
	}
	/* ClassicWorld -> Metadata -> CPE -> ExtName -> [values]
	        0             1         2        3          4   */
}

ReturnCode Cw_Load(struct Stream* stream) {
	uint8_t tag;
	ReturnCode res;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	if ((res = Map_SkipGZipHeader(stream))) return res;
	if ((res = compStream.ReadU8(&compStream, &tag))) return res;
	if (tag != NBT_DICT) return CW_ERR_ROOT_TAG;

	res = Nbt_ReadTag(NBT_DICT, true, &compStream, NULL, Cw_Callback);
	if (res) return res;

	/* Older versions incorrectly multiplied spawn coords by * 32, so we check for that */
	Vector3* spawn = &LocalPlayer_Instance.Spawn;
	Vector3I P; Vector3I_Floor(&P, spawn);
	if (!World_IsValidPos_3I(P)) { spawn->X /= 32.0f; spawn->Y /= 32.0f; spawn->Z /= 32.0f; }
	return 0;
}


/*########################################################################################################################*
*-------------------------------------------------Minecraft .dat format---------------------------------------------------*
*#########################################################################################################################*/
enum JTypeCode {
	TC_NULL = 0x70, TC_REFERENCE = 0x71, TC_CLASSDESC = 0x72, TC_OBJECT = 0x73, 
	TC_STRING = 0x74, TC_ARRAY = 0x75, TC_ENDBLOCKDATA = 0x78
};
enum JFieldType {
	JFIELD_I8 = 'B', JFIELD_F32 = 'F', JFIELD_I32 = 'I', JFIELD_I64 = 'J',
	JFIELD_BOOL = 'Z', JFIELD_ARRAY = '[', JFIELD_OBJECT = 'L'
};

#define JNAME_SIZE 48
struct JFieldDesc {
	uint8_t Type;
	char FieldName[JNAME_SIZE];
	union {
		uint8_t  Value_U8;
		int32_t  Value_I32;
		uint32_t Value_U32;
		float    Value_F32;
		struct { uint8_t* Value_Ptr; uint32_t Value_Size; };
	};
};

struct JClassDesc {
	char ClassName[JNAME_SIZE];
	int FieldsCount;
	struct JFieldDesc Fields[22];
};

static ReturnCode Dat_ReadString(struct Stream* stream, char* buffer) {
	int len;
	ReturnCode res;

	if ((res = Stream_Read(stream, buffer, 2))) return res;
	len = Stream_GetU16_BE(buffer);

	Mem_Set(buffer, 0, JNAME_SIZE);
	if (len > JNAME_SIZE) return DAT_ERR_JSTRING_LEN;
	return Stream_Read(stream, buffer, len);
}

static ReturnCode Dat_ReadFieldDesc(struct Stream* stream, struct JFieldDesc* desc) {
	uint8_t typeCode;
	char className1[JNAME_SIZE];
	ReturnCode res;

	if ((res = stream->ReadU8(stream, &desc->Type)))     return res;
	if ((res = Dat_ReadString(stream, desc->FieldName))) return res;

	if (desc->Type == JFIELD_ARRAY || desc->Type == JFIELD_OBJECT) {		
		if ((res = stream->ReadU8(stream, &typeCode))) return res;

		if (typeCode == TC_STRING) {
			return Dat_ReadString(stream, className1);
		} else if (typeCode == TC_REFERENCE) {
			return stream->Skip(stream, 4); /* (4) handle */
		} else {
			return DAT_ERR_JFIELD_CLASS_NAME;
		}
	}
	return 0;
}

static ReturnCode Dat_ReadClassDesc(struct Stream* stream, struct JClassDesc* desc) {
	uint8_t typeCode;
	uint8_t count[2];
	struct JClassDesc superClassDesc;
	ReturnCode res;
	int i;

	if ((res = stream->ReadU8(stream, &typeCode))) return res;
	if (typeCode == TC_NULL) { desc->ClassName[0] = '\0'; desc->FieldsCount = 0; return 0; }
	if (typeCode != TC_CLASSDESC) return DAT_ERR_JCLASS_TYPE;

	if ((res = Dat_ReadString(stream, desc->ClassName))) return res;
	if ((res = stream->Skip(stream, 9))) return res; /* (8) serial version UID, (1) flags */

	if ((res = Stream_Read(stream, count, 2))) return res;
	desc->FieldsCount = Stream_GetU16_BE(count);
	if (desc->FieldsCount > Array_Elems(desc->Fields)) return DAT_ERR_JCLASS_FIELDS;
	
	for (i = 0; i < desc->FieldsCount; i++) {
		if ((res = Dat_ReadFieldDesc(stream, &desc->Fields[i]))) return res;
	}

	if ((res = stream->ReadU8(stream, &typeCode))) return res;
	if (typeCode != TC_ENDBLOCKDATA) return DAT_ERR_JCLASS_ANNOTATION;

	return Dat_ReadClassDesc(stream, &superClassDesc);
}

static ReturnCode Dat_ReadFieldData(struct Stream* stream, struct JFieldDesc* field) {
	uint8_t typeCode;
	String fieldName;
	uint32_t count;
	struct JClassDesc arrayClassDesc;
	ReturnCode res;

	switch (field->Type) {
	case JFIELD_I8:
	case JFIELD_BOOL:
		return stream->ReadU8(stream, &field->Value_U8);
	case JFIELD_F32:
	case JFIELD_I32:
		return Stream_ReadU32_BE(stream, &field->Value_U32);
	case JFIELD_I64:
		return stream->Skip(stream, 8); /* (8) data */

	case JFIELD_OBJECT: {
		/* Luckily for us, we only have to account for blockMap object */
		/* Other objects (e.g. player) are stored after the fields we actually care about, so ignore them */
		fieldName = String_FromRawArray(field->FieldName);
		if (!String_CaselessEqualsConst(&fieldName, "blockMap")) return 0;
		if ((res = stream->ReadU8(stream, &typeCode))) return res;

		/* Skip all blockMap data with awful hacks */
		/* These offsets were based on server_level.dat map from original minecraft classic server */
		if (typeCode == TC_OBJECT) {
			if ((res = stream->Skip(stream, 315)))         return res;
			if ((res = Stream_ReadU32_BE(stream, &count))) return res;

			if ((res = stream->Skip(stream, 17 * count)))  return res;
			if ((res = stream->Skip(stream, 152)))         return res;
		} else if (typeCode != TC_NULL) {
			/* WoM maps have this field as null, which makes things easier for us */
			return DAT_ERR_JOBJECT_TYPE;
		}
	} break;

	case JFIELD_ARRAY: {
		if ((res = stream->ReadU8(stream, &typeCode))) return res;
		if (typeCode == TC_NULL) break;
		if (typeCode != TC_ARRAY) return DAT_ERR_JARRAY_TYPE;

		if ((res = Dat_ReadClassDesc(stream, &arrayClassDesc))) return res;
		if (arrayClassDesc.ClassName[1] != JFIELD_I8) return DAT_ERR_JARRAY_CONTENT;

		if ((res = Stream_ReadU32_BE(stream, &count))) return res;
		field->Value_Size = count;

		field->Value_Ptr = Mem_Alloc(count, 1, ".dat map blocks");
		res = Stream_Read(stream, field->Value_Ptr, count);	
		if (res) { Mem_Free(field->Value_Ptr); return res; }
	} break;
	}
	return 0;
}

static int32_t Dat_I32(struct JFieldDesc* field) {
	if (field->Type != JFIELD_I32) ErrorHandler_Fail("Field type must be Int32");
	return field->Value_I32;
}

ReturnCode Dat_Load(struct Stream* stream) {
	uint8_t header[10];
	struct JClassDesc obj;
	struct JFieldDesc* field;
	String fieldName;
	ReturnCode res;
	int i;

	struct LocalPlayer* p = &LocalPlayer_Instance;
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	if ((res = Map_SkipGZipHeader(stream)))                       return res;
	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;
	/* .dat header */
	if (Stream_GetU32_BE(&header[0]) != 0x271BB788) return DAT_ERR_IDENTIFIER;
	if (header[4] != 0x02) return DAT_ERR_VERSION;

	/* Java seralisation headers */
	if (Stream_GetU16_BE(&header[5]) != 0xACED) return DAT_ERR_JIDENTIFIER;
	if (Stream_GetU16_BE(&header[7]) != 0x0005) return DAT_ERR_JVERSION;
	if (header[9] != TC_OBJECT)                 return DAT_ERR_ROOT_TYPE;
	if ((res = Dat_ReadClassDesc(&compStream, &obj))) return res;

	for (i = 0; i < obj.FieldsCount; i++) {
		field = &obj.Fields[i];
		if ((res = Dat_ReadFieldData(&compStream, field))) return res;
		fieldName = String_FromRawArray(field->FieldName);

		if (String_CaselessEqualsConst(&fieldName, "width")) {
			World_Width  = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "height")) {
			World_Length = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "depth")) {
			World_Height = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "blocks")) {
			if (field->Type != JFIELD_ARRAY) ErrorHandler_Fail("Blocks field must be Array");
			World_Blocks     = field->Value_Ptr;
#ifdef EXTENDED_BLOCKS
			World_Blocks2    = World_Blocks;
#endif
			World_BlocksSize = field->Value_Size;
		} else if (String_CaselessEqualsConst(&fieldName, "xSpawn")) {
			p->Spawn.X = (float)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "ySpawn")) {
			p->Spawn.Y = (float)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "zSpawn")) {
			p->Spawn.Z = (float)Dat_I32(field);
		}
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld export----------------------------------------------------*
*#########################################################################################################################*/
#define CW_META_VERSION 'E','x','t','e','n','s','i','o','n','V','e','r','s','i','o','n'
#define CW_META_RGB NBT_I16,0,1,'R',0,0,  NBT_I16,0,1,'G',0,0,  NBT_I16,0,1,'B',0,0,
static int Cw_WriteEndString(uint8_t* data, const String* text) {
	int i, len = 0;
	uint8_t* cur = data + 2;

	for (i = 0; i < text->length; i++) {
		Codepoint cp = Convert_CP437ToUnicode(text->buffer[i]);
		int bytes    = Stream_WriteUtf8(cur, cp);
		len += bytes; cur += bytes;
	}

	Stream_SetU16_BE(data, len);
	*cur = NBT_END;
	return len + 1;
}

uint8_t cw_begin[131] = {
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
uint8_t cw_meta_cpe[395] = {
	NBT_DICT, 0,8,  'M','e','t','a','d','a','t','a',
		NBT_DICT, 0,3, 'C','P','E',
			NBT_DICT, 0,13, 'C','l','i','c','k','D','i','s','t','a','n','c','e',
				NBT_I32,  0,16, CW_META_VERSION,                 0,0,0,1,
				NBT_I16,  0,8,  'D','i','s','t','a','n','c','e', 0,0,
			NBT_END,
			NBT_DICT, 0,14, 'E','n','v','W','e','a','t','h','e','r','T','y','p','e',
				NBT_I32,  0,16, CW_META_VERSION,                             0,0,0,1,
				NBT_I8,   0,11, 'W','e','a','t','h','e','r','T','y','p','e', 0,
			NBT_END,
			NBT_DICT, 0,9,  'E','n','v','C','o','l','o','r','s',
				NBT_I32,  0,16, CW_META_VERSION,                0,0,0,1,
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
				NBT_I32,  0,16, CW_META_VERSION,                         0,0,0,1,
				NBT_I8,   0,9,  'S','i','d','e','B','l','o','c','k',     0,
				NBT_I8,   0,9,  'E','d','g','e','B','l','o','c','k',     0,
				NBT_I16,  0,9,  'S','i','d','e','L','e','v','e','l',     0,0,
				NBT_STR,  0,10, 'T','e','x','t','u','r','e','U','R','L', 0,0,
};
uint8_t cw_meta_defs[19] = {
			NBT_DICT, 0,16, 'B','l','o','c','k','D','e','f','i','n','i','t','i','o','n','s',
};
uint8_t cw_meta_def[173] = {
				NBT_DICT, 0,7,  'B','l','o','c','k','\0','\0',
					NBT_I8,  0,2,  'I','D',                              0,
					NBT_I8,  0,11, 'C','o','l','l','i','d','e','T','y','p','e', 0,
					NBT_F32, 0,5,  'S','p','e','e','d',                  0,0,0,0,
					NBT_I8S, 0,8,  'T','e','x','t','u','r','e','s',      0,0,0,6, 0,0,0,0,0,0,
					NBT_I8,  0,14, 'T','r','a','n','s','m','i','t','s','L','i','g','h','t', 0,
					NBT_I8,  0,9,  'W','a','l','k','S','o','u','n','d',  0,
					NBT_I8,  0,10, 'F','u','l','l','B','r','i','g','h','t', 0,
					NBT_I8,  0,5,  'S','h','a','p','e',                  0,
					NBT_I8,  0,9,  'B','l','o','c','k','D','r','a','w',  0,
					NBT_I8S, 0,3,  'F','o','g',                          0,0,0,4, 0,0,0,0,
					NBT_I8S, 0,6,  'C','o','o','r','d','s',              0,0,0,6, 0,0,0,0,0,0,
					NBT_STR, 0,4,  'N','a','m','e',                      0,0,
};
uint8_t cw_end[4] = {
			NBT_END,
		NBT_END,
	NBT_END,
NBT_END,
};

static ReturnCode Cw_WriteBockDef(struct Stream* stream, int b) {
	uint8_t tmp[512];
	bool sprite = Block_Draw[b] == DRAW_SPRITE;
	union IntAndFloat speed;

	Mem_Copy(tmp, cw_meta_def, sizeof(cw_meta_def));
	{
		/* Hacky unique tag name for each */
		String nameStr = { &tmp[8], 0, 2 };
		String_AppendHex(&nameStr, b);
		tmp[15] = b;

		tmp[30] = Block_Collide[b];
		speed.f = Block_SpeedMultiplier[b];
		Stream_SetU32_BE(&tmp[39], speed.u);

		tmp[58] = (uint8_t)Block_GetTex(b, FACE_YMAX);
		tmp[59] = (uint8_t)Block_GetTex(b, FACE_YMIN);
		tmp[60] = (uint8_t)Block_GetTex(b, FACE_XMIN);
		tmp[61] = (uint8_t)Block_GetTex(b, FACE_XMAX);
		tmp[62] = (uint8_t)Block_GetTex(b, FACE_ZMIN);
		tmp[63] = (uint8_t)Block_GetTex(b, FACE_ZMAX);

		tmp[81]  = Block_BlocksLight[b] ? 0 : 1;
		tmp[94]  = Block_DigSounds[b];
		tmp[108] = Block_FullBright[b] ? 1 : 0;
		tmp[117] = sprite ? 0 : (uint8_t)(Block_MaxBB[b].Y * 16);
		tmp[130] = sprite ? Block_SpriteOffset[b] : Block_Draw[b];

		uint8_t fog = (uint8_t)(128 * Block_FogDensity[b] - 1);
		PackedCol col = Block_FogCol[b];
		tmp[141] = Block_FogDensity[b] ? fog : 0;
		tmp[142] = col.R; tmp[143] = col.G; tmp[144] = col.B;

		Vector3 minBB = Block_MinBB[b], maxBB = Block_MaxBB[b];
		tmp[158] = (uint8_t)(minBB.X * 16); tmp[159] = (uint8_t)(minBB.Y * 16); tmp[160] = (uint8_t)(minBB.Z * 16);
		tmp[161] = (uint8_t)(maxBB.X * 16); tmp[162] = (uint8_t)(maxBB.Y * 16); tmp[163] = (uint8_t)(maxBB.Z * 16);
	}

	String name = Block_UNSAFE_GetName(b);
	int len     = Cw_WriteEndString(&tmp[171], &name);
	return Stream_Write(stream, tmp, sizeof(cw_meta_def) + len);
}

ReturnCode Cw_Save(struct Stream* stream) {
	uint8_t tmp[768];
	PackedCol col;
	struct LocalPlayer* p = &LocalPlayer_Instance;
	ReturnCode res;
	int b, len;

	Mem_Copy(tmp, cw_begin, sizeof(cw_begin));
	{
		Mem_Copy(&tmp[43], World_Uuid, sizeof(World_Uuid));
		Stream_SetU16_BE(&tmp[63], World_Width);
		Stream_SetU16_BE(&tmp[69], World_Height);
		Stream_SetU16_BE(&tmp[75], World_Length);
		Stream_SetU32_BE(&tmp[127], World_BlocksSize);
		
		Vector3 spawn = p->Spawn; /* TODO: Maybe keep real spawn too? */
		Stream_SetU16_BE(&tmp[89],  (uint16_t)p->Spawn.X);
		Stream_SetU16_BE(&tmp[95],  (uint16_t)p->Spawn.Y);
		Stream_SetU16_BE(&tmp[101], (uint16_t)p->Spawn.Z);
		tmp[107] = Math_Deg2Packed(p->SpawnRotY);
		tmp[112] = Math_Deg2Packed(p->SpawnHeadX);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(cw_begin)))) return res;
	if ((res = Stream_Write(stream, World_Blocks, World_BlocksSize))) return res;

	Mem_Copy(tmp, cw_meta_cpe, sizeof(cw_meta_cpe));
	{
		Stream_SetU16_BE(&tmp[67], (uint16_t)(LocalPlayer_Instance.ReachDistance * 32));
		tmp[124] = Env_Weather;

		col = Env_SkyCol;    tmp[172] = col.R; tmp[178] = col.G; tmp[184] = col.B;
		col = Env_CloudsCol; tmp[199] = col.R; tmp[205] = col.G; tmp[211] = col.B;
		col = Env_FogCol;    tmp[224] = col.R; tmp[230] = col.G; tmp[236] = col.B;
		col = Env_ShadowCol; tmp[253] = col.R; tmp[259] = col.G; tmp[265] = col.B;
		col = Env_SunCol;    tmp[283] = col.R; tmp[289] = col.G; tmp[295] = col.B;

		tmp[352] = (BlockRaw)Env_SidesBlock;
		tmp[365] = (BlockRaw)Env_EdgeBlock;
		Stream_SetU16_BE(&tmp[378], Env_EdgeHeight);
	}
	len = Cw_WriteEndString(&tmp[393], &World_TextureUrl);
	if ((res = Stream_Write(stream, tmp, sizeof(cw_meta_cpe) + len))) return res;

	if ((res = Stream_Write(stream, cw_meta_defs, sizeof(cw_meta_defs)))) return res;
	for (b = 1; b < 256; b++) {
		if (!Block_IsCustomDefined(b)) continue;
		if ((res = Cw_WriteBockDef(stream, b))) return res;
	}
	return Stream_Write(stream, cw_end, sizeof(cw_end));
}


/*########################################################################################################################*
*---------------------------------------------------Schematic export------------------------------------------------------*
*#########################################################################################################################*/

uint8_t sc_begin[78] = {
NBT_DICT, 0,9, 'S','c','h','e','m','a','t','i','c',
	NBT_STR,  0,9,  'M','a','t','e','r','i','a','l','s', 0,7, 'C','l','a','s','s','i','c',
	NBT_I16,  0,5,  'W','i','d','t','h',                 0,0,
	NBT_I16,  0,6,  'H','e','i','g','h','t',             0,0,
	NBT_I16,  0,6,  'L','e','n','g','t','h',             0,0,
	NBT_I8S,  0,6,  'B','l','o','c','k','s',             0,0,0,0,
};
uint8_t sc_data[11] = {
	NBT_I8S,  0,4,  'D','a','t','a',                     0,0,0,0,
};
uint8_t sc_end[37] = {
	NBT_LIST, 0,8,  'E','n','t','i','t','i','e','s',                 NBT_DICT, 0,0,0,0,
	NBT_LIST, 0,12, 'T','i','l','e','E','n','t','i','t','i','e','s', NBT_DICT, 0,0,0,0,
NBT_END,
};

ReturnCode Schematic_Save(struct Stream* stream) {
	uint8_t tmp[256], chunk[8192] = { 0 };
	ReturnCode res;
	int i;

	Mem_Copy(tmp, sc_begin, sizeof(sc_begin));
	{
		Stream_SetU16_BE(&tmp[41], World_Width);
		Stream_SetU16_BE(&tmp[52], World_Height);
		Stream_SetU16_BE(&tmp[63], World_Length);
		Stream_SetU32_BE(&tmp[74], World_BlocksSize);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(sc_begin)))) return res;
	if ((res = Stream_Write(stream, World_Blocks, World_BlocksSize))) return res;

	Mem_Copy(tmp, sc_data, sizeof(sc_data));
	{
		Stream_SetU32_BE(&sc_data[7], World_BlocksSize);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(sc_data)))) return res;

	for (i = 0; i < World_BlocksSize; i += sizeof(chunk)) {
		int count = World_BlocksSize - i; count = min(count, sizeof(chunk));
		if ((res = Stream_Write(stream, chunk, count))) return res;
	}
	return Stream_Write(stream, sc_end, sizeof(sc_end));
}
