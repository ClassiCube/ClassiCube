#include "DefaultSet.h"
#include "BlockID.h"
#include "Block.h"

Real32 DefaultSet_Height(BlockID b) {
	if (b == BlockID_Slab) return 0.5f;
	if (b == BlockID_CobblestoneSlab) return 0.5f;
	if (b == BlockID_Snow) return 0.25f;
	return 1.0f;
}

bool DefaultSet_FullBright(BlockID b) {
	return b == BlockID_Lava || b == BlockID_StillLava
		|| b == BlockID_Magma || b == BlockID_Fire;
}

Real32 DefaultSet_FogDensity(BlockID b) {
	if (b == BlockID_Water || b == BlockID_StillWater)
		return 0.1f;
	if (b == BlockID_Lava || b == BlockID_StillLava)
		return 1.8f;
	return 0.0f;
}

PackedCol DefaultSet_FogColour(BlockID b) {
	if (b == BlockID_Water || b == BlockID_StillWater)
		return PackedCol_Create3(5, 5, 51);
	if (b == BlockID_Lava || b == BlockID_StillLava)
		return PackedCol_Create3(153, 25, 0);
	return PackedCol_Create4(0, 0, 0, 0);
}

CollideType DefaultSet_Collide(BlockID b) {
	if (b == BlockID_Ice) return CollideType_Ice;
	if (b == BlockID_Water || b == BlockID_StillWater)
		return CollideType_LiquidWater;
	if (b == BlockID_Lava || b == BlockID_StillLava)
		return CollideType_LiquidLava;

	if (b == BlockID_Snow || b == BlockID_Air || DefaultSet_Draw(b) == DrawType_Sprite)
		return CollideType_Gas;
	return CollideType_Solid;
}

CollideType DefaultSet_MapOldCollide(BlockID b, CollideType collide) {
	if (b == BlockID_Ice && collide == CollideType_Solid)
		return CollideType_Ice;
	if ((b == BlockID_Water || b == BlockID_StillWater) && collide == CollideType_Liquid)
		return CollideType_LiquidWater;
	if ((b == BlockID_Lava || b == BlockID_StillLava) && collide == CollideType_Liquid)
		return CollideType_LiquidLava;
	return collide;
}

bool DefaultSet_BlocksLight(BlockID b) {
	return !(b == BlockID_Glass || b == BlockID_Leaves
		|| b == BlockID_Air || DefaultSet_Draw(b) == DrawType_Sprite);
}

SoundType DefaultSet_StepSound(BlockID b) {
	if (b == BlockID_Glass) return SoundType_Stone;
	if (b == BlockID_Rope) return SoundType_Cloth;
	if (DefaultSet_Draw(b) == DrawType_Sprite) return SoundType_None;
	return DefaultSet_DigSound(b);
}

DrawType DefaultSet_Draw(BlockID b) {
	if (b == BlockID_Air || b == BlockID_Invalid) return DrawType_Gas;
	if (b == BlockID_Leaves) return DrawType_TransparentThick;

	if (b == BlockID_Ice || b == BlockID_Water || b == BlockID_StillWater)
		return DrawType_Translucent;
	if (b == BlockID_Glass || b == BlockID_Leaves)
		return DrawType_Transparent;

	if (b >= BlockID_Dandelion && b <= BlockID_RedMushroom)
		return DrawType_Sprite;
	if (b == BlockID_Sapling || b == BlockID_Rope || b == BlockID_Fire)
		return DrawType_Sprite;
	return DrawType_Opaque;
}

SoundType DefaultSet_DigSound(BlockID b) {
	if (b >= BlockID_Red && b <= BlockID_White)
		return SoundType_Cloth;
	if (b >= BlockID_LightPink && b <= BlockID_Turquoise)
		return SoundType_Cloth;
	if (b == BlockID_Iron || b == BlockID_Gold)
		return SoundType_Metal;

	if (b == BlockID_Bookshelf || b == BlockID_Wood
		|| b == BlockID_Log || b == BlockID_Crate || b == BlockID_Fire)
		return SoundType_Wood;

	if (b == BlockID_Rope) return SoundType_Cloth;
	if (b == BlockID_Sand) return SoundType_Sand;
	if (b == BlockID_Snow) return SoundType_Snow;
	if (b == BlockID_Glass) return SoundType_Glass;
	if (b == BlockID_Dirt || b == BlockID_Gravel)
		return SoundType_Gravel;

	if (b == BlockID_Grass || b == BlockID_Sapling || b == BlockID_TNT
		|| b == BlockID_Leaves || b == BlockID_Sponge)
		return SoundType_Grass;

	if (b >= BlockID_Dandelion && b <= BlockID_RedMushroom)
		return SoundType_Grass;
	if (b >= BlockID_Water && b <= BlockID_StillLava)
		return SoundType_None;
	if (b >= BlockID_Stone && b <= BlockID_StoneBrick)
		return SoundType_Stone;
	return SoundType_None;
}