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

static ReturnCode Map_ReadBlocks(struct Stream* stream) {
	World_BlocksSize = World_Width * World_Length * World_Height;
	World_Blocks = Mem_Alloc(World_BlocksSize, 1, "map blocks");
	return Stream_Read(stream, World_Blocks, World_BlocksSize);
}

static ReturnCode Map_SkipGZipHeader(struct Stream* stream) {
	struct GZipHeader gzHeader;
	GZipHeader_Init(&gzHeader);
	ReturnCode res;

	while (!gzHeader.Done) {
		if ((res = GZipHeader_Read(stream, &gzHeader))) return res;
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------MCSharp level Format---------------------------------------------------*
*#########################################################################################################################*/
#define LVL_CUSTOMTILE 163
#define LVL_CHUNKSIZE 16
UInt8 Lvl_table[256 - BLOCK_CPE_COUNT] = { 0, 0, 0, 0, 39, 36, 36, 10, 46, 21, 22,
22, 22, 22, 4, 0, 22, 21, 0, 22, 23, 24, 22, 26, 27, 28, 30, 31, 32, 33,
34, 35, 36, 22, 20, 49, 45, 1, 4, 0, 9, 11, 4, 19, 5, 17, 10, 49, 20, 1,
18, 12, 5, 25, 46, 44, 17, 49, 20, 1, 18, 12, 5, 25, 36, 34, 0, 9, 11, 46,
44, 0, 9, 11, 8, 10, 22, 27, 22, 8, 10, 28, 17, 49, 20, 1, 18, 12, 5, 25, 46,
44, 11, 9, 0, 9, 11, LVL_CUSTOMTILE, 0, 0, 9, 11, 0, 0, 0, 0, 0, 0, 0, 28, 22, 21,
11, 0, 0, 0, 46, 46, 10, 10, 46, 20, 41, 42, 11, 9, 0, 8, 10, 10, 8, 0, 22, 22,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 10, 0, 0, 0, 0, 0, 22, 22, 42, 3, 2, 29,
47, 0, 0, 0, 0, 0, 27, 46, 48, 24, 22, 36, 34, 8, 10, 21, 29, 22, 10, 22, 22,
41, 19, 35, 21, 29, 49, 34, 16, 41, 0, 22 };

static ReturnCode Lvl_ReadCustomBlocks(struct Stream* stream) {
	Int32 x, y, z, i;
	UInt8 chunk[LVL_CHUNKSIZE * LVL_CHUNKSIZE * LVL_CHUNKSIZE];
	ReturnCode res; UInt8 hasCustom;

	/* skip bounds checks when we know chunk is entirely inside map */
	Int32 adjWidth  = World_Width  & ~0x0F;
	Int32 adjHeight = World_Height & ~0x0F;
	Int32 adjLength = World_Length & ~0x0F;

	for (y = 0; y < World_Height; y += LVL_CHUNKSIZE) {
		for (z = 0; z < World_Length; z += LVL_CHUNKSIZE) {
			for (x = 0; x < World_Width; x += LVL_CHUNKSIZE) {

				if ((res = stream->ReadU8(stream, &hasCustom))) return res;
				if (hasCustom != 1) continue;
				if ((res = Stream_Read(stream, chunk, sizeof(chunk)))) return res;

				Int32 baseIndex = World_Pack(x, y, z);
				if ((x + LVL_CHUNKSIZE) <= adjWidth && (y + LVL_CHUNKSIZE) <= adjHeight && (z + LVL_CHUNKSIZE) <= adjLength) {
					for (i = 0; i < sizeof(chunk); i++) {
						Int32 xx = i & 0xF, yy = (i >> 8) & 0xF, zz = (i >> 4) & 0xF;
						Int32 index = baseIndex + World_Pack(xx, yy, zz);
						World_Blocks[index] = World_Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World_Blocks[index];
					}
				} else {
					for (i = 0; i < sizeof(chunk); i++) {
						Int32 xx = i & 0xF, yy = (i >> 8) & 0xF, zz = (i >> 4) & 0xF;
						if ((x + xx) >= World_Width || (y + yy) >= World_Height || (z + zz) >= World_Length) continue;
						Int32 index = baseIndex + World_Pack(xx, yy, zz);
						World_Blocks[index] = World_Blocks[index] == LVL_CUSTOMTILE ? chunk[i] : World_Blocks[index];
					}
				}
				
			}
		}
	}
	return 0;
}

static void Lvl_ConvertPhysicsBlocks(void) {
	UInt8 conv[256];
	Int32 i;
	for (i = 0; i < BLOCK_CPE_COUNT; i++)
		conv[i] = (UInt8)i;
	for (i = BLOCK_CPE_COUNT; i < 256; i++)
		conv[i] = Lvl_table[i - BLOCK_CPE_COUNT];

	Int32 alignedBlocksSize = World_BlocksSize & ~3;
	/* Bulk convert 4 blocks at once */
	UInt8* blocks = World_Blocks;
	for (i = 0; i < alignedBlocksSize; i += 4) {
		*blocks = conv[*blocks]; blocks++;
		*blocks = conv[*blocks]; blocks++;
		*blocks = conv[*blocks]; blocks++;
		*blocks = conv[*blocks]; blocks++;
	}
	for (i = alignedBlocksSize; i < World_BlocksSize; i++) {
		*blocks = conv[*blocks]; blocks++;
	}
}

ReturnCode Lvl_Load(struct Stream* stream) {
	ReturnCode res = Map_SkipGZipHeader(stream);
	if (res) return res;
	
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	UInt8 header[8 + 2];
	if ((res = Stream_Read(&compStream, header, 8))) return res;
	if (Stream_GetU16_LE(&header[0]) != 1874) return LVL_ERR_VERSION;

	World_Width  = Stream_GetU16_LE(&header[2]);
	World_Length = Stream_GetU16_LE(&header[4]);
	World_Height = Stream_GetU16_LE(&header[6]);

	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;
	struct LocalPlayer* p = &LocalPlayer_Instance;

	p->Spawn.X = Stream_GetU16_LE(&header[0]);
	p->Spawn.Z = Stream_GetU16_LE(&header[2]);
	p->Spawn.Y = Stream_GetU16_LE(&header[4]);
	p->SpawnRotY  = Math_Packed2Deg(header[6]);
	p->SpawnHeadX = Math_Packed2Deg(header[7]);

	/* (2) pervisit, perbuild permissions */
	res = Map_ReadBlocks(&compStream);
	if (res) return res;
	Lvl_ConvertPhysicsBlocks();

	/* 0xBD section type may not be present in older .lvl files */
	UInt8 type; res = compStream.ReadU8(&compStream, &type);
	if (res == ERR_END_OF_STREAM) return 0;

	if (res) return res;
	return type == 0xBD ? Lvl_ReadCustomBlocks(&compStream) : 0;
}


/*########################################################################################################################*
*----------------------------------------------------fCraft map format----------------------------------------------------*
*#########################################################################################################################*/
static ReturnCode Fcm_ReadString(struct Stream* stream) {
	UInt8 data[2];
	ReturnCode res;
	if ((res = Stream_Read(stream, data, sizeof(data)))) return res;

	UInt16 len = Stream_GetU16_LE(data);
	return Stream_Skip(stream, len);
}

ReturnCode Fcm_Load(struct Stream* stream) {
	UInt8 header[(3 * 2) + (3 * 4) + (2 * 1) + (2 * 4) + 16 + 26 + 4];
	ReturnCode res;

	if ((res = Stream_Read(stream, header, 4 + 1))) return res;
	if (Stream_GetU32_LE(&header[0]) != 0x0FC2AF40UL) return FCM_ERR_IDENTIFIER;
	if (header[4] != 13) return FCM_ERR_REVISION;
	
	if ((res = Stream_Read(stream, header, sizeof(header)))) return res;
	World_Width  = Stream_GetU16_LE(&header[0]);
	World_Height = Stream_GetU16_LE(&header[2]);
	World_Length = Stream_GetU16_LE(&header[4]);

	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Spawn.X = ((Int32)Stream_GetU32_LE(&header[ 6])) / 32.0f;
	p->Spawn.Y = ((Int32)Stream_GetU32_LE(&header[10])) / 32.0f;
	p->Spawn.Z = ((Int32)Stream_GetU32_LE(&header[14])) / 32.0f;
	p->SpawnRotY  = Math_Packed2Deg(header[18]);
	p->SpawnHeadX = Math_Packed2Deg(header[19]);

	/* header[20] (4) date modified */
	/* header[24] (4) date created */
	Mem_Copy(&World_Uuid, &header[28], sizeof(World_Uuid));
	/* header[44] (26) layer index */
	Int32 metaSize = (Int32)Stream_GetU32_LE(&header[70]);

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	Int32 i;
	for (i = 0; i < metaSize; i++) {
		if ((res = Fcm_ReadString(&compStream))) return res; /* Group */
		if ((res = Fcm_ReadString(&compStream))) return res; /* Key   */
		if ((res = Fcm_ReadString(&compStream))) return res; /* Value */
	}
	return Map_ReadBlocks(&compStream);
}


/*########################################################################################################################*
*---------------------------------------------------------NBTFile---------------------------------------------------------*
*#########################################################################################################################*/
enum NBT_TAG { 
	NBT_END, NBT_I8, NBT_I16, NBT_I32, NBT_I64, NBT_F32, 
	NBT_R64, NBT_I8S, NBT_STR, NBT_LIST, NBT_DICT, NBT_I32S, 
};

#define NBT_SMALL_SIZE STRING_SIZE
struct NbtTag;
struct NbtTag {
	struct NbtTag* Parent;
	UInt8  TagID;
	char   NameBuffer[NBT_SMALL_SIZE];
	UInt32 NameSize;
	UInt32 DataSize; /* size of data for arrays */

	union {
		UInt8  Value_U8;
		Int16  Value_I16;
		Int32  Value_I32;
		float  Value_F32;
		UInt8  DataSmall[NBT_SMALL_SIZE];
		UInt8* DataBig; /* malloc for big byte arrays */
	};
};

static UInt8 NbtTag_U8(struct NbtTag* tag) {
	if (tag->TagID != NBT_I8) ErrorHandler_Fail("Expected I8 NBT tag");
	return tag->Value_U8;
}

static Int16 NbtTag_I16(struct NbtTag* tag) {
	if (tag->TagID != NBT_I16) ErrorHandler_Fail("Expected I16 NBT tag");
	return tag->Value_I16;
}

static float NbtTag_F32(struct NbtTag* tag) {
	if (tag->TagID != NBT_F32) ErrorHandler_Fail("Expected F32 NBT tag");
	return tag->Value_F32;
}

static UInt8* NbtTag_U8_Array(struct NbtTag* tag, Int32 minSize) {
	if (tag->TagID != NBT_I8S) ErrorHandler_Fail("Expected I8_Array NBT tag");
	if (tag->DataSize < minSize) ErrorHandler_Fail("I8_Array NBT tag too small");

	return tag->DataSize < NBT_SMALL_SIZE ? tag->DataSmall : tag->DataBig;
}

static String NbtTag_String(struct NbtTag* tag) {
	if (tag->TagID != NBT_STR) ErrorHandler_Fail("Expected String NBT tag");
	return String_Init(tag->DataSmall, tag->DataSize, tag->DataSize);
}

static ReturnCode Nbt_ReadString(struct Stream* stream, char* strBuffer, UInt32* strLen) {
	ReturnCode res;
	char nameBuffer[NBT_SMALL_SIZE * 4];
	if ((res = Stream_Read(stream, nameBuffer, 2))) return res;

	UInt16 nameLen = Stream_GetU16_BE(nameBuffer);
	if (nameLen > NBT_SMALL_SIZE * 4) return CW_ERR_STRING_LEN;
	if ((res = Stream_Read(stream, nameBuffer, nameLen))) return res;

	String str = String_Init(strBuffer, 0, NBT_SMALL_SIZE);
	String_DecodeUtf8(&str, nameBuffer, nameLen);
	*strLen = str.length; return 0;
}

typedef bool (*Nbt_Callback)(struct NbtTag* tag);
static ReturnCode Nbt_ReadTag(UInt8 typeId, bool readTagName, struct Stream* stream, struct NbtTag* parent, Nbt_Callback callback) {
	if (typeId == NBT_END) return 0;

	struct NbtTag tag;
	tag.TagID = typeId; tag.Parent = parent;
	tag.NameSize = 0;   tag.DataSize = 0;

	UInt8 childType;
	UInt32 i, count;
	ReturnCode res;
	UInt8 tmp[5];

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
		res = Stream_ReadU32_BE(stream, &tag.Value_I32);
		break;
	case NBT_I64:
	case NBT_R64:
		res = Stream_Skip(stream, 8); 
		break; /* (8) data */

	case NBT_I8S:
		if ((res = Stream_ReadU32_BE(stream, &tag.DataSize))) break;

		if (tag.DataSize < NBT_SMALL_SIZE) {
			res = Stream_Read(stream, tag.DataSmall, tag.DataSize);
		} else {
			tag.DataBig = Mem_Alloc(tag.DataSize, sizeof(UInt8), "NBT data");
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
	bool processed = callback(&tag);
	/* don't leak memory for unprocessed tags */
	if (!processed && tag.DataSize >= NBT_SMALL_SIZE) Mem_Free(tag.DataBig);
	return 0;
}

static bool IsTag(struct NbtTag* tag, const char* tagName) {
	String name = { tag->NameBuffer, tag->NameSize, tag->NameSize };
	return String_CaselessEqualsConst(&name, tagName);
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld format----------------------------------------------------*
*#########################################################################################################################*/
static bool Cw_Callback_1(struct NbtTag* tag) {
	if (IsTag(tag, "X")) { World_Width  = (UInt16)NbtTag_I16(tag); return true; }
	if (IsTag(tag, "Y")) { World_Height = (UInt16)NbtTag_I16(tag); return true; }
	if (IsTag(tag, "Z")) { World_Length = (UInt16)NbtTag_I16(tag); return true; }

	if (IsTag(tag, "UUID")) {
		if (tag->DataSize != sizeof(World_Uuid)) ErrorHandler_Fail("Map UUID must be 16 bytes");
		Mem_Copy(World_Uuid, tag->DataSmall, sizeof(World_Uuid));
		return true;
	}
	if (IsTag(tag, "BlockArray")) {
		World_BlocksSize = tag->DataSize;
		if (tag->DataSize < NBT_SMALL_SIZE) {
			World_Blocks = Mem_Alloc(World_BlocksSize, 1, ".cw map blocks");
			Mem_Copy(World_Blocks, tag->DataSmall, tag->DataSize);
		} else {
			World_Blocks = tag->DataBig;
		}
		return true;
	}
	return false;
}

static bool Cw_Callback_2(struct NbtTag* tag) {
	if (!IsTag(tag->Parent, "Spawn")) return false;

	struct LocalPlayer*p = &LocalPlayer_Instance;
	if (IsTag(tag, "X")) { p->Spawn.X = NbtTag_I16(tag); return true; }
	if (IsTag(tag, "Y")) { p->Spawn.Y = NbtTag_I16(tag); return true; }
	if (IsTag(tag, "Z")) { p->Spawn.Z = NbtTag_I16(tag); return true; }
	if (IsTag(tag, "H")) { p->SpawnRotY  = Math_Deg2Packed(NbtTag_U8(tag)); return true; }
	if (IsTag(tag, "P")) { p->SpawnHeadX = Math_Deg2Packed(NbtTag_U8(tag)); return true; }

	return false;
}

BlockID cw_curID;
Int16 cw_colR, cw_colG, cw_colB;
static PackedCol Cw_ParseCol(PackedCol defValue) {
	Int16 r = cw_colR, g = cw_colG, b = cw_colB;	
	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
		return defValue;
	} else {
		PackedCol col = PACKEDCOL_CONST((UInt8)r, (UInt8)g, (UInt8)b, 255);
		return col;		
	}
}

static bool Cw_Callback_4(struct NbtTag* tag) {
	if (!IsTag(tag->Parent->Parent, "CPE")) return false;
	if (!IsTag(tag->Parent->Parent->Parent, "Metadata")) return false;
	struct LocalPlayer*p = &LocalPlayer_Instance;

	if (IsTag(tag->Parent, "ClickDistance")) {
		if (IsTag(tag, "Distance")) { p->ReachDistance = NbtTag_I16(tag) / 32.0f; return true; }
	}
	if (IsTag(tag->Parent, "EnvWeatherType")) {
		if (IsTag(tag, "WeatherType")) { Env_SetWeather(NbtTag_U8(tag)); return true; }
	}

	if (IsTag(tag->Parent, "EnvMapAppearance")) {
		if (IsTag(tag, "SideBlock")) { Env_SetSidesBlock(NbtTag_U8(tag));  return true; }
		if (IsTag(tag, "EdgeBlock")) { Env_SetEdgeBlock(NbtTag_U8(tag));   return true; }
		if (IsTag(tag, "SideLevel")) { Env_SetEdgeHeight(NbtTag_I16(tag)); return true; }

		if (IsTag(tag, "TextureURL")) {
			String url = NbtTag_String(tag);
			if (Game_AllowServerTextures && url.length) {
				ServerConnection_RetrieveTexturePack(&url);
			}
			return true;
		}
	}

	/* Callback for compound tag is called after all its children have been processed */
	if (IsTag(tag->Parent, "EnvColors")) {
		if (IsTag(tag, "Sky")) {
			Env_SetSkyCol(Cw_ParseCol(Env_DefaultSkyCol)); return true;
		} else if (IsTag(tag, "Cloud")) {
			Env_SetCloudsCol(Cw_ParseCol(Env_DefaultCloudsCol)); return true;
		} else if (IsTag(tag, "Fog")) {
			Env_SetFogCol(Cw_ParseCol(Env_DefaultFogCol)); return true;
		} else if (IsTag(tag, "Sunlight")) {
			Env_SetSunCol(Cw_ParseCol(Env_DefaultSunCol)); return true;
		} else if (IsTag(tag, "Ambient")) {
			Env_SetShadowCol(Cw_ParseCol(Env_DefaultShadowCol)); return true;
		}
	}

	if (IsTag(tag->Parent, "BlockDefinitions")) {
		String tagName = { tag->NameBuffer, tag->NameSize, tag->NameSize };
		String blockStr = String_FromConst("Block");
		if (!String_CaselessStarts(&tagName, &blockStr)) return false;
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
		return true;
	}
	return false;
}

static bool Cw_Callback_5(struct NbtTag* tag) {
	if (!IsTag(tag->Parent->Parent->Parent, "CPE")) return false;
	if (!IsTag(tag->Parent->Parent->Parent->Parent, "Metadata")) return false;

	if (IsTag(tag->Parent->Parent, "EnvColors")) {
		if (IsTag(tag, "R")) { cw_colR = NbtTag_I16(tag); return true; }
		if (IsTag(tag, "G")) { cw_colG = NbtTag_I16(tag); return true; }
		if (IsTag(tag, "B")) { cw_colB = NbtTag_I16(tag); return true; }
	}

	if (IsTag(tag->Parent->Parent, "BlockDefinitions") && Game_AllowCustomBlocks) {
		BlockID id = cw_curID;
		if (IsTag(tag, "ID"))             { cw_curID = NbtTag_U8(tag); return true; }
		if (IsTag(tag, "CollideType"))    { Block_SetCollide(id, NbtTag_U8(tag)); return true; }
		if (IsTag(tag, "Speed"))          { Block_SpeedMultiplier[id] = NbtTag_F32(tag); return true; }
		if (IsTag(tag, "TransmitsLight")) { Block_BlocksLight[id] = NbtTag_U8(tag) == 0; return true; }
		if (IsTag(tag, "FullBright"))     { Block_FullBright[id] = NbtTag_U8(tag) != 0; return true; }
		if (IsTag(tag, "BlockDraw"))      { Block_Draw[id] = NbtTag_U8(tag); return true; }
		if (IsTag(tag, "Shape"))          { Block_SpriteOffset[id] = NbtTag_U8(tag); return true; }

		if (IsTag(tag, "Name")) {
			String name = NbtTag_String(tag);
			Block_SetName(id, &name);
			return true;
		}

		if (IsTag(tag, "Textures")) {
			UInt8* tex = NbtTag_U8_Array(tag, 6);
			Block_SetTex(tex[0], FACE_YMAX, id);
			Block_SetTex(tex[1], FACE_YMIN, id);
			Block_SetTex(tex[2], FACE_XMIN, id);
			Block_SetTex(tex[3], FACE_XMAX, id);
			Block_SetTex(tex[4], FACE_ZMIN, id);
			Block_SetTex(tex[5], FACE_ZMAX, id);
			return true;
		}
		
		if (IsTag(tag, "WalkSound")) {
			UInt8 sound = NbtTag_U8(tag);
			Block_DigSounds[id]  = sound;
			Block_StepSounds[id] = sound;
			if (sound == SOUND_GLASS) Block_StepSounds[id] = SOUND_STONE;
			return true;
		}

		if (IsTag(tag, "Fog")) {
			UInt8* fog = NbtTag_U8_Array(tag, 4);
			Block_FogDensity[id] = (fog[0] + 1) / 128.0f;
			/* Fix for older ClassicalSharp versions which saved wrong fog density value */
			if (fog[0] == 0xFF) Block_FogDensity[id] = 0.0f;
 
			Block_FogCol[id].R = fog[1];
			Block_FogCol[id].G = fog[2];
			Block_FogCol[id].B = fog[3];
			Block_FogCol[id].A = 255;
			return true;
		}

		if (IsTag(tag, "Coords")) {
			UInt8* coords = NbtTag_U8_Array(tag, 6);
			Block_MinBB[id].X = coords[0] / 16.0f; Block_MaxBB[id].X = coords[3] / 16.0f;
			Block_MinBB[id].Y = coords[1] / 16.0f; Block_MaxBB[id].Y = coords[4] / 16.0f;
			Block_MinBB[id].Z = coords[2] / 16.0f; Block_MaxBB[id].Z = coords[5] / 16.0f;
			return true;
		}
	}
	return false;
}

static bool Cw_Callback(struct NbtTag* tag) {
	UInt32 depth = 0;
	struct NbtTag* tmp = tag->Parent;
	while (tmp) { depth++; tmp = tmp->Parent; }

	switch (depth) {
	case 1: return Cw_Callback_1(tag);
	case 2: return Cw_Callback_2(tag);
	case 4: return Cw_Callback_4(tag);
	case 5: return Cw_Callback_5(tag);
	}
	return false;

	/* ClassicWorld -> Metadata -> CPE -> ExtName -> [values]
	        0             1         2        3          4   */
}

ReturnCode Cw_Load(struct Stream* stream) {
	ReturnCode res = Map_SkipGZipHeader(stream);
	if (res) return res;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	UInt8 tag;
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
	TC_STRING = 0x74, TC_ARRAY = 0x75, TC_ENDBLOCKDATA = 0x78,
};

enum JFieldType {
	JFIELD_I8 = 'B', JFIELD_F32 = 'F', JFIELD_I32 = 'I', JFIELD_I64 = 'J',
	JFIELD_BOOL = 'Z', JFIELD_ARRAY = '[', JFIELD_OBJECT = 'L',
};

#define JNAME_SIZE 48
struct JFieldDesc {
	UInt8 Type;
	char FieldName[JNAME_SIZE];
	union {
		UInt8 Value_U8;
		Int32 Value_I32;
		float Value_F32;
		struct { UInt8* Value_Ptr; UInt32 Value_Size; };
	};
};

struct JClassDesc {
	char ClassName[JNAME_SIZE];
	UInt16 FieldsCount;
	struct JFieldDesc Fields[22];
};

static ReturnCode Dat_ReadString(struct Stream* stream, char* buffer) {
	ReturnCode res;
	if ((res = Stream_Read(stream, buffer, 2))) return res;
	UInt16 len = Stream_GetU16_BE(buffer);

	Mem_Set(buffer, 0, JNAME_SIZE);
	if (len > JNAME_SIZE) return DAT_ERR_JSTRING_LEN;
	return Stream_Read(stream, buffer, len);
}

static ReturnCode Dat_ReadFieldDesc(struct Stream* stream, struct JFieldDesc* desc) {
	ReturnCode res;
	if ((res = stream->ReadU8(stream, &desc->Type)))     return res;
	if ((res = Dat_ReadString(stream, desc->FieldName))) return res;

	if (desc->Type == JFIELD_ARRAY || desc->Type == JFIELD_OBJECT) {
		UInt8 typeCode;
		if ((res = stream->ReadU8(stream, &typeCode))) return res;

		if (typeCode == TC_STRING) {
			char className1[JNAME_SIZE];
			return Dat_ReadString(stream, className1);
		} else if (typeCode == TC_REFERENCE) {
			return Stream_Skip(stream, 4); /* (4) handle */
		} else {
			return DAT_ERR_JFIELD_CLASS_NAME;
		}
	}
	return 0;
}

static ReturnCode Dat_ReadClassDesc(struct Stream* stream, struct JClassDesc* desc) {
	ReturnCode res;
	UInt8 typeCode;
	UInt8 tmp[2];

	if ((res = stream->ReadU8(stream, &typeCode))) return res;
	if (typeCode == TC_NULL) { desc->ClassName[0] = '\0'; desc->FieldsCount = 0; return 0; }
	if (typeCode != TC_CLASSDESC) return DAT_ERR_JCLASS_TYPE;

	if ((res = Dat_ReadString(stream, desc->ClassName))) return res;
	if ((res = Stream_Skip(stream, 9))) return res; /* (8) serial version UID, (1) flags */

	if ((res = Stream_Read(stream, tmp, 2))) return res;
	desc->FieldsCount = Stream_GetU16_BE(tmp);
	if (desc->FieldsCount > Array_Elems(desc->Fields)) return DAT_ERR_JCLASS_FIELDS;

	Int32 i;
	for (i = 0; i < desc->FieldsCount; i++) {
		if ((res = Dat_ReadFieldDesc(stream, &desc->Fields[i]))) return res;
	}

	if ((res = stream->ReadU8(stream, &typeCode))) return res;
	if (typeCode != TC_ENDBLOCKDATA) return DAT_ERR_JCLASS_ANNOTATION;

	struct JClassDesc superClassDesc;
	return Dat_ReadClassDesc(stream, &superClassDesc);
}

static ReturnCode Dat_ReadFieldData(struct Stream* stream, struct JFieldDesc* field) {
	ReturnCode res;
	UInt8 typeCode;
	UInt32 count;

	switch (field->Type) {
	case JFIELD_I8:
	case JFIELD_BOOL:
		return stream->ReadU8(stream, &field->Value_U8);
	case JFIELD_F32:
	case JFIELD_I32:
		return Stream_ReadU32_BE(stream, &field->Value_I32);
	case JFIELD_I64:
		return Stream_Skip(stream, 8); /* (8) data */

	case JFIELD_OBJECT: {
		/* Luckily for us, we only have to account for blockMap object */
		/* Other objects (e.g. player) are stored after the fields we actually care about, so ignore them */
		String fieldName = String_FromRawArray(field->FieldName);
		if (!String_CaselessEqualsConst(&fieldName, "blockMap")) return 0;
		if ((res = stream->ReadU8(stream, &typeCode))) return res;

		/* Skip all blockMap data with awful hacks */
		/* These offsets were based on server_level.dat map from original minecraft classic server */
		if (typeCode == TC_OBJECT) {
			if ((res = Stream_Skip(stream, 315)))          return res;
			if ((res = Stream_ReadU32_BE(stream, &count))) return res;

			if ((res = Stream_Skip(stream, 17 * count))) return res;
			if ((res = Stream_Skip(stream, 152)))        return res;
		} else if (typeCode != TC_NULL) {
			/* WoM maps have this field as null, which makes things easier for us */
			return DAT_ERR_JOBJECT_TYPE;
		}
	} break;

	case JFIELD_ARRAY: {
		if ((res = stream->ReadU8(stream, &typeCode))) return res;
		if (typeCode == TC_NULL) break;
		if (typeCode != TC_ARRAY) return DAT_ERR_JARRAY_TYPE;

		struct JClassDesc arrayClassDesc;
		if ((res = Dat_ReadClassDesc(stream, &arrayClassDesc))) return res;
		if (arrayClassDesc.ClassName[1] != JFIELD_I8) return DAT_ERR_JARRAY_CONTENT;

		if ((res = Stream_ReadU32_BE(stream, &count))) return res;
		field->Value_Size = count;

		field->Value_Ptr = Mem_Alloc(count, sizeof(UInt8), ".dat map blocks");
		res = Stream_Read(stream, field->Value_Ptr, count);	
		if (res) { Mem_Free(field->Value_Ptr); return res; }
	} break;
	}
	return 0;
}

static Int32 Dat_I32(struct JFieldDesc* field) {
	if (field->Type != JFIELD_I32) ErrorHandler_Fail("Field type must be Int32");
	return field->Value_I32;
}

ReturnCode Dat_Load(struct Stream* stream) {
	ReturnCode res = Map_SkipGZipHeader(stream);
	if (res) return res;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	UInt8 header[4 + 1 + 2 * 2];
	if ((res = Stream_Read(&compStream, header, sizeof(header)))) return res;

	/* .dat header */
	if (Stream_GetU32_BE(&header[0]) != 0x271BB788) return DAT_ERR_IDENTIFIER;
	if (header[4] != 0x02) return DAT_ERR_VERSION;

	/* Java seralisation headers */
	if (Stream_GetU16_BE(&header[5]) != 0xACED) return DAT_ERR_JIDENTIFIER;
	if (Stream_GetU16_BE(&header[7]) != 0x0005) return DAT_ERR_JVERSION;

	UInt8 typeCode;
	if ((res = compStream.ReadU8(&compStream, &typeCode))) return res;
	if (typeCode != TC_OBJECT) return DAT_ERR_ROOT_TYPE;

	struct JClassDesc obj; 
	if ((res = Dat_ReadClassDesc(&compStream, &obj))) return res;

	Int32 i;
	Vector3* spawn = &LocalPlayer_Instance.Spawn;
	for (i = 0; i < obj.FieldsCount; i++) {
		struct JFieldDesc* field = &obj.Fields[i];

		if ((res = Dat_ReadFieldData(&compStream, field))) return res;
		String fieldName = String_FromRawArray(field->FieldName);

		if (String_CaselessEqualsConst(&fieldName, "width")) {
			World_Width  = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "height")) {
			World_Length = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "depth")) {
			World_Height = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "blocks")) {
			if (field->Type != JFIELD_ARRAY) ErrorHandler_Fail("Blocks field must be Array");
			World_Blocks     = field->Value_Ptr;
			World_BlocksSize = field->Value_Size;
		} else if (String_CaselessEqualsConst(&fieldName, "xSpawn")) {
			spawn->X = (float)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "ySpawn")) {
			spawn->Y = (float)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "zSpawn")) {
			spawn->Z = (float)Dat_I32(field);
		}
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld export----------------------------------------------------*
*#########################################################################################################################*/
#define CW_META_VERSION 'E','x','t','e','n','s','i','o','n','V','e','r','s','i','o','n'
#define CW_META_RGB NBT_I16,0,1,'R',0,0,  NBT_I16,0,1,'G',0,0,  NBT_I16,0,1,'B',0,0,
static Int32 Cw_WriteEndString(UInt8* data, const String* text) {
	Int32 i, len = 0;
	UInt8* cur = data + 2;

	for (i = 0; i < text->length; i++) {
		UInt16 codepoint = Convert_CP437ToUnicode(text->buffer[i]);
		Int32 bytes = Stream_WriteUtf8(cur, codepoint);
		len += bytes; cur += bytes;
	}

	Stream_SetU16_BE(data, len);
	*cur = NBT_END;
	return len + 1;
}

UInt8 cw_begin[131] = {
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
UInt8 cw_meta_cpe[395] = {
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
UInt8 cw_meta_defs[19] = {
			NBT_DICT, 0,16, 'B','l','o','c','k','D','e','f','i','n','i','t','i','o','n','s',
};
UInt8 cw_meta_def[171] = {
				NBT_DICT, 0,5,  'B','l','o','c','k',
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
UInt8 cw_end[4] = {
			NBT_END,
		NBT_END,
	NBT_END,
NBT_END,
};

static ReturnCode Cw_WriteBockDef(struct Stream* stream, Int32 b) {
	UInt8 tmp[512];
	bool sprite = Block_Draw[b] == DRAW_SPRITE;

	Mem_Copy(tmp, cw_meta_def, sizeof(cw_meta_def));
	{
		tmp[13] = b;
		tmp[28] = Block_Collide[b];
		union IntAndFloat speed; speed.f = Block_SpeedMultiplier[b];
		Stream_SetU32_BE(&tmp[37], speed.u);

		tmp[56] = (UInt8)Block_GetTexLoc(b, FACE_YMAX);
		tmp[57] = (UInt8)Block_GetTexLoc(b, FACE_YMIN);
		tmp[58] = (UInt8)Block_GetTexLoc(b, FACE_XMIN);
		tmp[59] = (UInt8)Block_GetTexLoc(b, FACE_XMAX);
		tmp[60] = (UInt8)Block_GetTexLoc(b, FACE_ZMIN);
		tmp[61] = (UInt8)Block_GetTexLoc(b, FACE_ZMAX);

		tmp[79]  = Block_BlocksLight[b] ? 0 : 1;
		tmp[92]  = Block_DigSounds[b];
		tmp[106] = Block_FullBright[b] ? 1 : 0;
		tmp[115] = sprite ? 0 : (UInt8)(Block_MaxBB[b].Y * 16);
		tmp[128] = sprite ? Block_SpriteOffset[b] : Block_Draw[b];

		UInt8 fog = (UInt8)(128 * Block_FogDensity[b] - 1);
		PackedCol col = Block_FogCol[b];
		tmp[139] = Block_FogDensity[b] ? fog : 0;
		tmp[140] = col.R; tmp[141] = col.G; tmp[142] = col.B;

		Vector3 minBB = Block_MinBB[b], maxBB = Block_MaxBB[b];
		tmp[156] = (UInt8)(minBB.X * 16); tmp[157] = (UInt8)(minBB.Y * 16); tmp[158] = (UInt8)(minBB.Z * 16);
		tmp[159] = (UInt8)(maxBB.X * 16); tmp[160] = (UInt8)(maxBB.Y * 16); tmp[161] = (UInt8)(maxBB.Z * 16);
	}

	String name = Block_UNSAFE_GetName(b);
	Int32 len   = Cw_WriteEndString(&tmp[169], &name);
	return Stream_Write(stream, tmp, sizeof(cw_meta_def) + len);
}

ReturnCode Cw_Save(struct Stream* stream) {
	UInt8 tmp[768];
	PackedCol col;
	ReturnCode res;

	Mem_Copy(tmp, cw_begin, sizeof(cw_begin));
	{
		Mem_Copy(&tmp[43], World_Uuid, sizeof(World_Uuid));
		Stream_SetU16_BE(&tmp[63], World_Width);
		Stream_SetU16_BE(&tmp[69], World_Height);
		Stream_SetU16_BE(&tmp[75], World_Length);
		Stream_SetU32_BE(&tmp[127], World_BlocksSize);

		struct LocalPlayer* p = &LocalPlayer_Instance;
		Vector3 spawn = p->Spawn; /* TODO: Maybe keep real spawn too? */
		Stream_SetU16_BE(&tmp[89],  (UInt16)spawn.X);
		Stream_SetU16_BE(&tmp[95],  (UInt16)spawn.Y);
		Stream_SetU16_BE(&tmp[101], (UInt16)spawn.Z);
		tmp[107] = Math_Deg2Packed(p->SpawnRotY);
		tmp[112] = Math_Deg2Packed(p->SpawnHeadX);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(cw_begin)))) return res;
	if ((res = Stream_Write(stream, World_Blocks, World_BlocksSize))) return res;

	Mem_Copy(tmp, cw_meta_cpe, sizeof(cw_meta_cpe));
	{
		Stream_SetU16_BE(&tmp[67], (UInt16)(LocalPlayer_Instance.ReachDistance * 32));
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
	Int32 b, len = Cw_WriteEndString(&tmp[393], &World_TextureUrl);
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

UInt8 sc_begin[78] = {
NBT_DICT, 0,9, 'S','c','h','e','m','a','t','i','c',
	NBT_STR,  0,9,  'M','a','t','e','r','i','a','l','s', 0,7, 'C','l','a','s','s','i','c',
	NBT_I16,  0,5,  'W','i','d','t','h',                 0,0,
	NBT_I16,  0,6,  'H','e','i','g','h','t',             0,0,
	NBT_I16,  0,6,  'L','e','n','g','t','h',             0,0,
	NBT_I8S,  0,6,  'B','l','o','c','k','s',             0,0,0,0,
};
UInt8 sc_data[11] = {
	NBT_I8S,  0,4,  'D','a','t','a',                     0,0,0,0,
};
UInt8 sc_end[37] = {
	NBT_LIST, 0,8,  'E','n','t','i','t','i','e','s',                 NBT_DICT, 0,0,0,0,
	NBT_LIST, 0,12, 'T','i','l','e','E','n','t','i','t','i','e','s', NBT_DICT, 0,0,0,0,
NBT_END,
};

ReturnCode Schematic_Save(struct Stream* stream) {
	UInt8 tmp[256];
	ReturnCode res;

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
		Stream_SetU32_BE(&tmp[7], World_BlocksSize);
	}
	if ((res = Stream_Write(stream, tmp, sizeof(sc_data)))) return res;

	UInt8 chunk[8192] = { 0 };
	Int32 i;
	for (i = 0; i < World_BlocksSize; i += sizeof(chunk)) {
		Int32 count = World_BlocksSize - i; count = min(count, sizeof(chunk));
		if ((res = Stream_Write(stream, chunk, count))) return res;
	}
	return Stream_Write(stream, sc_end, sizeof(sc_end));
}
