#include "Game.h"
#include "Block.h"
#include "World.h"
#include "WeatherRenderer.h"
#include "Lighting.h"
#include "MapRenderer.h"
#include "GraphicsAPI.h"
#include "Camera.h"
#include "Options.h"
#include "Funcs.h"

void Game_UpdateProjection(void) {
	Game_DefaultFov = Options_GetInt(OptionsKey_FieldOfView, 1, 150, 70);
	Camera_ActiveCamera->GetProjection(&Game_Projection);

	Gfx_SetMatrixMode(MatrixType_Projection);
	Gfx_LoadMatrix(&Game_Projection);
	Gfx_SetMatrixMode(MatrixType_Modelview);
	Event_RaiseVoid(&GfxEvents_ProjectionChanged);
}

void Game_UpdateBlock(Int32 x, Int32 y, Int32 z, BlockID block) {
	BlockID oldBlock = World_GetBlock(x, y, z);
	World_SetBlock(x, y, z, block);

	if (Weather_Heightmap != NULL) {
		WeatherRenderer_OnBlockChanged(x, y, z, oldBlock, block);
	}
	Lighting_OnBlockChanged(x, y, z, oldBlock, block);

	/* Refresh the chunk the block was located in. */
	Int32 cx = x >> 4, cy = y >> 4, cz = z >> 4;
	ChunkInfo* chunk = MapRenderer_GetChunk(cx, cy, cz);
	chunk->AllAir &= Block_Draw[block] == DrawType_Gas;
	MapRenderer_RefreshChunk(cx, cy, cz);
}

void Game_SetViewDistance(Real32 distance, bool userDist) {
	if (userDist) {
		Game_UserViewDistance = distance;
		Options_SetInt32(OptionsKey_ViewDist, (Int32)distance);
	}

	distance = min(distance, Game_MaxViewDistance);
	if (distance == Game_ViewDistance) return;
	Game_ViewDistance = distance;

	Event_RaiseVoid(&GfxEvents_ViewDistanceChanged);
	Game_UpdateProjection();
}

bool Game_CanPick(BlockID block) {
	if (Block_Draw[block] == DrawType_Gas)    return false;
	if (Block_Draw[block] == DrawType_Sprite) return true;

	if (Block_Collide[block] != CollideType_Liquid) return true;
	return Game_ModifiableLiquids && Block_CanPlace[block] && Block_CanDelete[block];
}