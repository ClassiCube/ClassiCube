#ifndef CS_CHUNKINFO_H
#define CS_CHUNKINFO_H
#include "Typedefs.h"
/* Describes data necessary for rendering a chunk.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct ChunkPartInfo_ {
	Int32 VbId, IndicesCount, SpriteCount;
	UInt16 XMinCount, XMaxCount, ZMinCount,
		ZMaxCount, YMinCount, YMaxCount;
} ChunkPartInfo;

typedef struct ChunkInfo_ {
	/* Centre coordinates of the chunk. */
	UInt16 CentreX, CentreY, CentreZ;

	/* Whether chunk is visibile to the player. */
	UInt8 Visible : 1;
	/* Whether the chunk is empty of data. */
	UInt8 Empty : 1;
	/* Whether chunk is pending deletion.*/
	UInt8 PendingDelete : 1;
	/* Whether chunk is completely air. */
	UInt8 AllAir : 1;
	UInt8 : 0; /* pad to next byte*/

	UInt8 DrawXMin : 1;
	UInt8 DrawXMax : 1;
	UInt8 DrawZMin : 1;
	UInt8 DrawZMax : 1;
	UInt8 DrawYMin : 1;
	UInt8 DrawYMax : 1;
	UInt8 : 0; /* pad to next byte */
#if OCCLUSION
	public bool Visited = false, Occluded = false;
	public byte OcclusionFlags, OccludedFlags, DistanceFlags;
#endif

	ChunkPartInfo* NormalParts;
	ChunkPartInfo* TranslucentParts;
} ChunkInfo;

/* Resets contents of given chunk render info structure. */
void ChunkInfo_Reset(ChunkInfo* chunk, Int32 x, Int32 y, Int32 z);
#endif