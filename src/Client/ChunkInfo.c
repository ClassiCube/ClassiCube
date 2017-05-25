#include "ChunkInfo.h"

void ChunkInfo_Reset(ChunkInfo* chunk, Int32 x, Int32 y, Int32 z) {
	chunk->CentreX = (UInt16)(x + 8);
	chunk->CentreY = (UInt16)(y + 8);
	chunk->CentreZ = (UInt16)(z + 8);

	chunk->Visible = true; chunk->Empty = false; 
	chunk->PendingDelete = false; chunk->AllAir = false;
	chunk->DrawXMin = false; chunk->DrawXMax = false; chunk->DrawZMin = false;
	chunk->DrawZMax = false; chunk->DrawYMin = false; chunk->DrawYMax = false;
}