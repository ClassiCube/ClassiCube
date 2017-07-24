#include "ChunkSorter.h"
#include "ChunkUpdater.h"
#include "MapRenderer.h"
#include "GameProps.h"
#include "Vectors.h"
#include "Vector3I.h"
#include "Constants.h"

void ChunkSorter_UpdateSortOrder(void) {
	Vector3 cameraPos = Game_CurrentCameraPos;
	Vector3I newChunkPos;
	Vector3I_Floor(&newChunkPos, &cameraPos);

	newChunkPos.X = (newChunkPos.X & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	newChunkPos.Y = (newChunkPos.Y & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	newChunkPos.Z = (newChunkPos.Z & ~CHUNK_MAX) + HALF_CHUNK_SIZE;
	/* Same chunk, therefore don't need to recalculate sort order. */
	if (Vector3I_Equals(&newChunkPos, &ChunkUpdater_ChunkPos)) return;

	Vector3I pPos = newChunkPos;
	ChunkUpdater_ChunkPos = pPos;
	if (MapRenderer_ChunksCount == 0) return;

	Int32 i = 0;
	for (i = 0; i < MapRenderer_ChunksCount; i++) {
		ChunkInfo* info = MapRenderer_SortedChunks[i];

		/* Calculate distance to chunk centre*/
		Int32 dx = info->CentreX - pPos.X, dy = info->CentreY - pPos.Y, dz = info->CentreZ - pPos.Z;
		ChunkUpdater_Distances[i] = dx * dx + dy * dy + dz * dz; /* TODO: do we need to cast to unsigned for the mulitplies? */

		/* Can work out distance to chunk faces as offset from distance to chunk centre on each axis. */
		Int32 dXMin = dx - HALF_CHUNK_SIZE, dXMax = dx + HALF_CHUNK_SIZE;
		Int32 dYMin = dy - HALF_CHUNK_SIZE, dYMax = dy + HALF_CHUNK_SIZE;
		Int32 dZMin = dz - HALF_CHUNK_SIZE, dZMax = dz + HALF_CHUNK_SIZE;

		/* Back face culling: make sure that the chunk is definitely entirely back facing. */
		info->DrawXMin = !(dXMin <= 0 && dXMax <= 0);
		info->DrawXMax = !(dXMin >= 0 && dXMax >= 0);
		info->DrawZMin = !(dZMin <= 0 && dZMax <= 0);
		info->DrawZMax = !(dZMin >= 0 && dZMax >= 0);
		info->DrawYMin = !(dYMin <= 0 && dYMax <= 0);
		info->DrawYMax = !(dYMin >= 0 && dYMax >= 0);
	}

	ChunkSorter_QuickSort(0, MapRenderer_ChunksCount - 1);
	ChunkUpdater_ResetPartFlags();
	//SimpleOcclusionCulling();
}

void ChunkSorter_QuickSort(Int32 left, Int32 right) {
	ChunkInfo** values = MapRenderer_SortedChunks;
	Int32* keys = ChunkUpdater_Distances;
	while (left < right) {
		Int32 i = left, j = right;
		Int32 pivot = keys[(i + j) / 2];
		/* partition the list */
		while (i <= j) {
			while (pivot > keys[i]) i++;
			while (pivot < keys[j]) j--;

			if (i <= j) {
				Int32 key = keys[i]; keys[i] = keys[j]; keys[j] = key;
				ChunkInfo* value = values[i]; values[i] = values[j]; values[j] = value;
				i++; j--;
			}
		}

		/* recurse into the smaller subset */
		if (j - left <= right - i) {
			if (left < j) ChunkSorter_QuickSort(left, j);
			left = i;
		} else {
			if (i < right) ChunkSorter_QuickSort(i, right);
			right = j;
		}
	}
}