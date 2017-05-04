#include "TextureRec.h"

TextureRec TextureRec_FromRegion(Real32 u, Real32 v, Real32 uWidth, Real32 vHeight) {
	TextureRec rec;
	rec.U1 = u; rec.V1 = v;
	rec.U2 = u + uWidth; rec.V2 = v + vHeight;
	return rec;
}

TextureRec TextureRec_FromPoints(Real32 u1, Real32 v1, Real32 u2, Real32 v2) {
	TextureRec rec;
	rec.U1 = u1; rec.V1 = v1;
	rec.U2 = u2; rec.V2 = v2;
	return rec;
}

void TextureRec_SwapU(TextureRec* rec) {
	Real32 u2 = rec->U2; rec->U2 = rec->U1; rec->U1 = u2;
}