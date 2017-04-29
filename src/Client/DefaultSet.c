#include "DefaultSet.h"
#include "Block.h"

Real32 DefaultSet_Height(BlockID b) {
	if (b == Block_Slab) return 0.5f;
	if (b == Block_CobblestoneSlab) return 0.5f;
	if (b == Block_Snow) return 0.25f;
	return 1.0f;
}

bool DefaultSet_FullBright(BlockID b) {
	return b == Block_Lava || b == Block_StillLava
		|| b == Block_Magma || b == Block_Fire;
}

Real32 DefaultSet_FogDensity(BlockID b) {
	if (b == Block_Water || b == Block_StillWater)
		return 0.1f;
	if (b == Block_Lava || b == Block_StillLava)
		return 2.0f;
	return 0.0f;
}

FastColour DefaultSet_FogColour(BlockID b) {
	if (b == Block_Water || b == Block_StillWater)
		return FastColour_Create3(5, 5, 51);
	if (b == Block_Lava || b == Block_StillLava)
		return FastColour_Create3(153, 25, 0);
	return FastColour_Create4(0, 0, 0, 0);
}

UInt8 DefaultSet_Collide(BlockID b) {
	if (b == Block_Ice) return CollideType_Ice;
	if (b == Block_Water || b == Block_StillWater)
		return CollideType_LiquidWater;
	if (b == Block_Lava || b == Block_StillLava)
		return CollideType_LiquidLava;

	if (b == Block_Snow || b == Block_Air || DefaultSet_Draw(b) == DrawType_Sprite)
		return CollideType_Gas;
	return CollideType_Solid;
}

UInt8 DefaultSet_MapOldCollide(BlockID b, UInt8 collide) {
		if (b == Block_Ice && collide == CollideType_Solid)
			return CollideType_Ice;
		if ((b == Block_Water || b == Block_StillWater) && collide == CollideType_Liquid)
			return CollideType_LiquidWater;
		if ((b == Block_Lava || b == Block_StillLava) && collide == CollideType_Liquid)
			return CollideType_LiquidLava;
		return collide;
	}

bool DefaultSet_BlocksLight(BlockID b) {
		return !(b == Block_Glass || b == Block_Leaves
			|| b == Block_Air || Draw(b) == DrawType_Sprite);
	}

UInt8 DefaultSet_StepSound(BlockID b) {
	if (b == Block_Glass) return SoundType_Stone;
	if (b == Block_Rope) return SoundType_Cloth;
	if (Draw(b) == DrawType_Sprite) return SoundType_None;
	return DigSound(b);
}


UInt8 DefaultSet_Draw(BlockID b) {
	if (b == Block_Air || b == Block_Invalid) return DrawType_Gas;
	if (b == Block_Leaves) return DrawType_TransparentThick;

	if (b == Block_Ice || b == Block_Water || b == Block_StillWater)
		return DrawType_Translucent;
	if (b == Block_Glass || b == Block_Leaves)
		return DrawType_Transparent;

	if (b >= Block_Dandelion && b <= Block_RedMushroom)
		return DrawType_Sprite;
	if (b == Block_Sapling || b == Block_Rope || b == Block_Fire)
		return DrawType_Sprite;
	return DrawType_Opaque;
}

UInt8 DefaultSet_DigSound(BlockID b) {
	if (b >= Block_Red && b <= Block_White)
		return SoundType_Cloth;
	if (b >= Block_LightPink && b <= Block_Turquoise)
		return SoundType_Cloth;
	if (b == Block_Iron || b == Block_Gold)
		return SoundType_Metal;

	if (b == Block_Bookshelf || b == Block_Wood
		|| b == Block_Log || b == Block_Crate || b == Block_Fire)
		return SoundType_Wood;

	if (b == Block_Rope) return SoundType_Cloth;
	if (b == Block_Sand) return SoundType_Sand;
	if (b == Block_Snow) return SoundType_Snow;
	if (b == Block_Glass) return SoundType_Glass;
	if (b == Block_Dirt || b == Block_Gravel)
		return SoundType_Gravel;

	if (b == Block_Grass || b == Block_Sapling || b == Block_TNT
		|| b == Block_Leaves || b == Block_Sponge)
		return SoundType_Grass;

	if (b >= Block_Dandelion && b <= Block_RedMushroom)
		return SoundType_Grass;
	if (b >= Block_Water && b <= Block_StillLava)
		return SoundType_None;
	if (b >= Block_Stone && b <= Block_StoneBrick)
		return SoundType_Stone;
	return SoundType_None;
}