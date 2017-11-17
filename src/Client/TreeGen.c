#include "TreeGen.h"
#include "BlockID.h"
#include "ExtMath.h"

bool TreeGen_CanGrow(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 treeHeight) {
	/* check tree base */
	Int32 baseHeight = treeHeight - 4;
	Int32 x, y, z;

	for (y = treeY; y < treeY + baseHeight; y++) {
		for (z = treeZ - 1; z <= treeZ + 1; z++) {
			for (x = treeX - 1; x <= treeX + 1; x++)
			{
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;
				Int32 index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}

	/* and also check canopy */
	for (y = treeY + baseHeight; y < treeY + treeHeight; y++) {
		for (z = treeZ - 2; z <= treeZ + 2; z++) {
			for (x = treeX - 2; x <= treeX + 2; x++) {
				if (x < 0 || y < 0 || z < 0 || x >= Tree_Width || y >= Tree_Height || z >= Tree_Length)
					return false;
				Int32 index = Tree_Pack(x, y, z);
				if (Tree_Blocks[index] != BLOCK_AIR) return false;
			}
		}
	}
	return true;
}

#define TreeGen_Place(x, y, z, block)\
coords[count].X = (x); coords[count].Y = (y); coords[count].Z = (z);\
blocks[count] = block;\
count++;

Int32 TreeGen_Grow(Int32 treeX, Int32 treeY, Int32 treeZ, Int32 height, Vector3I* coords, BlockID* blocks) {
	Int32 baseHeight = height - 4;
	Int32 count = 0;
	Int32 xx, y, zz;

	/* leaves bottom layer */
	for (y = treeY + baseHeight; y < treeY + baseHeight + 2; y++) {
		for (zz = -2; zz <= 2; zz++) {
			for (xx = -2; xx <= 2; xx++) {
				Int32 x = treeX + xx, z = treeZ + zz;

				if (Math_AbsI(xx) == 2 && Math_AbsI(zz) == 2) {
					if (Random_Float(Tree_Rnd) >= 0.5f) {
						TreeGen_Place(x, y, z, BLOCK_LEAVES);
					}
				} else {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* leaves top layer */
	Int32 bottomY = treeY + baseHeight + 2;
	for (y = treeY + baseHeight + 2; y < treeY + height; y++) {
		for (zz = -1; zz <= 1; zz++) {
			for (xx = -1; xx <= 1; xx++) {
				Int32 x = xx + treeX, z = zz + treeZ;

				if (xx == 0 || zz == 0) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				} else if (y == bottomY && Random_Float(Tree_Rnd) >= 0.5f) {
					TreeGen_Place(x, y, z, BLOCK_LEAVES);
				}
			}
		}
	}

	/* then place trunk */
	for (y = 0; y < height - 1; y++) {
		TreeGen_Place(treeX, treeY + y, treeZ, BLOCK_LOG);
	}
	return count;
}