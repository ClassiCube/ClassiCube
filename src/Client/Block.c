#include "DefaultSet.h"
#include "Funcs.h"
#include "ExtMath.h"
#include "Block.h"

void Block_Reset(Game* game) {
	Block_Init();
	/* TODO: Make this part of TerrainAtlas2D maybe? */
	/* Block_RecalculateSpriteBB(game->TerrainAtlas.AtlasBitmap); */
}

void Block_Init() {
	#define DefinedCustomBlocks_Len (Block_Count >> 5)
	for (Int32 i = 0; i < DefinedCustomBlocks_Len; i++) {
		DefinedCustomBlocks[i] = 0;
	}
	for (Int32 block = 0; block < Block_Count; block++) {
		Block_ResetProps((BlockID)block);
	}
	Block_UpdateCullingAll();
}

void Block_SetDefaultPerms() {
	for (Int32 block = BlockID_Air; block <= Block_MaxDefined; block++) {
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

	if (block >= Block_CpeCount) {
#if USE16_BIT
		/* give some random texture ids */
		Block_SetTex((block * 10 + (block % 7) + 20) % 80, Side_Top, block);
		Block_SetTex((block * 8 + (block & 5) + 5) % 80, Side_Bottom, block);
		Block_SetSide((block * 4 + (block / 4) + 4) % 80, block);
#else
		Block_SetTex(0, Side_Top, block);
		Block_SetTex(0, Side_Bottom, block);
		Block_SetSide(0, block);
#endif
	} else {
		Block_SetTex(topTex[block], Side_Top, block);
		Block_SetTex(bottomTex[block], Side_Bottom, block);
		Block_SetSide(sideTex[block], block);
	}
}

Int32 Block_FindID(String* name) {
	Int32 block;
	for (block = BlockID_Air; block < Block_Count; block++) {
		if (String_CaselessEquals(&Block_Name[block], name)) return i;
	}
	return -1;
}

bool Block_IsLiquid(BlockID b) { return b >= BlockID_Water && b <= BlockID_StillLava; }



String Block_DefaultName(BlockID block) {
#if USE16_BIT
	if (block >= 256) return "ID " + block;
#endif
	if (block >= Block_CpeCount) {
		return String_FromConstant("Invalid");
	}

	/* TODO: how much performance impact here. */
	String blockNames = String_FromConstant(Block_RawNames);

	/* Find start and end of this particular block name. */
	Int32 start = 0;
	for (Int32 i = 0; i < block; i++) {
		start = String_IndexOf(&blockNames, ' ', start) + 1;
	}
	Int32 end = String_IndexOf(&blockNames, ' ', start);
	if (end == -1) end = blockNames.length;

	String buffer = String_FromRawBuffer(Block_NamePtr(block), STRING_SIZE);
	Block_SplitUppercase(&buffer, &blockNames, start, end);
	return buffer;
}

static void Block_SplitUppercase(String* buffer, String* blockNames, Int32 start, Int32 end) {
	for (Int32 i = start; i < end; i++) {
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



void Block_SetSide(Int32 textureId, BlockID blockId) {
	Int32 index = blockId * Side_Sides;
	Block_Textures[index + Side_Left] = (UInt8)textureId;
	Block_Textures[index + Side_Right] = (UInt8)textureId;
	Block_Textures[index + Side_Front] = (UInt8)textureId;
	Block_Textures[index + Side_Back] = (UInt8)textureId;
}

void Block_SetTex(Int32 textureId, Int32 face, BlockID blockId) {
	Block_Textures[blockId * Side_Sides + face] = (UInt8)textureId;
}

UInt8 Block_GetTextureLoc(BlockID block, Int32 face) {
	return Block_Textures[block * Side_Sides + face];
}

void Block_GetTextureRegion(BlockID block, Int32 side, Vector2* min, Vector2* max) {
	*min = Vector2_Zero; *max = Vector2_One;
	Vector3 bbMin = Block_MinBB[block], bbMax = Block_MaxBB[block];

	switch (side) {
	case Side_Left:
	case Side_Right:
		*min = Vector2_Create2(bbMin.Z, bbMin.Y);
		*max = Vector2_Create2(bbMax.Z, bbMax.Y);
		if (Block_IsLiquid(block)) max->Y -= 1.5f / 16.0f;
		break;

	case Side_Front:
	case Side_Back:
		*min = Vector2_Create2(bbMin.X, bbMin.Y);
		*max = Vector2_Create2(bbMax.X, bbMax.Y);
		if (Block_IsLiquid(block)) max->Y -= 1.5f / 16.0f;
		break;

	case Side_Top:
	case Side_Bottom:
		*min = Vector2_Create2(bbMin.X, bbMin.Z);
		*max = Vector2_Create2(bbMax.X, bbMax.Z);
		break;
	}
}

bool Block_FaceOccluded(BlockID block, BlockID other, Int32 side) {
	Vector2 bMin, bMax, oMin, oMax;
	Block_GetTextureRegion(block, side, &bMin, &bMax);
	Block_GetTextureRegion(other, side, &oMin, &oMax);

	return bMin.X >= oMin.X && bMin.Y >= oMin.Y
		&& bMax.X <= oMax.X && bMax.Y <= oMax.Y;
}



void Block_CalcRenderBounds(BlockID block) {
	Vector3 min = Block_MinBB[block], max = Block_MaxBB[block];

	if (block >= BlockID_Water && block <= BlockID_StillLava) {
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

	if (min.X != 0) flags &= ~(1 << Side_Left);
	if (max.X != 1) flags &= ~(1 << Side_Right);
	if (min.Z != 0) flags &= ~(1 << Side_Front);
	if (max.Z != 1) flags &= ~(1 << Side_Back);

	if ((min.Y != 0 && max.Y == 1) && Block_Draw[block] != DrawType_Gas) {
		flags &= ~(1 << Side_Top);
		flags &= ~(1 << Side_Bottom);
	}
	return (UInt8)flags;
}

void Block_RecalculateSpriteBB(Bitmap* bmp) {
	Int32 block;
	for (block = BlockID_Air; block < Block_Count; block++) {
		if (Block_Draw[block] != DrawType_Sprite) continue;
		Block_RecalculateBB((BlockID)block, bmp);
	}
}

void Block_RecalculateBB(BlockID block, Bitmap* bmp) {
	Int32 elemSize = bmp->Width / 16;
	Int32 texId = Block_GetTextureLoc(block, Side_Right);
	Int32 texX = texId & 0x0F, texY = texId >> 4;

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

Real32 Block_GetSpriteBB_TopY(Int32 size, Int32 tileX, Int32 tileY, Bitmap* bmp) {
	Int32 x, y;
	for (y = 0; y < size; y++) {
		UInt32* row = Bitmap_GetRow(bmp, tileY * size + y) + (tileX * size);
		for (x = 0; x < size; x++) {
			if ((UInt8)(row[x] >> 24) != 0) {
				return 1 - (float)y / size;
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
				return 1 - (float)(y + 1) / size;
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
				return (float)x / size;
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
				return (float)(x + 1) / size;
			}
		}
	}
	return 0;
}



void Block_UpdateCullingAll() {
	Int32 block, neighbour;
	for (block = BlockID_Air; block < Block_Count; block++)
		Block_CanStretch[block] = 0x3F;

	for (block = BlockID_Air; block < Block_Count; block++) {
		for (neighbour = BlockID_Air; neighbour < Block_Count; neighbour++) {
			Block_CalcCulling((BlockID)block, (BlockID)neighbour);
		}
	}
}

void Block_UpdateCulling(BlockID block) {
	Block_CanStretch[block] = 0x3F;

	Int32 other;
	for (other = BlockID_Air; other < Block_Count; other++) {
		Block_CalcCulling(block, (BlockID)other);
		Block_CalcCulling((BlockID)other, block);
	}
}

void Block_CalcCulling(BlockID block, BlockID other) {
	Vector3 bMin = Block_MinBB[block], bMax = Block_MaxBB[block];
	Vector3 oMin = Block_MinBB[other], oMax = Block_MaxBB[other];
	if (Block_IsLiquid(block)) bMax.Y -= 1.5f / 16;
	if (Block_IsLiquid(other)) oMax.Y -= 1.5f / 16;

	if (Block_Draw[block] == DrawType_Sprite) {
		Block_SetHidden(block, other, Side_Left, true);
		Block_SetHidden(block, other, Side_Right, true);
		Block_SetHidden(block, other, Side_Front, true);
		Block_SetHidden(block, other, Side_Back, true);
		Block_SetHidden(block, other, Side_Bottom, oMax.Y == 1);
		Block_SetHidden(block, other, Side_Top, bMax.Y == 1);
	} else {
		Block_SetXStretch(block, bMin.X == 0 && bMax.X == 1);
		Block_SetZStretch(block, bMin.Z == 0 && bMax.Z == 1);
		bool bothLiquid = Block_IsLiquid(block) && Block_IsLiquid(other);

		Block_SetHidden(block, other, Side_Left, oMax.X == 1 && bMin.X == 0);
		Block_SetHidden(block, other, Side_Right, oMin.X == 0 && bMax.X == 1);
		Block_SetHidden(block, other, Side_Front, oMax.Z == 1 && bMin.Z == 0);
		Block_SetHidden(block, other, Side_Back, oMin.Z == 0 && bMax.Z == 1);

		Block_SetHidden(block, other, Side_Bottom,
			bothLiquid || (oMax.Y == 1 && bMin.Y == 0));
		Block_SetHidden(block, other, Side_Top,
			bothLiquid || (oMin.Y == 0 && bMax.Y == 1));
	}
}

bool Block_IsHidden(BlockID block, BlockID other, Int32 side) {
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

void Block_SetHidden(BlockID block, BlockID other, Int32 side, bool value) {
	value = Block_IsHidden(block, other, side) && Block_FaceOccluded(block, other, side) && value;
	int bit = value ? 1 : 0;
	Block_Hidden[block * Block_Count + other] &= (UInt8)~(1 << side);
	Block_Hidden[block * Block_Count + other] |= (UInt8)(bit << side);
}

bool Block_IsFaceHidden(BlockID block, BlockID other, Int32 side) {
#if USE16_BIT
	return (hidden[(block << 12) | other] & (1 << side)) != 0;
#else
	return (Block_Hidden[(block << 8) | other] & (1 << side)) != 0;
#endif
}

void Block_SetXStretch(BlockID block, bool stretch) {
	Block_CanStretch[block] &= 0xC3; /* ~0x3C */
	Block_CanStretch[block] |= (stretch ? 0x3C : (UInt8)0);
}

void Block_SetZStretch(BlockID block, bool stretch) {
	Block_CanStretch[block] &= 0xFC; /* ~0x03 */
	Block_CanStretch[block] |= (stretch ? 0x03 : (UInt8)0);
}