#include "VertexStructs.h"

void VertexP3fC4b_Set(VertexP3fC4b* target, Real32 x, Real32 y, Real32 z, PackedCol col) {
	target->X = x; target->Y = y; target->Z = z; 
	target->Col = col;
}

void VertexP3fT2fC4b_Set(VertexP3fT2fC4b* target, Real32 x, Real32 y, Real32 z,
	Real32 u, Real32 v, PackedCol col) {
	target->X = x; target->Y = y; target->Z = z;
	target->Col = col; target->U = u; target->V = v;
}