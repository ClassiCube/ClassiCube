#include "Block.h"
#include "DefaultSet.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "TerrainAtlas.h"
#include "Player.h"

TextureLoc Block_TopTex[BLOCK_CPE_COUNT] = { 0,  1,  0,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 24, 23,  6,  6,  7,  9,  4,
36, 37, 16, 11, 25, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 26, 53, 52, };

TextureLoc Block_SideTex[BLOCK_CPE_COUNT] = { 0,  1,  3,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 20, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 40, 39,  5,  5,  7,  8, 35,
36, 37, 16, 11, 41, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 42, 53, 52, };

TextureLoc Block_BottomTex[BLOCK_CPE_COUNT] = { 0,  1,  2,  2, 16,  4, 15, 17, 14, 14,
30, 30, 18, 19, 32, 33, 34, 21, 22, 48, 49, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 13, 12, 29, 28, 56, 55,  6,  6,  7, 10,  4,
36, 37, 16, 11, 57, 50, 38, 80, 81, 82, 83, 84, 51, 54, 86, 58, 53, 52 };

void Block_Reset(void) {
	Block_Init();
	Block_RecalculateSpriteBB();
}

void Block_Init(void) {
	Int32 count = Array_NumElements(DefinedCustomBlocks);
	Int32 i;
	for (i = 0; i < count; i++) {
		DefinedCustomBlocks[i] = 0;
	}

	Int32 block;
	for (block = BlockID_Air; block < BLOCK_COUNT; block++) {
		Block_ResetProps((BlockID)block);
	}
	Block_UpdateCullingAll();
}

void Block_SetDefaultPerms(void) {
	Int32 block;
	for (block = BlockID_Air; block <= BLOCK_MAX_DEFINED; block++) {
		Block_CanPlace[block] = true;
		Block_CanDelete[block] = true;
	}

	Block_CanPlace[BlockID_Air] = false;        Block_CanDelete[BlockID_Air] = false;
	Block_CanPlace[BlockID_Lava] = false;       Block_CanDelete[BlockID_Lava] = false;
	Block_CanPlace[BlockID_Water] = false;      Block_CanDelete[BlockID_Water] = false;
	Block_CanPlace[BlockID_StillLava] = false;  Block_CanDelete[BlockID_StillLava] = false;
	Block_CanPlace[BlockID_StillWater] = false; Block_CanDelete[BlockID_StillWater] = false;
	Block_CanPlace[BlockID_Bedrock] = false;    Block_CanDelete[BlockID_Bedrock] = false;
}

void Block_SetCollide(BlockID block, UInt8 collide) {
	/* necessary for cases where servers redefined core blocks before extended types were introduced. */
	collide = DefaultSet_MapOldCollide(block, collide);
	Block_ExtendedCollide[block] = collide;

	/* Reduce extended collision types to their simpler forms. */
	if (collide == CollideType_Ice) collide = CollideType_Solid;
	if (collide == CollideType_SlipperyIce) collide = CollideType_Solid;

	if (collide == CollideType_LiquidWater) collide = CollideType_Liquid;
	if (collide == CollideType_LiquidLava) collide = CollideType_Liquid;
	Block_Collide[block] = collide;
}

void Block_SetDrawType(BlockID block, UInt8 draw) {
	if (draw == DrawType_Opaque && Block_Collide[block] != CollideType_Solid) {
		draw = DrawType_Transparent;
	}
	Block_Draw[block] = draw;

	Vector3 zero = Vector3_Zero, one = Vector3_One;
	Block_FullOpaque[block] = draw == DrawType_Opaque
		&& Vector3_Equals(&Block_MinBB[block], &zero)
		&& Vector3_Equals(&Block_MaxBB[block], &one);
}


void Block_SplitUppercase(STRING_TRANSIENT String* buffer, STRING_PURE String* blockNames, Int32 start, Int32 end) {
	Int32 i;
	for (i = start; i < end; i++) {
		UInt8 c = String_CharAt(blockNames, i);
		bool upper = Char_IsUpper(c) && i > start;
		bool nextLower = i < end - 1 && !Char_IsUpper(String_CharAt(blockNames, i + 1));

		if (upper && nextLower) {
			String_Append(buffer, ' ');
			String_Append(buffer, Char_ToLower(c));
		} else {
			String_Append(buffer, c);
		}
	}
}

String Block_DefaultName(BlockID block) {
#if USE16_BIT
	if (block >= 256) return "ID " + block;
#endif
	if (block >= BLOCK_CPE_COUNT) {
		String invalid = String_FromConst("Invalid");
		return invalid;
	}

	String blockNames = String_FromConst(BLOCK_RAW_NAMES);
	/* Find start and end of this particular block name. */
	Int32 start = 0, i;
	for (i = 0; i < block; i++) {
		start = String_IndexOf(&blockNames, ' ', start) + 1;
	}
	Int32 end = String_IndexOf(&blockNames, ' ', start);
	if (end == -1) end = blockNames.length;

	String buffer = String_FromRawBuffer(Block_NamePtr(block), STRING_SIZE);
	Block_SplitUppercase(&buffer, &blockNames, start, end);
	return buffer;
}

void Block_ResetProps(BlockID block) {
	Block_BlocksLight[block] = DefaultSet_BlocksLight(block);
	Block_FullBright[block] = DefaultSet_FullBright(block);
	Block_FogColour[block] = DefaultSet_FogColour(block);
	Block_FogDensity[block] = DefaultSet_FogDensity(block);
	Block_SetCollide(block, DefaultSet_Collide(block));
	Block_DigSounds[block] = DefaultSet_DigSound(block);
	Block_StepSounds[block] = DefaultSet_StepSound(block);
	Block_SpeedMultiplier[block] = 1;
	Block_Name[block] = Block_DefaultName(block);
	Block_Tinted[block] = false;

	Block_Draw[block] = DefaultSet_Draw(block);
	if (Block_Draw[block] == DrawType_Sprite) {
		Block_MinBB[block] = Vector3_Create3(2.50f / 16.0f, 0.0f, 2.50f / 16.0f);
		Block_MaxBB[block] = Vector3_Create3(13.5f / 16.0f, 1.0f, 13.5f / 16.0f);
	} else {
		Block_MinBB[block] = Vector3_Zero;
		Block_MaxBB[block] = Vector3_One;
		Block_MaxBB[block].Y = DefaultSet_Height(block);
	}

	Block_SetDrawType(block, Block_Draw[block]);
	Block_CalcRenderBounds(block);
	Block_LightOffset[block] = Block_CalcLightOffset(block);

	if (block >= BLOCK_CPE_COUNT) {
#if USE16_BIT
		/* give some random texture ids */
		Block_SetTex((block * 10 + (block % 7) + 20) % 80, Face_YMax, block);
		Block_SetTex((block * 8 + (block & 5) + 5) % 80, Face_YMin, block);
		Block_SetSide((block * 4 + (block / 4) + 4) % 80, block);
#else
		Block_SetTex(0, Face_YMax, block);
		Block_SetTex(0, Face_YMin, block);
		Block_SetSide(0, block);
#endif
	} else {
		Block_SetTex(Block_TopTex[block], Face_YMax, block);
		Block_SetTex(Block_BottomTex[block], Face_YMin, block);
		Block_SetSide(Block_SideTex[block], block);
	}
}

Int32 Block_FindID(STRING_PURE String* name) {
	Int32 block;
	for (block = BlockID_Air; block < BLOCK_COUNT; block++) {
		if (String_CaselessEquals(&Block_Name[block], name)) return block;
	}
	return -1;
}

bool Block_IsLiquid(BlockID b) {
	CollideType collide = Block_ExtendedCollide[b];
	return
		(collide == CollideType_LiquidWater && Block_Draw[b] == DrawType_Translucent) ||
		(collide == CollideType_LiquidLava  && Block_Draw[b] == DrawType_Transparent);
}


void Block_SetSide(TextureLoc texLoc, BlockID blockId) {
	Int32 index = blockId * Face_Count;
	Block_Textures[index + Face_XMin] = texLoc;
	Block_Textures[index + Face_XMax] = texLoc;
	Block_Textures[index + Face_ZMin] = texLoc;
	Block_Textures[index + Face_ZMax] = texLoc;
}

void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId) {
	Block_Textures[blockId * Face_Count + face] = texLoc;
}

void Block_GetTextureRegion(BlockID block, Face face, Vector2* min, Vector2* max) {
	Vector3 bbMin = Block_MinBB[block], bbMax = Block_MaxBB[block];

	switch (face) {
	case Face_XMin:
	case Face_XMax:
		min->X = bbMin.Z; min->Y = bbMin.Y;
		max->X = bbMax.Z; max->Y = bbMax.Y;
		if (Block_IsLiquid(block)) max->Y -= 1.5f / 16.0f;
		break;

	case Face_ZMin:
	case Face_ZMax:
		min->X = bbMin.X; min->Y = bbMin.Y;
		max->X = bbMax.X; max->Y = bbMax.Y;
		if (Block_IsLiquid(block)) max->Y -= 1.5f / 16.0f;
		break;

	case Face_YMax:
	case Face_YMin:
		min->X = bbMin.X; min->Y = bbMin.Z;
		max->X = bbMax.X; max->Y = bbMax.Z;
		break;
	}
}

bool Block_FaceOccluded(BlockID block, BlockID other, Face face) {
	Vector2 bMin, bMax, oMin, oMax;
	Block_GetTextureRegion(block, face, &bMin, &bMax);
	Block_GetTextureRegion(other, face, &oMin, &oMax);

	return bMin.X >= oMin.X && bMin.Y >= oMin.Y
		&& bMax.X <= oMax.X && bMax.Y <= oMax.Y;
}



void Block_CalcRenderBounds(BlockID block) {
	Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

	if (Block_IsLiquid(block)) {
		min.X -= 0.1f / 16.0f; max.X -= 0.1f / 16.0f;
		min.Z -= 0.1f / 16.0f; max.Z -= 0.1f / 16.0f;
		min.Y -= 1.5f / 16.0f; max.Y -= 1.5f / 16.0f;
	} else if (Block_Draw[block] == DrawType_Translucent && Block_Collide[block] != CollideType_Solid) {
		min.X += 0.1f / 16.0f; max.X += 0.1f / 16.0f;
		min.Z += 0.1f / 16.0f; max.Z += 0.1f / 16.0f;
		min.Y -= 0.1f / 16.0f; max.Y -= 0.1f / 16.0f;
	}

	Block_RenderMinBB[block] = min; Block_RenderMaxBB[block] = max;
}

UInt8 Block_CalcLightOffset(BlockID block) {
	Int32 flags = 0xFF;
	Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

	if (min.X != 0) flags &= ~(1 << Face_XMin);
	if (max.X != 1) flags &= ~(1 << Face_XMax);
	if (min.Z != 0) flags &= ~(1 << Face_ZMin);
	if (max.Z != 1) flags &= ~(1 << Face_ZMax);

	if ((min.Y != 0 && max.Y == 1) && Block_Draw[block] != DrawType_Gas) {
		flags &= ~(1 << Face_YMax);
		flags &= ~(1 << Face_YMin);
	}
	return (UInt8)flags;
}

void Block_RecalculateSpriteBB(void) {
	Int32 block;
	for (block = BlockID_Air; block < BLOCK_COUNT; block++) {
		if (Block_Draw[block] != DrawType_Sprite) continue;

		Block_RecalculateBB((BlockID)block);
	}
}

Real32 Block_GetSpriteBB_TopY(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp) {
	Int32 x, y;
	for (y = 0; y < size; y++) {
		UInt32* row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
		for (x = 0; x < size; x++) {
			if ((UInt8)(row[x] >> 24) != 0) {
				return 1 - (Real32)y / size;
			}
		}
	}
	return 0;
}

Real32 Block_GetSpriteBB_BottomY(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp) {
	Int32 x, y;
	for (y = size - 1; y >= 0; y--) {
		UInt32* row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
		for (x = 0; x < size; x++) {
			if ((UInt8)(row[x] >> 24) != 0) {
				return 1 - (Real32)(y + 1) / size;
			}
		}
	}
	return 1;
}

Real32 Block_GetSpriteBB_LeftX(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp) {
	Int32 x, y;
	for (x = 0; x < size; x++) {
		for (y = 0; y < size; y++) {
			UInt32* row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
			if ((UInt8)(row[x] >> 24) != 0) {
				return (Real32)x / size;
			}
		}
	}
	return 1;
}

Real32 Block_GetSpriteBB_RightX(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp) {
	Int32 x, y;
	for (x = size - 1; x >= 0; x--) {
		for (y = 0; y < size; y++) {
			UInt32* row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
			if ((UInt8)(row[x] >> 24) != 0) {
				return (Real32)(x + 1) / size;
			}
		}
	}
	return 0;
}

void Block_RecalculateBB(BlockID block) {
	Bitmap* bmp = &Atlas2D_Bitmap;
	Int32 elemSize = Atlas2D_ElementSize;
	TextureLoc texLoc = Block_GetTexLoc(block, Face_XMax);
	Int32 texX = texLoc & 0x0F, texY = texLoc >> 4;

	Real32 topY = Block_GetSpriteBB_TopY(elemSize, texX, texY, bmp);
	Real32 bottomY = Block_GetSpriteBB_BottomY(elemSize, texX, texY, bmp);
	Real32 leftX = Block_GetSpriteBB_LeftX(elemSize, texX, texY, bmp);
	Real32 rightX = Block_GetSpriteBB_RightX(elemSize, texX, texY, bmp);

	Vector3 centre = Vector3_Create3(0.5f, 0, 0.5f);
	Vector3 minRaw = Vector3_RotateY3(leftX - 0.5f, bottomY, 0, 45.0f * MATH_DEG2RAD);
	Vector3 maxRaw = Vector3_RotateY3(rightX - 0.5f, topY, 0, 45.0f * MATH_DEG2RAD);

	Vector3_Add(&minRaw, &centre, &Block_MinBB[block]);
	Vector3_Add(&maxRaw, &centre, &Block_MaxBB[block]);
	Block_CalcRenderBounds(block);
}



void Block_SetXStretch(BlockID block, bool stretch) {
	Block_CanStretch[block] &= 0xC3; /* ~0x3C */
	Block_CanStretch[block] |= (stretch ? 0x3C : (UInt8)0);
}

void Block_SetZStretch(BlockID block, bool stretch) {
	Block_CanStretch[block] &= 0xFC; /* ~0x03 */
	Block_CanStretch[block] |= (stretch ? 0x03 : (UInt8)0);
}

void Block_CalcCulling(BlockID block, BlockID other) {
	Vector3 bMin = Block_MinBB[block], bMax = Block_MaxBB[block];
	Vector3 oMin = Block_MinBB[other], oMax = Block_MaxBB[other];
	if (Block_IsLiquid(block)) bMax.Y -= 1.5f / 16;
	if (Block_IsLiquid(other)) oMax.Y -= 1.5f / 16;

	if (Block_Draw[block] == DrawType_Sprite) {
		Block_SetHidden(block, other, Face_XMin, true);
		Block_SetHidden(block, other, Face_XMax, true);
		Block_SetHidden(block, other, Face_ZMin, true);
		Block_SetHidden(block, other, Face_ZMax, true);
		Block_SetHidden(block, other, Face_YMin, oMax.Y == 1);
		Block_SetHidden(block, other, Face_YMax, bMax.Y == 1);
	} else {
		Block_SetXStretch(block, bMin.X == 0 && bMax.X == 1);
		Block_SetZStretch(block, bMin.Z == 0 && bMax.Z == 1);
		bool bothLiquid = Block_IsLiquid(block) && Block_IsLiquid(other);

		Block_SetHidden(block, other, Face_XMin, oMax.X == 1 && bMin.X == 0);
		Block_SetHidden(block, other, Face_XMax, oMin.X == 0 && bMax.X == 1);
		Block_SetHidden(block, other, Face_ZMin, oMax.Z == 1 && bMin.Z == 0);
		Block_SetHidden(block, other, Face_ZMax, oMin.Z == 0 && bMax.Z == 1);

		Block_SetHidden(block, other, Face_YMin,
			bothLiquid || (oMax.Y == 1 && bMin.Y == 0));
		Block_SetHidden(block, other, Face_YMax,
			bothLiquid || (oMin.Y == 0 && bMax.Y == 1));
	}
}

void Block_UpdateCullingAll(void) {
	Int32 block, neighbour;
	for (block = BlockID_Air; block < BLOCK_COUNT; block++)
		Block_CanStretch[block] = 0x3F;

	for (block = BlockID_Air; block < BLOCK_COUNT; block++) {
		for (neighbour = BlockID_Air; neighbour < BLOCK_COUNT; neighbour++) {
			Block_CalcCulling((BlockID)block, (BlockID)neighbour);
		}
	}
}

void Block_UpdateCulling(BlockID block) {
	Block_CanStretch[block] = 0x3F;

	Int32 other;
	for (other = BlockID_Air; other < BLOCK_COUNT; other++) {
		Block_CalcCulling(block, (BlockID)other);
		Block_CalcCulling((BlockID)other, block);
	}
}

bool Block_IsHidden(BlockID block, BlockID other) {
	/* Sprite blocks can never hide faces. */
	if (Block_Draw[block] == DrawType_Sprite) return false;

	/* NOTE: Water is always culled by lava. */
	if ((block == BlockID_Water || block == BlockID_StillWater)
		&& (other == BlockID_Lava || other == BlockID_StillLava))
		return true;

	/* All blocks (except for say leaves) cull with themselves. */
	if (block == other) return Block_Draw[block] != DrawType_TransparentThick;

	/* An opaque neighbour (asides from lava) culls the face. */
	if (Block_Draw[other] == DrawType_Opaque && !Block_IsLiquid(other)) return true;
	if (Block_Draw[block] != DrawType_Translucent || Block_Draw[other] != DrawType_Translucent) return false;

	/* e.g. for water / ice, don't need to draw water. */
	UInt8 bType = Block_Collide[block], oType = Block_Collide[other];
	bool canSkip = (bType == CollideType_Solid && oType == CollideType_Solid)
		|| bType != CollideType_Solid;
	return canSkip;
}

void Block_SetHidden(BlockID block, BlockID other, Face face, bool value) {
	value = Block_IsHidden(block, other) && Block_FaceOccluded(block, other, face) && value;
	Int32 bit = value ? 1 : 0;
	Block_Hidden[block * BLOCK_COUNT + other] &= (UInt8)~(1 << face);
	Block_Hidden[block * BLOCK_COUNT + other] |= (UInt8)(bit << face);
}

bool Block_IsFaceHidden(BlockID block, BlockID other, Face face) {
#if USE16_BIT
	return (hidden[(block << 12) | other] & (1 << face)) != 0;
#else
	return (Block_Hidden[(block << 8) | other] & (1 << face)) != 0;
#endif
}



#define AR_EQ1(s, x) (s.length >= 1 && Char_ToLower(s.buffer[0]) == x)
#define AR_EQ2(s, x, y) (s.length >= 2 && Char_ToLower(s.buffer[0]) == x && Char_ToLower(s.buffer[1]) == y)

BlockID AutoRotate_Find(BlockID block, String* name, const UInt8* suffix) {
	UInt8 buffer[String_BufferSize(128)];
	String temp = String_FromRawBuffer(buffer, 128);
	String_AppendString(&temp, name);
	String_AppendConst(&temp, suffix);

	Int32 rotated = Block_FindID(&temp);
	if (rotated != -1) return (BlockID)rotated;
	return block;
}

BlockID AutoRotate_RotateCorner(BlockID block, String* name, Vector3 offset) {
	if (offset.X < 0.5f && offset.Z < 0.5f) {
		return AutoRotate_Find(block, name, "-NW");
	} else if (offset.X >= 0.5f && offset.Z < 0.5f) {
		return AutoRotate_Find(block, name, "-NE");
	} else if (offset.X < 0.5f && offset.Z >= 0.5f) {
		return AutoRotate_Find(block, name, "-SW");
	} else if (offset.X >= 0.5f && offset.Z >= 0.5f) {
		return AutoRotate_Find(block, name, "-SE");
	}
	return block;
}

BlockID AutoRotate_RotateVertical(BlockID block, String* name, Vector3 offset) {
	if (offset.Y >= 0.5f) {
		return AutoRotate_Find(block, name, "-U");
	} else {
		return AutoRotate_Find(block, name, "-D");
	}
}

BlockID AutoRotate_RotateOther(BlockID block, String* name, Vector3 offset) {
	/* Fence type blocks */
	if (AutoRotate_Find(BlockID_Invalid, name, "-UD") == BlockID_Invalid) {
		Real32 headY = LocalPlayer_Instance.Base.Base.HeadY;
		headY = LocationUpdate_Clamp(headY);

		if (headY < 45.0f || (headY >= 135.0f && headY < 225.0f) || headY > 315.0f) {
			return AutoRotate_Find(block, name, "-WE");
		} else {
			return AutoRotate_Find(block, name, "-NS");
		}
	}

	/* Thin pillar type blocks */
	Face face = Game_SelectedPos.ClosestFace;
	if (face == Face_YMax || face == Face_YMin)
		return AutoRotate_Find(block, name, "-UD");
	if (face == Face_XMax || face == Face_XMin)
		return AutoRotate_Find(block, name, "-WE");
	if (face == Face_ZMax || face == Face_ZMin)
		return AutoRotate_Find(block, name, "-NS");
	return block;
}

BlockID AutoRotate_RotateDirection(BlockID block, String* name, Vector3 offset) {
	Vector3 SE = Vector3_Create3(+1.0f, 0.0f, 1.0f);
	Vector3 SW = Vector3_Create3(-1.0f, 0.0f, 1.0f);

	Vector3I pos = Game_SelectedPos.TranslatedPos;
	Vector3 exact = Game_SelectedPos.Intersect;
	Vector3 exactFlat = exact; exactFlat.Y = 0.0f;

	Vector3 SEToPoint = exactFlat; SEToPoint.X -= pos.X; SEToPoint.Z -= pos.Z;
	Vector3 SWToPoint = exactFlat; SWToPoint.X -= (pos.X + 1); SWToPoint.Z -= pos.Z;

	Real32 dotSE = Vector3_Dot(&SEToPoint, &SW);
	Real32 dotSW = Vector3_Dot(&SWToPoint, &SE);

	if (dotSE <= 0.0f) { /* NorthEast */
		if (dotSW <= 0.0f) { /* NorthWest */
			return AutoRotate_Find(block, name, "-N");
		} else { /* SouthEast */
			return AutoRotate_Find(block, name, "-E");
		}
	} else { /* SouthWest */
		if (dotSW <= 0.0f) { /* NorthWest */
			return AutoRotate_Find(block, name, "-W");
		} else { /* SouthEast */
			return AutoRotate_Find(block, name, "-S");
		}
	}
}

BlockID AutoRotate_RotateBlock(BlockID block) {
	String name = Block_Name[block];
	Int32 dirIndex = String_LastIndexOf(&name, '-');
	if (dirIndex == -1) return block; /* not a directional block */

	String dir = String_UNSAFE_SubstringAt(&name, dirIndex + 1);
	String baseName = String_UNSAFE_Substring(&name, 0, dirIndex);

	Vector3 translated, offset;
	Vector3I_ToVector3(&translated, &Game_SelectedPos.TranslatedPos);
	Vector3_Subtract(&offset, &Game_SelectedPos.Intersect, &translated);

	if (AR_EQ2(dir, 'n', 'w') || AR_EQ2(dir, 'n', 'e') || AR_EQ2(dir, 's', 'w') || AR_EQ2(dir, 's', 'e')) {
		return AutoRotate_RotateCorner(block, &baseName, offset);
	} else if (AR_EQ1(dir, 'u') || AR_EQ1(dir, 'd')) {
		return AutoRotate_RotateVertical(block, &baseName, offset);
	} else if (AR_EQ1(dir, 'n') || AR_EQ1(dir, 'w') || AR_EQ1(dir, 's') || AR_EQ1(dir, 'e')) {
		return AutoRotate_RotateDirection(block, &baseName, offset);
	} else if (AR_EQ2(dir, 'u', 'd') || AR_EQ2(dir, 'w', 'e') || AR_EQ2(dir, 'n', 's')) {
		return AutoRotate_RotateOther(block, &baseName, offset);
	}
	return block;
}