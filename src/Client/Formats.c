#include "Formats.h"
#include "World.h"
#include "Deflate.h"
#include "Block.h"
#include "Entity.h"
#include "Platform.h"
#include "ExtMath.h"
#include "ErrorHandler.h"

void Map_ReadBlocks(Stream* stream) {
	World_BlocksSize = World_Width * World_Length * World_Height;
	World_Blocks = Platform_MemAlloc(World_BlocksSize);
	if (World_Blocks == NULL) {
		ErrorHandler_Fail("Failed to allocate memory for reading blocks array from file");
	}
	Stream_Read(stream, World_Blocks, World_BlocksSize);
}


#define LVL_VERSION 1874
#define LVL_CUSTOMTILE ((BlockID)163)
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

void Lvl_ReadCustomBlocks(Stream* stream) {
	Int32 x, y, z, i;
	UInt8 chunk[LVL_CHUNKSIZE * LVL_CHUNKSIZE * LVL_CHUNKSIZE];
	/* skip bounds checks when we know chunk is entirely inside map */
	Int32 adjWidth  = World_Width  & ~0x0F;
	Int32 adjHeight = World_Height & ~0x0F;
	Int32 adjLength = World_Length & ~0x0F;

	for (y = 0; y < World_Height; y += LVL_CHUNKSIZE) {
		for (z = 0; z < World_Length; z += LVL_CHUNKSIZE) {
			for (x = 0; x < World_Width; x += LVL_CHUNKSIZE) {
				if (Stream_TryReadByte(stream) != 1) continue;
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
}

void Lvl_ConvertPhysicsBlocks(void) {
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

void Lvl_Load(Stream* stream) {
	GZipHeader gzipHeader;
	GZipHeader_Init(&gzipHeader);
	while (!gzipHeader.Done) {
		GZipHeader_Read(stream, &gzipHeader);
	}
	
	Stream compStream;
	DeflateState state;
	Deflate_MakeStream(&compStream, &state, stream);

	UInt16 header = Stream_ReadUInt16_LE(&compStream);
	World_Width   = header == LVL_VERSION ? Stream_ReadUInt16_LE(&compStream) : header;
	World_Length  = Stream_ReadUInt16_LE(&compStream);
	World_Height  = Stream_ReadUInt16_LE(&compStream);

	LocalPlayer* p = &LocalPlayer_Instance;
	p->Spawn.X = (Real32)Stream_ReadUInt16_LE(&compStream);
	p->Spawn.Z = (Real32)Stream_ReadUInt16_LE(&compStream);
	p->Spawn.Y = (Real32)Stream_ReadUInt16_LE(&compStream);
	p->SpawnRotY  = Math_Packed2Deg(Stream_ReadUInt8(&compStream));
	p->SpawnHeadX = Math_Packed2Deg(Stream_ReadUInt8(&compStream));

	if (header == LVL_VERSION) {
		Stream_ReadUInt16_LE(&compStream); /* pervisit and perbuild perms */
	}
	Map_ReadBlocks(&compStream);

	Lvl_ConvertPhysicsBlocks();
	if (Stream_TryReadByte(&compStream) == 0xBD) {
		Lvl_ReadCustomBlocks(&compStream);
	}
}


#define FCM_IDENTIFIER 0x0FC2AF40UL
#define FCM_REVISION 13
void Fcm_ReadString(Stream* stream) {
	UInt16 length = Stream_ReadUInt16_LE(stream);
	UInt8 buffer[UInt16_MaxValue];
	Stream_Read(stream, buffer, length);
}

void Fcm_Load(Stream* stream) {
	if (Stream_ReadUInt32_LE(stream) != FCM_IDENTIFIER) {
		ErrorHandler_Fail("Invalid identifier in .fcm file");
	}
	if (Stream_ReadUInt8(stream) != FCM_REVISION) {
		ErrorHandler_Fail("Invalid revision in .fcm file");
	}

	World_Width  = Stream_ReadUInt16_LE(stream);
	World_Length = Stream_ReadUInt16_LE(stream);
	World_Height = Stream_ReadUInt16_LE(stream);

	LocalPlayer* p = &LocalPlayer_Instance;
	p->Spawn.X = Stream_ReadInt32_LE(stream) / 32.0f;
	p->Spawn.Y = Stream_ReadInt32_LE(stream) / 32.0f;
	p->Spawn.Z = Stream_ReadInt16_LE(stream) / 32.0f;
	p->SpawnRotY = Math_Packed2Deg(Stream_ReadUInt8(stream));
	p->SpawnHeadX = Math_Packed2Deg(Stream_ReadUInt8(stream));

	UInt8 tmp[26];
	Stream_Read(stream, tmp, 4); /* date modified */
	Stream_Read(stream, tmp, 4); /* date created */
	Stream_Read(stream, World_Uuid, sizeof(World_Uuid));
	Stream_Read(stream, tmp, 26); /* layer index */
	Int32 metaSize = Stream_ReadUInt32_LE(stream);

	Stream compStream;
	DeflateState state;
	Deflate_MakeStream(&compStream, &state, stream);

	Int32 i;
	for (i = 0; i < metaSize; i++) {
		Fcm_ReadString(&compStream); /* Group */
		Fcm_ReadString(&compStream); /* Key */
		Fcm_ReadString(&compStream); /* Value */
	}
	Map_ReadBlocks(&compStream);
}


#define NBT_TAG_END         0
#define NBT_TAG_INT8        1
#define NBT_TAG_INT16       2
#define NBT_TAG_INT32       3
#define NBT_TAG_INT64       4
#define NBT_TAG_REAL32      5
#define NBT_TAG_REAL64      6
#define NBT_TAG_INT8_ARRAY  7
#define NBT_TAG_STRING      8
#define NBT_TAG_LIST        9
#define NBT_TAG_COMPOUND    10
#define NBT_TAG_INT32_ARRAY 11
#define NBT_TAG_INVALID     255

struct NbtTag_;
typedef struct NbtTag_ {
	struct NbtTag_* Parent;
	UInt8 TagID;
	UInt8 NameBuffer[String_BufferSize(STRING_SIZE)];

	UInt32 DataSize; /* size of data for arrays */
	union {
		UInt8 Value_U8;
		Int16 Value_I16;
		Int32 Value_I32;
		Int64 Value_I64;
		Real32 Value_R32;
		Real64 Value_R64;
		UInt8 DataSmall[String_BufferSize(STRING_SIZE)];
		UInt8* DataBig; /* malloc for big byte arrays */
	};
} NbtTag;

UInt8 NbtTag_U8(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT8) { ErrorHandler_Fail("Expected I8 NBT tag"); }
	return tag->Value_U8;
}

Int16 NbtTag_I16(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT16) { ErrorHandler_Fail("Expected I16 NBT tag"); }
	return tag->Value_I16;
}

Int32 NbtTag_I32(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT32) { ErrorHandler_Fail("Expected I32 NBT tag"); }
	return tag->Value_I32;
}

Int64 NbtTag_I64(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_INT64) { ErrorHandler_Fail("Expected I64 NBT tag"); }
	return tag->Value_I64;
}

Real32 NbtTag_R32(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_REAL32) { ErrorHandler_Fail("Expected R32 NBT tag"); }
	return tag->Value_R32;
}

Real64 NbtTag_R64(NbtTag* tag) {
	if (tag->TagID != NBT_TAG_REAL64) { ErrorHandler_Fail("Expected R64 NBT tag"); }
	return tag->Value_R64;
}

UInt8 NbtTag_U8_At(NbtTag* tag, Int32 i) {
	if (tag->TagID != NBT_TAG_INT8_ARRAY) { ErrorHandler_Fail("Expected I8_Array NBT tag"); }
	if (i >= tag->DataSize) { ErrorHandler_Fail("Tried to access past bounds of I8_Array tag"); }

	if (tag->DataSize < STRING_SIZE) return tag->DataSmall[i];
	return tag->DataBig[i];
}

void Nbt_ReadTag(UInt8 typeId, bool readTagName, Stream* stream, NbtTag* parent) {
	NbtTag tag; 
	tag.TagID = NBT_TAG_INVALID;
	tag.NameBuffer[0] = NULL;
	if (typeId == 0) return tag;

	tag.Name = readTagName ? ReadString() : null;
	tag.TagID = typeId;
	tag.Parent = parent;
	UInt8 childTagId;
	UInt32 i, count;

	switch (typeId) {
	case NBT_TAG_INT8:
		tag.Value_U8 = Stream_ReadUInt8(stream); break;
	case NBT_TAG_INT16:
		tag.Value_I16 = Stream_ReadInt16_BE(stream); break;
	case NBT_TAG_INT32:
		tag.Value_I32 = Stream_ReadInt32_BE(stream); break;
	case NBT_TAG_INT64:
		tag.Value_I64 = Stream_ReadInt64_BE(stream); break;
	case NBT_TAG_REAL32:
		/* TODO: Is this union abuse even legal */
		tag.Value_I32 = Stream_ReadInt32_BE(stream); break;
	case NBT_TAG_REAL64:
		/* TODO: Is this union abuse even legal */
		tag.Value_I64 = Stream_ReadInt64_BE(stream); break;
	case NBT_TAG_INT8_ARRAY:
		count = Stream_ReadUInt32_BE(stream);
		tag.Value = reader.ReadBytes(count); break;
	case NBT_TAG_STRING:
		tag.Value = ReadString(); break;

	case NBT_TAG_LIST:
		childTagId = Stream_ReadUInt8(stream);
		count = Stream_ReadUInt32_BE(stream);
		for (i = 0; i < count; i++) {
			Nbt_ReadTag(childTagId, false, stream, &tag);
		}
		break;

	case NBT_TAG_COMPOUND:
		while ((childTagId = Stream_ReadUInt8(stream)) != NBT_TAG_INVALID) {
			Nbt_ReadTag(childTagId, true, stream, &tag);
		} 
		break;

	case NBT_TAG_INT32_ARRAY:
		ErrorHandler_Fail("Nbt Tag Int32_Array not supported");
		break;

	default:
		ErrorHandler_Fail("Unrecognised NBT tag");
	}
	return tag;
}