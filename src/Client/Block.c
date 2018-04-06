#include "Block.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "TerrainAtlas.h"
#include "Game.h"
#include "Entity.h"

UInt32 Block_DefinedCustomBlocks[BLOCK_COUNT >> 5];
UInt8 Block_NamesBuffer[String_BufferSize(STRING_SIZE) * BLOCK_COUNT];
#define Block_NamePtr(i) &Block_NamesBuffer[String_BufferSize(STRING_SIZE) * i]

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
	Int32 count = Array_Elems(Block_DefinedCustomBlocks);
	Int32 i;
	for (i = 0; i < count; i++) {
		Block_DefinedCustomBlocks[i] = 0;
	}

	Int32 block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Block_ResetProps((BlockID)block);
	}
	Block_UpdateCullingAll();
}

void Block_SetDefaultPerms(void) {
	Int32 block;
	for (block = BLOCK_AIR; block <= BLOCK_MAX_DEFINED; block++) {
		Block_CanPlace[block] = true;
		Block_CanDelete[block] = true;
	}

	Block_CanPlace[BLOCK_AIR] = false;         Block_CanDelete[BLOCK_AIR] = false;
	Block_CanPlace[BLOCK_LAVA] = false;        Block_CanDelete[BLOCK_LAVA] = false;
	Block_CanPlace[BLOCK_WATER] = false;       Block_CanDelete[BLOCK_WATER] = false;
	Block_CanPlace[BLOCK_STILL_LAVA] = false;  Block_CanDelete[BLOCK_STILL_LAVA] = false;
	Block_CanPlace[BLOCK_STILL_WATER] = false; Block_CanDelete[BLOCK_STILL_WATER] = false;
	Block_CanPlace[BLOCK_BEDROCK] = false;     Block_CanDelete[BLOCK_BEDROCK] = false;
}

bool Block_IsCustomDefined(BlockID block) {
	return (Block_DefinedCustomBlocks[block >> 5] & (1u << (block & 0x1F))) != 0;
}

void Block_SetCustomDefined(BlockID block, bool defined) {
	if (defined) {
		Block_DefinedCustomBlocks[block >> 5] |=  (1u << (block & 0x1F));
	} else {
		Block_DefinedCustomBlocks[block >> 5] &= ~(1u << (block & 0x1F));
	}
}

void Block_RecalcIsLiquid(BlockID b) {
	UInt8 collide = Block_ExtendedCollide[b];
	Block_IsLiquid[b] =
		(collide == COLLIDE_LIQUID_WATER && Block_Draw[b] == DRAW_TRANSLUCENT) ||
		(collide == COLLIDE_LIQUID_LAVA  && Block_Draw[b] == DRAW_TRANSPARENT);
}

void Block_SetCollide(BlockID block, UInt8 collide) {
	/* necessary for cases where servers redefined core blocks before extended types were introduced. */
	collide = DefaultSet_MapOldCollide(block, collide);
	Block_ExtendedCollide[block] = collide;
	Block_RecalcIsLiquid(block);

	/* Reduce extended collision types to their simpler forms. */
	if (collide == COLLIDE_ICE) collide = COLLIDE_SOLID;
	if (collide == COLLIDE_SLIPPERY_ICE) collide = COLLIDE_SOLID;

	if (collide == COLLIDE_LIQUID_WATER) collide = COLLIDE_LIQUID;
	if (collide == COLLIDE_LIQUID_LAVA) collide = COLLIDE_LIQUID;
	Block_Collide[block] = collide;
}

void Block_SetDrawType(BlockID block, UInt8 draw) {
	if (draw == DRAW_OPAQUE && Block_Collide[block] != COLLIDE_SOLID) {
		draw = DRAW_TRANSPARENT;
	}
	Block_Draw[block] = draw;
	Block_RecalcIsLiquid(block);

	Vector3 zero = Vector3_Zero; Vector3 one = Vector3_One;
	Block_FullOpaque[block] = draw == DRAW_OPAQUE
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

	String buffer = String_InitAndClear(Block_NamePtr(block), STRING_SIZE);
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
	Block_SpeedMultiplier[block] = 1.0f;
	Block_Name[block] = Block_DefaultName(block);
	Block_Tinted[block] = false;
	Block_SpriteOffset[block] = 0;

	Block_Draw[block] = DefaultSet_Draw(block);
	if (Block_Draw[block] == DRAW_SPRITE) {
		Block_MinBB[block] = Vector3_Create3(2.50f / 16.0f, 0.0f, 2.50f / 16.0f);
		Block_MaxBB[block] = Vector3_Create3(13.5f / 16.0f, 1.0f, 13.5f / 16.0f);
	} else {		
		Vector3 zero = Vector3_Zero; Block_MinBB[block] = zero;
		Vector3 one  = Vector3_One;  Block_MaxBB[block] = one;
		Block_MaxBB[block].Y = DefaultSet_Height(block);
	}

	Block_SetDrawType(block, Block_Draw[block]);
	Block_CalcRenderBounds(block);
	Block_LightOffset[block] = Block_CalcLightOffset(block);

	if (block >= BLOCK_CPE_COUNT) {
		Block_SetTex(0, FACE_YMAX, block);
		Block_SetTex(0, FACE_YMIN, block);
		Block_SetSide(0, block);
	} else {
		Block_SetTex(Block_TopTex[block], FACE_YMAX, block);
		Block_SetTex(Block_BottomTex[block], FACE_YMIN, block);
		Block_SetSide(Block_SideTex[block], block);
	}
}

Int32 Block_FindID(STRING_PURE String* name) {
	Int32 block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		if (String_CaselessEquals(&Block_Name[block], name)) return block;
	}
	return -1;
}


void Block_SetSide(TextureLoc texLoc, BlockID blockId) {
	Int32 index = blockId * FACE_COUNT;
	Block_Textures[index + FACE_XMIN] = texLoc;
	Block_Textures[index + FACE_XMAX] = texLoc;
	Block_Textures[index + FACE_ZMIN] = texLoc;
	Block_Textures[index + FACE_ZMAX] = texLoc;
}

void Block_SetTex(TextureLoc texLoc, Face face, BlockID blockId) {
	Block_Textures[blockId * FACE_COUNT + face] = texLoc;
}



void Block_CalcRenderBounds(BlockID block) {
	Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

	if (Block_IsLiquid[block]) {
		min.X -= 0.1f / 16.0f; max.X -= 0.1f / 16.0f;
		min.Z -= 0.1f / 16.0f; max.Z -= 0.1f / 16.0f;
		min.Y -= 1.5f / 16.0f; max.Y -= 1.5f / 16.0f;
	} else if (Block_Draw[block] == DRAW_TRANSLUCENT && Block_Collide[block] != COLLIDE_SOLID) {
		min.X += 0.1f / 16.0f; max.X += 0.1f / 16.0f;
		min.Z += 0.1f / 16.0f; max.Z += 0.1f / 16.0f;
		min.Y -= 0.1f / 16.0f; max.Y -= 0.1f / 16.0f;
	}

	Block_RenderMinBB[block] = min; Block_RenderMaxBB[block] = max;
}

UInt8 Block_CalcLightOffset(BlockID block) {
	Int32 flags = 0xFF;
	Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

	if (min.X != 0) flags &= ~(1 << FACE_XMIN);
	if (max.X != 1) flags &= ~(1 << FACE_XMAX);
	if (min.Z != 0) flags &= ~(1 << FACE_ZMIN);
	if (max.Z != 1) flags &= ~(1 << FACE_ZMAX);

	if ((min.Y != 0 && max.Y == 1) && Block_Draw[block] != DRAW_GAS) {
		flags &= ~(1 << FACE_YMAX);
		flags &= ~(1 << FACE_YMIN);
	}
	return (UInt8)flags;
}

void Block_RecalculateSpriteBB(void) {
	Int32 block;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		if (Block_Draw[block] != DRAW_SPRITE) continue;

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
	TextureLoc texLoc = Block_GetTexLoc(block, FACE_XMAX);
	Int32 texX = texLoc & 0x0F, texY = texLoc >> 4;

	Real32 topY    = Block_GetSpriteBB_TopY(elemSize,    texX, texY, bmp);
	Real32 bottomY = Block_GetSpriteBB_BottomY(elemSize, texX, texY, bmp);
	Real32 leftX   = Block_GetSpriteBB_LeftX(elemSize,   texX, texY, bmp);
	Real32 rightX  = Block_GetSpriteBB_RightX(elemSize,  texX, texY, bmp);

	Vector3 centre = Vector3_Create3(0.5f, 0, 0.5f);
	Vector3 minRaw = Vector3_RotateY3(leftX  - 0.5f, bottomY, 0, 45.0f * MATH_DEG2RAD);
	Vector3 maxRaw = Vector3_RotateY3(rightX - 0.5f, topY,    0, 45.0f * MATH_DEG2RAD);

	Vector3_Add(&minRaw, &centre, &Block_MinBB[block]);
	Vector3_Add(&maxRaw, &centre, &Block_MaxBB[block]);
	Block_CalcRenderBounds(block);
}


void Block_CalcStretch(BlockID block) {
	/* faces which can be stretched on X axis */
	if (Block_MinBB[block].X == 0.0f && Block_MaxBB[block].X == 1.0f) {
		Block_CanStretch[block] |= 0x3C;
	} else {
		Block_CanStretch[block] &= 0xC3; /* ~0x3C */
	}

	/* faces which can be stretched on Z axis */
	if (Block_MinBB[block].Z == 0.0f && Block_MaxBB[block].Z == 1.0f) {
		Block_CanStretch[block] |= 0x03;
	} else {
		Block_CanStretch[block] &= 0xFC; /* ~0x03 */
	}
}

bool Block_IsHidden(BlockID block, BlockID other) {
	/* Sprite blocks can never hide faces. */
	if (Block_Draw[block] == DRAW_SPRITE) return false;

	/* NOTE: Water is always culled by lava. */
	if ((block == BLOCK_WATER || block == BLOCK_STILL_WATER)
		&& (other == BLOCK_LAVA || other == BLOCK_STILL_LAVA))
		return true;

	/* All blocks (except for say leaves) cull with themselves. */
	if (block == other) return Block_Draw[block] != DRAW_TRANSPARENT_THICK;

	/* An opaque neighbour (asides from lava) culls the face. */
	if (Block_Draw[other] == DRAW_OPAQUE && !Block_IsLiquid[other]) return true;
	if (Block_Draw[block] != DRAW_TRANSLUCENT || Block_Draw[other] != DRAW_TRANSLUCENT) return false;

	/* e.g. for water / ice, don't need to draw water. */
	UInt8 bType = Block_Collide[block], oType = Block_Collide[other];
	bool canSkip = (bType == COLLIDE_SOLID && oType == COLLIDE_SOLID) || bType != COLLIDE_SOLID;
	return canSkip;
}

void Block_CalcCulling(BlockID block, BlockID other) {
	if (!Block_IsHidden(block, other)) {
		/* Block is not hidden at all, so we can just entirely skip per-face check */
		Block_Hidden[(block * BLOCK_COUNT) + other] = 0;
	} else {
		Vector3 bMin = Block_MinBB[block], bMax = Block_MaxBB[block];
		Vector3 oMin = Block_MinBB[other], oMax = Block_MaxBB[other];
		if (Block_IsLiquid[block]) bMax.Y -= 1.50f / 16.0f;
		if (Block_IsLiquid[other]) oMax.Y -= 1.50f / 16.0f;

		/* Don't need to care about sprites here since they never cull faces */
		bool bothLiquid = Block_IsLiquid[block] && Block_IsLiquid[other];
		Int32 f = 0; /* mark all faces initially 'not hidden' */

		/* Whether the 'texture region' of a face on block fits inside corresponding region on other block */
		bool occludedX = (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);
		bool occludedY = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Z >= oMin.Z && bMax.Z <= oMax.Z);
		bool occludedZ = (bMin.X >= oMin.X && bMax.X <= oMax.X) && (bMin.Y >= oMin.Y && bMax.Y <= oMax.Y);

		f |= occludedX && oMax.X == 1.0f && bMin.X == 0.0f ? (1 << FACE_XMIN) : 0;
		f |= occludedX && oMin.X == 0.0f && bMax.X == 1.0f ? (1 << FACE_XMAX) : 0;
		f |= occludedZ && oMax.Z == 1.0f && bMin.Z == 0.0f ? (1 << FACE_ZMIN) : 0;
		f |= occludedZ && oMin.Z == 0.0f && bMax.Z == 1.0f ? (1 << FACE_ZMAX) : 0;
		f |= occludedY && (bothLiquid || (oMax.Y == 1.0f && bMin.Y == 0.0f)) ? (1 << FACE_YMIN) : 0;
		f |= occludedY && (bothLiquid || (oMin.Y == 0.0f && bMax.Y == 1.0f)) ? (1 << FACE_YMAX) : 0;
		Block_Hidden[(block * BLOCK_COUNT) + other] = (UInt8)f;
	}
}

bool Block_IsFaceHidden(BlockID block, BlockID other, Face face) {
	return (Block_Hidden[(block * BLOCK_COUNT) + other] & (1 << face)) != 0;
}

void Block_UpdateCullingAll(void) {
	Int32 block, neighbour;
	for (block = BLOCK_AIR; block < BLOCK_COUNT; block++) {
		Block_CalcStretch((BlockID)block);
		for (neighbour = BLOCK_AIR; neighbour < BLOCK_COUNT; neighbour++) {
			Block_CalcCulling((BlockID)block, (BlockID)neighbour);
		}
	}
}

void Block_UpdateCulling(BlockID block) {
	Block_CalcStretch(block);
	Int32 other;
	for (other = BLOCK_AIR; other < BLOCK_COUNT; other++) {
		Block_CalcCulling(block, (BlockID)other);
		Block_CalcCulling((BlockID)other, block);
	}
}


#define AR_EQ1(s, x) (s.length >= 1 && Char_ToLower(s.buffer[0]) == x)
#define AR_EQ2(s, x, y) (s.length >= 2 && Char_ToLower(s.buffer[0]) == x && Char_ToLower(s.buffer[1]) == y)

BlockID AutoRotate_Find(BlockID block, String* name, const UInt8* suffix) {
	UInt8 buffer[String_BufferSize(STRING_SIZE * 2)];
	String temp = String_InitAndClearArray(buffer);
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
	if (AutoRotate_Find(BLOCK_AIR, name, "-UD") == BLOCK_AIR) {
		Real32 headY = LocalPlayer_Instance.Base.HeadY;
		headY = LocationUpdate_Clamp(headY);

		if (headY < 45.0f || (headY >= 135.0f && headY < 225.0f) || headY > 315.0f) {
			return AutoRotate_Find(block, name, "-WE");
		} else {
			return AutoRotate_Find(block, name, "-NS");
		}
	}

	/* Thin pillar type blocks */
	Face face = Game_SelectedPos.ClosestFace;
	if (face == FACE_YMAX || face == FACE_YMIN)
		return AutoRotate_Find(block, name, "-UD");
	if (face == FACE_XMAX || face == FACE_XMIN)
		return AutoRotate_Find(block, name, "-WE");
	if (face == FACE_ZMAX || face == FACE_ZMIN)
		return AutoRotate_Find(block, name, "-NS");
	return block;
}

BlockID AutoRotate_RotateDirection(BlockID block, String* name, Vector3 offset) {
	Real32 headY = LocalPlayer_Instance.Base.HeadY;
	headY = LocationUpdate_Clamp(headY);

	if (headY >= 45.0f && headY < 135.0f) {
		return AutoRotate_Find(block, name, "-E");
	} else if (headY >= 135.0f && headY < 225.0f) {
		return AutoRotate_Find(block, name, "-S");
	} else if (headY >= 225.0f && headY < 315.0f) {
		return AutoRotate_Find(block, name, "-W");
	} else {
		return AutoRotate_Find(block, name, "-N");
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


Real32 DefaultSet_Height(BlockID b) {
	if (b == BLOCK_SLAB) return 0.5f;
	if (b == BLOCK_COBBLE_SLAB) return 0.5f;
	if (b == BLOCK_SNOW) return 0.25f;
	return 1.0f;
}

bool DefaultSet_FullBright(BlockID b) {
	return b == BLOCK_LAVA || b == BLOCK_STILL_LAVA
		|| b == BLOCK_MAGMA || b == BLOCK_FIRE;
}

Real32 DefaultSet_FogDensity(BlockID b) {
	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER)
		return 0.1f;
	if (b == BLOCK_LAVA || b == BLOCK_STILL_LAVA)
		return 1.8f;
	return 0.0f;
}

PackedCol DefaultSet_FogColour(BlockID b) {
	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER)
		return PackedCol_Create3(5, 5, 51);
	if (b == BLOCK_LAVA || b == BLOCK_STILL_LAVA)
		return PackedCol_Create3(153, 25, 0);
	return PackedCol_Create4(0, 0, 0, 0);
}

UInt8 DefaultSet_Collide(BlockID b) {
	if (b == BLOCK_ICE) return COLLIDE_ICE;
	if (b == BLOCK_WATER || b == BLOCK_STILL_WATER)
		return COLLIDE_LIQUID_WATER;
	if (b == BLOCK_LAVA || b == BLOCK_STILL_LAVA)
		return COLLIDE_LIQUID_LAVA;

	if (b == BLOCK_SNOW || b == BLOCK_AIR || DefaultSet_Draw(b) == DRAW_SPRITE)
		return COLLIDE_GAS;
	return COLLIDE_SOLID;
}

UInt8 DefaultSet_MapOldCollide(BlockID b, UInt8 collide) {
	if (b == BLOCK_ROPE && collide == COLLIDE_GAS)
		return COLLIDE_CLIMB_ROPE;
	if (b == BLOCK_ICE && collide == COLLIDE_SOLID)
		return COLLIDE_ICE;
	if ((b == BLOCK_WATER || b == BLOCK_STILL_WATER) && collide == COLLIDE_LIQUID)
		return COLLIDE_LIQUID_WATER;
	if ((b == BLOCK_LAVA || b == BLOCK_STILL_LAVA) && collide == COLLIDE_LIQUID)
		return COLLIDE_LIQUID_LAVA;
	return collide;
}

bool DefaultSet_BlocksLight(BlockID b) {
	return !(b == BLOCK_GLASS || b == BLOCK_LEAVES
		|| b == BLOCK_AIR || DefaultSet_Draw(b) == DRAW_SPRITE);
}

UInt8 DefaultSet_StepSound(BlockID b) {
	if (b == BLOCK_GLASS) return SOUND_STONE;
	if (b == BLOCK_ROPE) return SOUND_CLOTH;
	if (DefaultSet_Draw(b) == DRAW_SPRITE) return SOUND_NONE;
	return DefaultSet_DigSound(b);
}

UInt8 DefaultSet_Draw(BlockID b) {
	if (b == BLOCK_AIR) return DRAW_GAS;
	if (b == BLOCK_LEAVES) return DRAW_TRANSPARENT_THICK;

	if (b == BLOCK_ICE || b == BLOCK_WATER || b == BLOCK_STILL_WATER)
		return DRAW_TRANSLUCENT;
	if (b == BLOCK_GLASS || b == BLOCK_LEAVES)
		return DRAW_TRANSPARENT;

	if (b >= BLOCK_DANDELION && b <= BLOCK_RED_SHROOM)
		return DRAW_SPRITE;
	if (b == BLOCK_SAPLING || b == BLOCK_ROPE || b == BLOCK_FIRE)
		return DRAW_SPRITE;
	return DRAW_OPAQUE;
}

UInt8 DefaultSet_DigSound(BlockID b) {
	if (b >= BLOCK_RED && b <= BLOCK_WHITE)
		return SOUND_CLOTH;
	if (b >= BLOCK_LIGHT_PINK && b <= BLOCK_TURQUOISE)
		return SOUND_CLOTH;
	if (b == BLOCK_IRON || b == BLOCK_GOLD)
		return SOUND_METAL;

	if (b == BLOCK_BOOKSHELF || b == BLOCK_WOOD || b == BLOCK_LOG || b == BLOCK_CRATE || b == BLOCK_FIRE)
		return SOUND_WOOD;

	if (b == BLOCK_ROPE) return SOUND_CLOTH;
	if (b == BLOCK_SAND) return SOUND_SAND;
	if (b == BLOCK_SNOW) return SOUND_SNOW;
	if (b == BLOCK_GLASS) return SOUND_GLASS;
	if (b == BLOCK_DIRT || b == BLOCK_GRAVEL)
		return SOUND_GRAVEL;

	if (b == BLOCK_GRASS || b == BLOCK_SAPLING || b == BLOCK_TNT || b == BLOCK_LEAVES || b == BLOCK_SPONGE)
		return SOUND_GRASS;

	if (b >= BLOCK_DANDELION && b <= BLOCK_RED_SHROOM)
		return SOUND_GRASS;
	if (b >= BLOCK_WATER && b <= BLOCK_STILL_LAVA)
		return SOUND_NONE;
	if (b >= BLOCK_STONE && b <= BLOCK_STONE_BRICK)
		return SOUND_STONE;
	return SOUND_NONE;
}