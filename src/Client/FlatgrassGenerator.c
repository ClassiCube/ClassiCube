#include "FlatgrassGenerator.h"
#include "BlockID.h"
#include "Funcs.h"
#include "Platform.h"
#include "ErrorHandler.h"

void FlatgrassGen_Generate() {
	Gen_Blocks = Platform_MemAlloc(Gen_Width * Gen_Height * Gen_Length * sizeof(BlockID));
	if (Gen_Blocks == NULL)
		ErrorHandler_Fail("FlatgrassGen_Blocks - failed to allocate memory");

	Gen_CurrentState = String_FromConstant("Setting dirt blocks");
	FlatgrassGen_MapSet(0, Gen_Height / 2 - 2, BlockID_Dirt);

	Gen_CurrentState = String_FromConstant("Setting grass blocks");
	FlatgrassGen_MapSet(Gen_Height / 2 - 1, Gen_Height / 2 - 1, BlockID_Grass);
}

void FlatgrassGen_MapSet(Int32 yStart, Int32 yEnd, BlockID block) {
	yStart = max(yStart, 0); yEnd = max(yEnd, 0);
	Int32 startIndex = yStart * Gen_Length * Gen_Width;
	Int32 endIndex = (yEnd * Gen_Length + (Gen_Length - 1)) * Gen_Width + (Gen_Width - 1);
	Int32 count = (endIndex - startIndex) + 1, offset = 0;

	Gen_CurrentProgress = 0.0f;
	BlockID* ptr = Gen_Blocks;

	while (offset < count) {
		Int32 bytes = min(count - offset, Gen_Width * Gen_Length) * sizeof(BlockID);
		Platform_MemSet(ptr, block, bytes);

		ptr += bytes; offset += bytes;
		Gen_CurrentProgress = (Real32)offset / count;
	}
}