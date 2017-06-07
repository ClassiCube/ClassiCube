#include "ChunkSorter.h"
#include "ChunkUpdater.h"
#include "MapRenderer.h"
#include "GameProps.h"
#include "Vectors.h"
#include "Vector3I.h"

void ChunkSorter_UpdateSortOrder(void) {
	Vector3 cameraPos = Game_CurrentCameraPos;
	Vector3I newChunkPos = Vector3I.Floor(cameraPos);
	newChunkPos.X = (newChunkPos.X & ~0x0F) + 8;
	newChunkPos.Y = (newChunkPos.Y & ~0x0F) + 8;
	newChunkPos.Z = (newChunkPos.Z & ~0x0F) + 8;
	if (newChunkPos == updater.chunkPos) return;

	ChunkInfo** chunks = MapRenderer_SortedChunks;
	UInt32* distances = updater.distances;
	Vector3I pPos = newChunkPos;
	updater.chunkPos = pPos;

	if (MapRenderer_ChunksCount == 0) return;

	Int32 i = 0;
	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkInfo* info = chunks[i];
		distances[i] = Utils.DistanceSquared(info.CentreX, info.CentreY, info.CentreZ,
			pPos.X, pPos.Y, pPos.Z);

		Int32 dX1 = (info->CentreX - 8) - pPos.X, dX2 = (info->CentreX + 8) - pPos.X;
		Int32 dY1 = (info->CentreY - 8) - pPos.Y, dY2 = (info->CentreY + 8) - pPos.Y;
		Int32 dZ1 = (info->CentreZ - 8) - pPos.Z, dZ2 = (info->CentreZ + 8) - pPos.Z;

		/* Back face culling: make sure that the chunk is definitely entirely back facing. */
		info->DrawXMin = !(dX1 <= 0 && dX2 <= 0);
		info->DrawXMax = !(dX1 >= 0 && dX2 >= 0);
		info->DrawZMin = !(dZ1 <= 0 && dZ2 <= 0);
		info->DrawZMax = !(dZ1 >= 0 && dZ2 >= 0);
		info->DrawYMin = !(dY1 <= 0 && dY2 <= 0);
		info->DrawYMax = !(dY1 >= 0 && dY2 >= 0);
	}

	ChunkSorter_QuickSort(0, MapRenderer_ChunksCount - 1);
	ChunkUpdater_ResetUsedFlags();
	//SimpleOcclusionCulling();
}

void ChunkSorter_QuickSort(Int32 left, Int32 right) {
	ChunkInfo** values = MapRenderer_SortedChunks;
	UInt32* keys = ChunkUpdater_Distances;
	while (left < right) {
		Int32 i = left, j = right;
		UInt32 pivot = keys[(i + j) / 2];
		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;

			if (i <= j) {
				UInt32 key = keys[i]; keys[i] = keys[j]; keys[j] = key;
				ChunkInfo* value = values[i]; values[i] = values[j]; values[j] = value;
				i++; j--;
			}
		}

		/* recurse into the smaller subset */
		if (j - left <= right - i) {
			if (left < j) QuickSort(left, j);
			left = i;
		} else {
			if (i < right) QuickSort(i, right);
			right = j;
		}
	}
}