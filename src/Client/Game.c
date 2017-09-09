#include "Game.h"
#include "Block.h"

bool Game_CanPick(BlockID block) {
	if (Block_Draw[block] == DrawType_Gas)    return false;
	if (Block_Draw[block] == DrawType_Sprite) return true;

	if (Block_Collide[block] != CollideType_Liquid) return true;
	return Game_ModifiableLiquids && Block_CanPlace[block] && Block_CanDelete[block];
}