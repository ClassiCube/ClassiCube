#include "IModel.h"

void ModelVertex_Init(ModelVertex* vertex, Real32 x, Real32 y, Real32 z, UInt16 u, UInt16 v) {
	vertex->X = x; vertex->Y = y; vertex->Z = z;
	vertex->U = u; vertex->V = v;
}