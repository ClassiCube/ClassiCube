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

static void Map_ReadBlocks(struct Stream* stream) {
	World_BlocksSize = World_Width * World_Length * World_Height;
	World_Blocks = Mem_Alloc(World_BlocksSize, sizeof(BlockID), "map blocks for load");
	Stream_Read(stream, World_Blocks, World_BlocksSize);
}

static ReturnCode Map_SkipGZipHeader(struct Stream* stream) {
	struct GZipHeader gzHeader;
	GZipHeader_Init(&gzHeader);

	while (!gzHeader.Done) {
		ReturnCode result = GZipHeader_Read(stream, &gzHeader);
		if (result) return result;
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
	ReturnCode result; UInt8 hasCustom;

	/* skip bounds checks when we know chunk is entirely inside map */
	Int32 adjWidth  = World_Width  & ~0x0F;
	Int32 adjHeight = World_Height & ~0x0F;
	Int32 adjLength = World_Length & ~0x0F;

	for (y = 0; y < World_Height; y += LVL_CHUNKSIZE) {
		for (z = 0; z < World_Length; z += LVL_CHUNKSIZE) {
			for (x = 0; x < World_Width; x += LVL_CHUNKSIZE) {
				result = stream->ReadU8(stream, &hasCustom);
				if (result) return result;

				if (hasCustom != 1) continue;
				Stream_Read(stream, chunk, sizeof(chunk));

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
	ReturnCode result = Map_SkipGZipHeader(stream);
	if (result) return result;
	
	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	UInt8 header[8 + 2];
	Stream_Read(&compStream, header, 8);
	if (Stream_GetU16_LE(&header[0]) != 1874) return LVL_ERR_VERSION;

	World_Width  = Stream_GetU16_LE(&header[2]);
	World_Length = Stream_GetU16_LE(&header[4]);
	World_Height = Stream_GetU16_LE(&header[6]);
	Stream_Read(&compStream, header, sizeof(header));

	struct LocalPlayer* p = &LocalPlayer_Instance;
	p->Spawn.X = Stream_GetU16_LE(&header[0]);
	p->Spawn.Z = Stream_GetU16_LE(&header[2]);
	p->Spawn.Y = Stream_GetU16_LE(&header[4]);
	p->SpawnRotY  = Math_Packed2Deg(header[6]);
	p->SpawnHeadX = Math_Packed2Deg(header[7]);

	/* (2) pervisit, perbuild permissions */
	Map_ReadBlocks(&compStream);
	Lvl_ConvertPhysicsBlocks();

	/* 0xBD section type may not be present in older .lvl files */
	UInt8 type; result = compStream.ReadU8(&compStream, &type);
	if (result == ERR_END_OF_STREAM) return 0;

	if (result) return result;
	return type == 0xBD ? Lvl_ReadCustomBlocks(&compStream) : 0;
}


/*########################################################################################################################*
*----------------------------------------------------fCraft map format----------------------------------------------------*
*#########################################################################################################################*/
static void Fcm_ReadString(struct Stream* stream) {
	Stream_Skip(stream, Stream_ReadU16_LE(stream));
}

ReturnCode Fcm_Load(struct Stream* stream) {
	UInt8 header[(3 * 2) + (3 * 4) + (2 * 1) + (2 * 4) + 16 + 26 + 4];

	Stream_Read(stream, header, 4 + 1);
	if (Stream_GetU32_LE(&header[0]) != 0x0FC2AF40UL) return FCM_ERR_IDENTIFIER;
	if (header[4] != 13) return FCM_ERR_REVISION;
	
	Stream_Read(stream, header, sizeof(header));
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
		Fcm_ReadString(&compStream); /* Group */
		Fcm_ReadString(&compStream); /* Key   */
		Fcm_ReadString(&compStream); /* Value */
	}
	Map_ReadBlocks(&compStream);
	return 0;
}


/*########################################################################################################################*
*---------------------------------------------------------NBTFile---------------------------------------------------------*
*#########################################################################################################################*/
enum NBT_TAG { 
	NBT_TAG_END, NBT_TAG_INT8, NBT_TAG_INT16, NBT_TAG_INT32, NBT_TAG_INT64, NBT_TAG_REAL32, NBT_TAG_REAL64, 
	NBT_TAG_INT8_ARRAY, NBT_TAG_STRING, NBT_TAG_LIST, NBT_TAG_COMPOUND, NBT_TAG_INT32_ARRAY, 
};

#define NBT_SMALL_SIZE STRING_SIZE
struct NbtTag;
struct NbtTag {
	struct NbtTag* Parent;
	UInt8  TagID;
	UChar  NameBuffer[String_BufferSize(NBT_SMALL_SIZE)];
	UInt32 NameSize;
	UInt32 DataSize; /* size of data for arrays */

	union {
		UInt8  Value_U8;
		Int16  Value_I16;
		Int32  Value_I32;
		Real32 Value_R32;
		UInt8  DataSmall[String_BufferSize(NBT_SMALL_SIZE)];
		UInt8* DataBig; /* malloc for big byte arrays */
	};
};

static UInt8 NbtTag_U8(struct NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT8) ErrorHandler_Fail("Expected I8 NBT tag");
	return tag->Value_U8;
}

static Int16 NbtTag_I16(struct NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT16) ErrorHandler_Fail("Expected I16 NBT tag");
	return tag->Value_I16;
}

static Int32 NbtTag_I32(struct NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT32) ErrorHandler_Fail("Expected I32 NBT tag");
	return tag->Value_I32;
}

static Real32 NbtTag_R32(struct NbtTag* tag) {
	if (tag->TagID != NBT_TAG_REAL32) ErrorHandler_Fail("Expected R32 NBT tag");
	return tag->Value_R32;
}

static UInt8* NbtTag_U8_Array(struct NbtTag* tag, Int32 minSize) {
	if (tag->TagID != NBT_TAG_INT8_ARRAY) ErrorHandler_Fail("Expected I8_Array NBT tag");
	if (tag->DataSize < minSize) ErrorHandler_Fail("I8_Array NBT tag too small");

	return tag->DataSize < NBT_SMALL_SIZE ? tag->DataSmall : tag->DataBig;
}

static String NbtTag_String(struct NbtTag* tag) {
	if (tag->TagID != NBT_TAG_STRING) ErrorHandler_Fail("Expected String NBT tag");
	return String_Init(tag->DataSmall, tag->DataSize, tag->DataSize);
}

static UInt32 Nbt_ReadString(struct Stream* stream, UChar* strBuffer) {
	UInt16 nameLen = Stream_ReadU16_BE(stream);
	if (nameLen > NBT_SMALL_SIZE * 4) ErrorHandler_Fail("NBT String too long");
	UChar nameBuffer[NBT_SMALL_SIZE * 4];
	Stream_Read(stream, nameBuffer, nameLen);

	String str = String_Init(strBuffer, 0, NBT_SMALL_SIZE);
	String_DecodeUtf8(&str, nameBuffer, nameLen);
	return str.length;
}

typedef bool (*Nbt_Callback)(struct NbtTag* tag);
static void Nbt_ReadTag(UInt8 typeId, bool readTagName, struct Stream* stream, struct NbtTag* parent, Nbt_Callback callback) {
	if (typeId == NBT_TAG_END) return;

	struct NbtTag tag;
	tag.TagID = typeId;
	tag.Parent = parent;
	tag.NameSize = readTagName ? Nbt_ReadString(stream, tag.NameBuffer) : 0;
	tag.DataSize = 0;

	UInt8 childTagId;
	UInt32 i, count;

	switch (typeId) {
	case NBT_TAG_INT8:
		tag.Value_U8 = Stream_ReadU8(stream); break;
	case NBT_TAG_INT16:
		tag.Value_I16 = Stream_ReadI16_BE(stream); break;
	case NBT_TAG_INT32:
		tag.Value_I32 = Stream_ReadI32_BE(stream); break;
	case NBT_TAG_INT64:
		Stream_Skip(stream, 8); break; /* (8) data */
	case NBT_TAG_REAL32:
		/* TODO: Is this union abuse even legal */
		tag.Value_I32 = Stream_ReadI32_BE(stream); break;
	case NBT_TAG_REAL64:
		Stream_Skip(stream, 8); break; /* (8) data */

	case NBT_TAG_INT8_ARRAY:
		count = Stream_ReadU32_BE(stream); 
		tag.DataSize = count;

		if (count < NBT_SMALL_SIZE) {
			Stream_Read(stream, tag.DataSmall, count);
		} else {
			tag.DataBig = Mem_Alloc(count, sizeof(UInt8), "NBT tag data");
			Stream_Read(stream, tag.DataBig, count);
		}
		break;

	case NBT_TAG_STRING:
		tag.DataSize = Nbt_ReadString(stream, tag.DataSmall);
		break;

	case NBT_TAG_LIST:
		childTagId = Stream_ReadU8(stream);
		count = Stream_ReadU32_BE(stream);
		for (i = 0; i < count; i++) {
			Nbt_ReadTag(childTagId, false, stream, &tag, callback);
		}
		break;

	case NBT_TAG_COMPOUND:
		while ((childTagId = Stream_ReadU8(stream)) != NBT_TAG_END) {
			Nbt_ReadTag(childTagId, true, stream, &tag, callback);
		} 
		break;

	case NBT_TAG_INT32_ARRAY:
		ErrorHandler_Fail("Nbt Tag Int32_Array not supported");
		break;

	default:
		ErrorHandler_Fail("Unrecognised NBT tag");
	}

	bool processed = callback(&tag);
	/* don't leak memory for unprocessed tags */
	if (!processed && tag.DataSize >= NBT_SMALL_SIZE) Mem_Free(&tag.DataBig);
}

static bool IsTag(struct NbtTag* tag, const UChar* tagName) {
	String name = { tag->NameBuffer, tag->NameSize, tag->NameSize };
	return String_CaselessEqualsConst(&name, tagName);
}


#define Nbt_WriteU8(stream, value)  Stream_WriteU8(stream, value)
#define Nbt_WriteI16(stream, value) Stream_WriteI16_BE(stream, value)
#define Nbt_WriteI32(stream, value) Stream_WriteI32_BE(stream, value)
#define Nbt_WriteU8_Array(stream, data, len) Nbt_WriteI32(stream, len); Stream_Write(stream, data, len)

static void Nbt_WriteString(struct Stream* stream, STRING_PURE String* text) {
	if (text->length > NBT_SMALL_SIZE) ErrorHandler_Fail("NBT String too long");
	UInt8 buffer[NBT_SMALL_SIZE * 3];
	Int32 i, len = 0;

	for (i = 0; i < text->length; i++) {
		UInt8* cur = buffer + len;
		UInt16 codepoint = Convert_CP437ToUnicode(text->buffer[i]);
		len += Stream_WriteUtf8(cur, codepoint);
	}

	Nbt_WriteI16(stream, len);
	Stream_Write(stream, buffer, len);
}

static void Nbt_WriteTag(struct Stream* stream, UInt8 tagType, const UInt8* tagName) {
	Nbt_WriteU8(stream, tagType);
	String str = String_FromReadonly(tagName);
	Nbt_WriteString(stream, &str);
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
			World_Blocks = Mem_Alloc(World_BlocksSize, sizeof(UInt8), ".cw map blocks");
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
		if (IsTag(tag, "WeatherType")) { WorldEnv_SetWeather(NbtTag_U8(tag)); return true; }
	}

	if (IsTag(tag->Parent, "EnvMapAppearance")) {
		if (IsTag(tag, "SideBlock")) { WorldEnv_SetSidesBlock(NbtTag_U8(tag));  return true; }
		if (IsTag(tag, "EdgeBlock")) { WorldEnv_SetEdgeBlock(NbtTag_U8(tag));   return true; }
		if (IsTag(tag, "SideLevel")) { WorldEnv_SetEdgeHeight(NbtTag_I16(tag)); return true; }

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
			WorldEnv_SetSkyCol(Cw_ParseCol(WorldEnv_DefaultSkyCol)); return true;
		} else if (IsTag(tag, "Cloud")) {
			WorldEnv_SetCloudsCol(Cw_ParseCol(WorldEnv_DefaultCloudsCol)); return true;
		} else if (IsTag(tag, "Fog")) {
			WorldEnv_SetFogCol(Cw_ParseCol(WorldEnv_DefaultFogCol)); return true;
		} else if (IsTag(tag, "Sunlight")) {
			WorldEnv_SetSunCol(Cw_ParseCol(WorldEnv_DefaultSunCol)); return true;
		} else if (IsTag(tag, "Ambient")) {
			WorldEnv_SetShadowCol(Cw_ParseCol(WorldEnv_DefaultShadowCol)); return true;
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
		if (IsTag(tag, "Speed"))          { Block_SpeedMultiplier[id] = NbtTag_R32(tag); return true; }
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
	ReturnCode result = Map_SkipGZipHeader(stream);
	if (result) return result;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	if (Stream_ReadU8(&compStream) != NBT_TAG_COMPOUND) {
		ErrorHandler_Fail("NBT file most start with Compound Tag");
	}
	Nbt_ReadTag(NBT_TAG_COMPOUND, true, &compStream, NULL, Cw_Callback);

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
	JFIELD_INT8 = 'B', JFIELD_REAL32 = 'F', JFIELD_INT32 = 'I', JFIELD_INT64 = 'J',
	JFIELD_BOOL = 'Z', JFIELD_ARRAY = '[', JFIELD_OBJECT = 'L',
};

#define JNAME_SIZE 48
struct JFieldDesc {
	UInt8 Type;
	UChar FieldName[String_BufferSize(JNAME_SIZE)];
	union {
		UInt8  Value_U8;
		Int32  Value_I32;
		Real32 Value_R32;
		struct { UInt8* Value_Ptr; UInt32 Value_Size; };
	};
};

struct JClassDesc {
	UChar ClassName[String_BufferSize(JNAME_SIZE)];
	UInt16 FieldsCount;
	struct JFieldDesc Fields[22];
};

static void Dat_ReadString(struct Stream* stream, UInt8* buffer) {
	Mem_Set(buffer, 0, JNAME_SIZE);
	UInt16 len = Stream_ReadU16_BE(stream);

	if (len > JNAME_SIZE) ErrorHandler_Fail("Dat string too long");
	Stream_Read(stream, buffer, len);
}

static void Dat_ReadFieldDesc(struct Stream* stream, struct JFieldDesc* desc) {
	desc->Type = Stream_ReadU8(stream);
	Dat_ReadString(stream, desc->FieldName);

	if (desc->Type == JFIELD_ARRAY || desc->Type == JFIELD_OBJECT) {
		UInt8 typeCode = Stream_ReadU8(stream);
		if (typeCode == TC_STRING) {
			UChar className1[String_BufferSize(JNAME_SIZE)];
			Dat_ReadString(stream, className1);
		} else if (typeCode == TC_REFERENCE) {
			Stream_Skip(stream, 4); /* (4) handle */
		} else {
			ErrorHandler_Fail("Unsupported type code in FieldDesc class name");
		}
	}
}

static void Dat_ReadClassDesc(struct Stream* stream, struct JClassDesc* desc) {
	UInt8 typeCode = Stream_ReadU8(stream);
	if (typeCode == TC_NULL) { desc->ClassName[0] = '\0'; desc->FieldsCount = 0; return; }
	if (typeCode != TC_CLASSDESC) ErrorHandler_Fail("Unsupported type code in ClassDesc header");

	Dat_ReadString(stream, desc->ClassName);
	Stream_Skip(stream, 8 + 1); /* (8) serial version UID, (1) flags */

	desc->FieldsCount = Stream_ReadU16_BE(stream);
	if (desc->FieldsCount > Array_Elems(desc->Fields)) ErrorHandler_Fail("ClassDesc has too many fields");
	Int32 i;
	for (i = 0; i < desc->FieldsCount; i++) {
		Dat_ReadFieldDesc(stream, &desc->Fields[i]);
	}

	typeCode = Stream_ReadU8(stream);
	if (typeCode != TC_ENDBLOCKDATA) ErrorHandler_Fail("Unsupported type code in ClassDesc footer");
	struct JClassDesc superClassDesc;
	Dat_ReadClassDesc(stream, &superClassDesc);
}

static void Dat_ReadFieldData(struct Stream* stream, struct JFieldDesc* field) {
	switch (field->Type) {
	case JFIELD_INT8:
		field->Value_U8  = Stream_ReadU8(stream); break;
	case JFIELD_REAL32:
		/* TODO: Is this union abuse even legal */
		field->Value_I32 = Stream_ReadI32_BE(stream); break;
	case JFIELD_INT32:
		field->Value_I32 = Stream_ReadI32_BE(stream); break;
	case JFIELD_INT64:
		Stream_Skip(stream, 8); break; /* (8) data */
	case JFIELD_BOOL:
		field->Value_I32 = Stream_ReadU8(stream) != 0; break;

	case JFIELD_OBJECT: {
		/* Luckily for us, we only have to account for blockMap object */
		/* Other objects (e.g. player) are stored after the fields we actually care about, so ignore them */
		String fieldName = String_FromRawArray(field->FieldName);
		if (!String_CaselessEqualsConst(&fieldName, "blockMap")) break;

		UInt8 typeCode = Stream_ReadU8(stream);
		/* Skip all blockMap data with awful hacks */
		/* These offsets were based on server_level.dat map from original minecraft classic server */
		if (typeCode == TC_OBJECT) {
			Stream_Skip(stream, 315);
			UInt32 count = Stream_ReadU32_BE(stream);
			Stream_Skip(stream, 17 * count);
			Stream_Skip(stream, 152);
		} else if (typeCode != TC_NULL) {
			/* WoM maps have this field as null, which makes things easier for us */
			ErrorHandler_Fail("Unsupported type code in Object field");
		}
	} break;

	case JFIELD_ARRAY: {
		UInt8 typeCode = Stream_ReadU8(stream);
		if (typeCode == TC_NULL) break;
		if (typeCode != TC_ARRAY) ErrorHandler_Fail("Unsupported type code in Array field");

		struct JClassDesc arrayClassDesc;
		Dat_ReadClassDesc(stream, &arrayClassDesc);
		if (arrayClassDesc.ClassName[1] != JFIELD_INT8) ErrorHandler_Fail("Only byte array fields supported");

		UInt32 size = Stream_ReadU32_BE(stream);
		field->Value_Ptr = Mem_Alloc(size, sizeof(UInt8), ".dat map blocks");

		Stream_Read(stream, field->Value_Ptr, size);
		field->Value_Size = size;
	} break;
	}
}

static Int32 Dat_I32(struct JFieldDesc* field) {
	if (field->Type != JFIELD_INT32) ErrorHandler_Fail("Field type must be Int32");
	return field->Value_I32;
}

ReturnCode Dat_Load(struct Stream* stream) {
	ReturnCode result = Map_SkipGZipHeader(stream);
	if (result) return result;

	struct Stream compStream;
	struct InflateState state;
	Inflate_MakeStream(&compStream, &state, stream);

	UInt8 header[4 + 1 + 2 * 2];
	Stream_Read(&compStream, header, sizeof(header));

	/* .dat header */
	if (Stream_GetU32_BE(&header[0]) != 0x271BB788) return DAT_ERR_IDENTIFIER;
	if (header[4] != 0x02) return DAT_ERR_VERSION;

	/* Java seralisation headers */
	if (Stream_GetU16_BE(&header[5]) != 0xACED) return DAT_ERR_JIDENTIFIER;
	if (Stream_GetU16_BE(&header[7]) != 0x0005) return DAT_ERR_JVERSION;

	UInt8 typeCode = Stream_ReadU8(&compStream);
	if (typeCode != TC_OBJECT) ErrorHandler_Fail("Unexpected type code for root class");
	struct JClassDesc obj; Dat_ReadClassDesc(&compStream, &obj);

	Int32 i;
	for (i = 0; i < obj.FieldsCount; i++) {
		Dat_ReadFieldData(&compStream, &obj.Fields[i]);
	}

	Vector3* spawn = &LocalPlayer_Instance.Spawn;
	for (i = 0; i < obj.FieldsCount; i++) {
		struct JFieldDesc* field = &obj.Fields[i];
		String fieldName = String_FromRawArray(field->FieldName);

		if (String_CaselessEqualsConst(&fieldName, "width")) {
			World_Width  = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "height")) {
			World_Length = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "depth")) {
			World_Height = Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "blocks")) {
			if (field->Type != JFIELD_ARRAY) ErrorHandler_Fail("Blocks field must be Array");
			World_Blocks = field->Value_Ptr;
			World_BlocksSize = field->Value_Size;
		} else if (String_CaselessEqualsConst(&fieldName, "xSpawn")) {
			spawn->X = (Real32)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "ySpawn")) {
			spawn->Y = (Real32)Dat_I32(field);
		} else if (String_CaselessEqualsConst(&fieldName, "zSpawn")) {
			spawn->Z = (Real32)Dat_I32(field);
		}
	}
	return 0;
}


/*########################################################################################################################*
*--------------------------------------------------ClassicWorld export----------------------------------------------------*
*#########################################################################################################################*/
static void Cw_WriteCpeExtCompound(struct Stream* stream, const UChar* tagName, Int32 version) {
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, tagName);
	Nbt_WriteTag(stream, NBT_TAG_INT32, "ExtensionVersion");
	Nbt_WriteI32(stream, version);
}

static void Cw_WriteSpawnCompound(struct Stream* stream) {
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "Spawn");
	struct LocalPlayer* p = &LocalPlayer_Instance;
	Vector3 spawn = p->Spawn; /* TODO: Maybe keep real spawn too? */

	Nbt_WriteTag(stream, NBT_TAG_INT16, "X");
	Nbt_WriteI16(stream, spawn.X);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Y");
	Nbt_WriteI16(stream, spawn.Y);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Z");
	Nbt_WriteI16(stream, spawn.Z);

	Nbt_WriteTag(stream, NBT_TAG_INT8, "H");
	Nbt_WriteU8(stream, Math_Deg2Packed(p->SpawnRotY));
	Nbt_WriteTag(stream, NBT_TAG_INT8, "P");
	Nbt_WriteU8(stream, Math_Deg2Packed(p->SpawnHeadX));

	Nbt_WriteU8(stream, NBT_TAG_END);
}

static void Cw_WriteColCompound(struct Stream* stream, const UChar* tagName, PackedCol col) {
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, tagName);

	Nbt_WriteTag(stream, NBT_TAG_INT16, "R");
	Nbt_WriteI16(stream, col.R);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "G");
	Nbt_WriteI16(stream, col.G);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "B");
	Nbt_WriteI16(stream, col.B);

	Nbt_WriteU8(stream, NBT_TAG_END);
}

static void Cw_WriteBlockDefinitionCompound(struct Stream* stream, BlockID id) {
	UInt8 tmp[6];
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "Block" + id);
	bool sprite = Block_Draw[id] == DRAW_SPRITE;

	Nbt_WriteTag(stream, NBT_TAG_INT8, "ID");
	Nbt_WriteU8(stream, id);
	Nbt_WriteTag(stream, NBT_TAG_STRING, "Name");
	String name = Block_UNSAFE_GetName(id);
	Nbt_WriteString(stream, &name);

	Nbt_WriteTag(stream, NBT_TAG_INT8, "CollideType");
	Nbt_WriteU8(stream, Block_Collide[id]);
	union IntAndFloat speed; speed.f = Block_SpeedMultiplier[id];
	Nbt_WriteTag(stream, NBT_TAG_REAL32, "Speed");
	Nbt_WriteI32(stream, speed.i);

	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "Textures");
	tmp[0] = Block_GetTexLoc(id, FACE_YMAX);
	tmp[1] = Block_GetTexLoc(id, FACE_YMIN);
	tmp[2] = Block_GetTexLoc(id, FACE_XMIN);
	tmp[3] = Block_GetTexLoc(id, FACE_XMAX);
	tmp[4] = Block_GetTexLoc(id, FACE_ZMIN);
	tmp[5] = Block_GetTexLoc(id, FACE_ZMAX);
	Nbt_WriteU8_Array(stream, tmp, 6);

	Nbt_WriteTag(stream, NBT_TAG_INT8, "TransmitsLight");
	Nbt_WriteU8(stream, Block_BlocksLight[id] ? 0 : 1);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "WalkSound");
	Nbt_WriteU8(stream, Block_DigSounds[id]);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "FullBright");
	Nbt_WriteU8(stream, Block_FullBright[id] ? 1 : 0);

	Nbt_WriteTag(stream, NBT_TAG_INT8, "Shape");
	UInt8 shape = sprite ? 0 : (UInt8)(Block_MaxBB[id].Y * 16);
	Nbt_WriteU8(stream, shape);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "BlockDraw");
	UInt8 draw = sprite ? Block_SpriteOffset[id] : Block_Draw[id];
	Nbt_WriteU8(stream, draw);

	UInt8 fog = (UInt8)(128 * Block_FogDensity[id] - 1);
	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "Fog");
	PackedCol col = Block_FogCol[id];	
	tmp[0] = Block_FogDensity[id] ? fog : 0;
	tmp[1] = col.R; tmp[2] = col.G; tmp[3] = col.B;
	Nbt_WriteU8_Array(stream, tmp, 4);

	Vector3 minBB = Block_MinBB[id], maxBB = Block_MaxBB[id];
	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "Coords");
	tmp[0] = (UInt8)(minBB.X * 16); tmp[1] = (UInt8)(minBB.Y * 16); tmp[2] = (UInt8)(minBB.Z * 16); 
	tmp[3] = (UInt8)(maxBB.X * 16); tmp[4] = (UInt8)(maxBB.Y * 16); tmp[5] = (UInt8)(maxBB.Z * 16);
	Nbt_WriteU8_Array(stream, tmp, 6);

	Nbt_WriteU8(stream, NBT_TAG_END);
}

static void Cw_WriteMetadataCompound(struct Stream* stream) {
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "Metadata");
	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "CPE");

	Cw_WriteCpeExtCompound(stream, "ClickDistance", 1);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Distance");
	Nbt_WriteI16(stream, LocalPlayer_Instance.ReachDistance * 32);
	Nbt_WriteU8(stream, NBT_TAG_END);

	Cw_WriteCpeExtCompound(stream, "EnvWeatherType", 1);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "WeatherType");
	Nbt_WriteU8(stream, WorldEnv_Weather);
	Nbt_WriteU8(stream, NBT_TAG_END);

	Cw_WriteCpeExtCompound(stream, "EnvMapAppearance", 1);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "SideBlock");
	Nbt_WriteU8(stream, WorldEnv_SidesBlock);
	Nbt_WriteTag(stream, NBT_TAG_INT8, "EdgeBlock");
	Nbt_WriteU8(stream, WorldEnv_EdgeBlock);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "SideLevel");
	Nbt_WriteI16(stream, WorldEnv_EdgeHeight);
	Nbt_WriteTag(stream, NBT_TAG_STRING, "TextureURL");
	Nbt_WriteString(stream, &World_TextureUrl);
	Nbt_WriteU8(stream, NBT_TAG_END);

	Cw_WriteCpeExtCompound(stream, "EnvColors", 1);
	Cw_WriteColCompound(stream, "Sky", WorldEnv_SkyCol);
	Cw_WriteColCompound(stream, "Cloud", WorldEnv_CloudsCol);
	Cw_WriteColCompound(stream, "Fog", WorldEnv_FogCol);
	Cw_WriteColCompound(stream, "Ambient", WorldEnv_ShadowCol);
	Cw_WriteColCompound(stream, "Sunlight", WorldEnv_SunCol);
	Nbt_WriteU8(stream, NBT_TAG_END);

	Cw_WriteCpeExtCompound(stream, "BlockDefinitions", 1);
	Int32 block;
	for (block = 1; block < 256; block++) {
		if (Block_IsCustomDefined((BlockID)block)) {
			Cw_WriteBlockDefinitionCompound(stream, (BlockID)block);
		}
	}
	Nbt_WriteU8(stream, NBT_TAG_END);

	Nbt_WriteU8(stream, NBT_TAG_END);
	Nbt_WriteU8(stream, NBT_TAG_END);
}

void Cw_Save(struct Stream* stream) {
	struct GZipState state;
	struct Stream compStream;
	GZip_MakeStream(&compStream, &state, stream);
	stream = &compStream;

	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "ClassicWorld");

	Nbt_WriteTag(stream, NBT_TAG_INT8, "FormatVersion");
	Nbt_WriteU8(stream, 1);

	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "UUID");
	Nbt_WriteI32(stream, sizeof(World_Uuid));
	Stream_Write(stream, World_Uuid, sizeof(World_Uuid));

	Nbt_WriteTag(stream, NBT_TAG_INT16, "X");
	Nbt_WriteI16(stream, World_Width);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Y");
	Nbt_WriteI16(stream, World_Height);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Z");
	Nbt_WriteI16(stream, World_Length);

	Cw_WriteSpawnCompound(stream);

	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "BlockArray");
	Nbt_WriteI32(stream, World_BlocksSize);
	Stream_Write(stream, World_Blocks, World_BlocksSize);

	Cw_WriteMetadataCompound(stream);

	Nbt_WriteU8(stream, NBT_TAG_END);
	stream->Close(stream);
}


/*########################################################################################################################*
*---------------------------------------------------Schematic export------------------------------------------------------*
*#########################################################################################################################*/
void Schematic_Save(struct Stream* stream) {
	struct GZipState state;
	struct Stream compStream;
	GZip_MakeStream(&compStream, &state, stream);
	stream = &compStream;

	Nbt_WriteTag(stream, NBT_TAG_COMPOUND, "Schematic");

	Nbt_WriteTag(stream, NBT_TAG_STRING, "Materials");
	String classic = String_FromConst("Classic");
	Nbt_WriteString(stream, &classic);

	Nbt_WriteTag(stream, NBT_TAG_INT16, "Width");
	Nbt_WriteI16(stream, World_Width);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Height");
	Nbt_WriteI16(stream, World_Height);
	Nbt_WriteTag(stream, NBT_TAG_INT16, "Length");
	Nbt_WriteI16(stream, World_Length);

	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "Blocks");
	Nbt_WriteI32(stream, World_BlocksSize);
	Stream_Write(stream, World_Blocks, World_BlocksSize);

	Nbt_WriteTag(stream, NBT_TAG_INT8_ARRAY, "Data");
	Nbt_WriteI32(stream, World_BlocksSize);
	UInt8 chunk[8192] = { 0 };
	Int32 i;
	for (i = 0; i < World_BlocksSize; i += sizeof(chunk)) {
		Int32 count = min(sizeof(chunk), World_BlocksSize - i);
		Stream_Write(stream, chunk, count);
	}

	Nbt_WriteTag(stream, NBT_TAG_LIST, "Entities");
	Nbt_WriteU8(stream, NBT_TAG_COMPOUND); Nbt_WriteI32(stream, 0);
	Nbt_WriteTag(stream, NBT_TAG_LIST, "TileEntities");
	Nbt_WriteU8(stream, NBT_TAG_COMPOUND); Nbt_WriteI32(stream, 0);

	Nbt_WriteU8(stream, NBT_TAG_END);
	stream->Close(stream);
}
