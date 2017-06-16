#include "Builder1DPart.h"
#include "Platform.h"
#include "ErrorHandler.h"
#include "GraphicsAPI.h"

void Builder1DPart_Prepare(Builder1DPart* part) {
	Int32 vCount = VCOUNT(part->iCount);
	part->sOffset = 0;
	part->sAdvance = (part->sCount / 6);

	/* ensure buffer can be accessed with 64 bytes alignment by putting 2 extra vertices at end. */
	if ((vCount + 2) > part->verticesBufferCount) {
		if (part->vertices != NULL)
			Platform_MemFree(part->vertices);

		part->vertices = Platform_MemAlloc((vCount + 2) * sizeof(VertexP3fT2fC4b));
		if (part->vertices == NULL) {
			ErrorHandler_Fail("Builder1DPart_Prepare - failed to allocate memory");
		}		
	}

	Int32 offset = VCOUNT(part->sCount), i;
	for (i = 0; i < Face_Count; i++) {
		part->fVertices[i] = &part->vertices[offset];
		offset += VCOUNT(part->fCount[i]);
	}
}

void Builder1DPart_Reset(Builder1DPart* part) {
	part->iCount = 0;
	part->sCount = 0; part->sOffset = 0; part->sAdvance = 0;

	Int32 i;
	for (i = 0; i < Face_Count; i++) {
		part->fVertices[i] = NULL;
		part->fCount[i] = 0;	
	}
}