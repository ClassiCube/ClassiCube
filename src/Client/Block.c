#include "Block.h"
#include "DefaultSet.h"
#include "Funcs.h"

void Block_Reset(Game* game) {
	Init();
	// TODO: Make this part of TerrainAtlas2D maybe?
	using (FastBitmap fastBmp = new FastBitmap(game.TerrainAtlas.AtlasBitmap, true, true))
		RecalculateSpriteBB(fastBmp);
}

void Block_Init() {
	for (Int32 i = 0; i < DefinedCustomBlocks.Length; i++)
		DefinedCustomBlocks[i] = 0;
	for (Int32 block = 0; block < Block_Count; block++)
		Block_ResetProps((BlockID)block);
	UpdateCulling();
}

void Block_SetDefaultPerms(InventoryPermissions* place, InventoryPermissions* del) {
	for (Int32 block = BlockID_Stone; block <= Block_MaxDefined; block++) {
		place[block] = true;
		del[block] = true;
	}

	place[BlockID_Lava] = false; del[BlockID_Lava] = false;
	place[BlockID_Water] = false; del[BlockID_Water] = false;
	place[BlockID_StillLava] = false; del[BlockID_StillLava] = false;
	place[BlockID_StillWater] = false; del[BlockID_StillWater] = false;
	place[BlockID_Bedrock] = false; del[BlockID_Bedrock] = false;
}

void Block_SetCollide(BlockID block, UInt8 collide) {
	// necessary for cases where servers redefined core blocks before extended types were introduced
	collide = DefaultSet_MapOldCollide(block, collide);
	ExtendedCollide[block] = collide;

	// Reduce extended collision types to their simpler forms
	if (collide == CollideType_Ice) collide = CollideType_Solid;
	if (collide == CollideType_SlipperyIce) collide = CollideType_Solid;

	if (collide == CollideType_LiquidWater) collide = CollideType_Liquid;
	if (collide == CollideType_LiquidLava) collide = CollideType_Liquid;
	Collide[block] = collide;
}

void Block_SetDrawType(BlockID block, UInt8 draw) {
	if (draw == DrawType_Opaque && Collide[block] != CollideType_Solid)
		draw = DrawType_Transparent;
	Draw[block] = draw;

	FullOpaque[block] = draw == DrawType_Opaque
		&& MinBB[block] == Vector3.Zero && MaxBB[block] == Vector3.One;
}

void Block_ResetProps(BlockID block) {
	BlocksLight[block] = DefaultSet_BlocksLight(block);
	FullBright[block] = DefaultSet_FullBright(block);
	FogColour[block] = DefaultSet_FogColour(block);
	FogDensity[block] = DefaultSet_FogDensity(block);
	Block_SetCollide(block, DefaultSet_Collide(block));
	DigSounds[block] = DefaultSet_DigSound(block);
	StepSounds[block] = DefaultSet_StepSound(block);
	SpeedMultiplier[block] = 1;
	Name[block] = Block_DefaultName(block);
	Tinted[block] = false;

	Draw[block] = DefaultSet_Draw(block);
	if (Draw[block] == DrawType_Sprite) {
		MinBB[block] = new Vector3(2.50f / 16f, 0, 2.50f / 16f);
		MaxBB[block] = new Vector3(13.5f / 16f, 1, 13.5f / 16f);
	}
	else {
		MinBB[block] = Vector3.Zero;
		MaxBB[block] = Vector3.One;
		MaxBB[block].Y = DefaultSet_Height(block);
	}

	Block_SetDrawType(block, Draw[block]);
	CalcRenderBounds(block);
	LightOffset[block] = CalcLightOffset(block);

	if (block >= Block_CpeCount) {
#if USE16_BIT
		// give some random texture ids
		SetTex((block * 10 + (block % 7) + 20) % 80, Side.Top, block);
		SetTex((block * 8 + (block & 5) + 5) % 80, Side.Bottom, block);
		SetSide((block * 4 + (block / 4) + 4) % 80, block);
#else
		SetTex(0, Side.Top, block);
		SetTex(0, Side.Bottom, block);
		SetSide(0, block);
#endif
	} else {
		SetTex(topTex[block], Side.Top, block);
		SetTex(bottomTex[block], Side.Bottom, block);
		SetSide(sideTex[block], block);
	}
}

Int32 Block_FindID(String* name) {
	for (Int32 i = 0; i < Block_Count; i++) {
		if (String_CaselessEquals(&Name[i], name)) return i;
	}
	return -1;
}


String Block_DefaultName(BlockID block) {
#if USE16_BIT
	if (block >= 256) return "ID " + block;
#endif
	if (block >= Block_CpeCount) {
		String str;
		String_Constant(&str, "Invalid");
		return str;
	}

	String blockNames;
	String_Constant(&blockNames, Block_RawNames);

	// Find start and end of this particular block name
	Int32 start = 0;
	for (Int32 i = 0; i < block; i++) {
		start = String_IndexOf(&blockNames, ' ', start) + 1;
	}
	Int32 end = String_IndexOf(&blockNames, ' ', start);
	if (end == -1) end = blockNames.length;

	String buffer;
	String_Empty(&buffer, Block_NamePtr(block), STRING_SIZE);
	Block_SplitUppercase(&buffer, &blockNames, start, end);
	return buffer;
}

static void Block_SplitUppercase(String* buffer, String* blockNames, Int32 start, Int32 end) {
	Int32 index = 0;
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