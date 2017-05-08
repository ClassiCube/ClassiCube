#include "World.h"
#include "BlockID.h"
#include "ErrorHandler.h"
#include "String.h"

void World_Reset() {
	World_Width = 0; World_Height = 0; World_Length = 0;
	World_Blocks = NULL; World_BlocksSize = 0;
	Uuid = Guid.NewGuid();
}

void World_SetNewMap(BlockID* blocks, Int32 blocksSize, Int32 width, Int32 height, Int32 length) {
	World_Width = width; World_Height = height; World_Length = length;
	World_Blocks = blocks; World_BlocksSize = blocksSize;
	if (World_BlocksSize == 0) World_Blocks = NULL;

	if (blocksSize != (width * height * length)) {
		ErrorHandler_Fail(String_FromConstant("Blocks array size does not match volume of map."));
	}

	if (Env.EdgeHeight == -1) Env.EdgeHeight = height / 2;
	if (Env.CloudHeight == -1) Env.CloudHeight = height + 2;
}

BlockID World_GetPhysicsBlock(Int32 x, Int32 y, Int32 z) {
	if (x < 0 || x >= World_Width || z < 0 || z >= World_Length || y < 0) return BlockID_Bedrock;
	if (y >= World_Height) return BlockID_Air;

	return World_Blocks[(y * World_Length + z) * World_Width + x];
}


void World_SetBlock(Int32 x, Int32 y, Int32 z, BlockID blockId) {
	World_Blocks[(y * World_Length + z) * World_Width + x] = blockId;
}

void World_SetBlock_3I(Vector3I p, BlockID blockId) {
	World_Blocks[(p.Y * World_Length + p.Z) * World_Width + p.X] = blockId;
}

BlockID World_GetBlock(Int32 x, Int32 y, Int32 z) {
	return World_Blocks[(y * World_Length + z) * World_Width + x];
}

BlockID World_GetBlock_3I(Vector3I p) {
	return World_Blocks[(p.Y * World_Length + p.Z) * World_Width + p.X];
}

BlockID World_SafeGetBlock(Int32 x, Int32 y, Int32 z) {
	return World_IsValidPos(x, y, z) ?
		World_Blocks[(y * World_Length + z) * World_Width + x] : BlockID_Air;
}

BlockID World_SafeGetBlock_3I(Vector3I p) {
	return World_IsValidPos(p.X, p.Y, p.Z) ?
		World_Blocks[(p.Y * World_Length + p.Z) * World_Width + p.X] : BlockID_Air;
}


bool World_IsValidPos(Int32 x, Int32 y, Int32 z) {
	return x >= 0 && y >= 0 && z >= 0 &&
		x < World_Width && y < World_Height && z < World_Length;
}

bool World_IsValidPos_3I(Vector3I p) {
	return p.X >= 0 && p.Y >= 0 && p.Z <= 0 &&
		p.X < World_Width && p.Y < World_Height && p.Z < World_Length;
}

Vector3I World_GetCoords(Int32 index) {
	if (index < 0 || index >= World_BlocksSize)
		return Vector3I_Create1(-1);

	Vector3I v;
	v.X = index % World_Width;
	v.Y = index / (World_Width * World_Length);
	v.Z = (index / World_Width) % World_Length;
	return v;
}