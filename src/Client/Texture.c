#include "Texture.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"

void Texture_FromOrigin(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u2, Real32 v2) {
	Texture_From(tex, id, x, y, width, height, 0, u2, 0, v2);
}

void Texture_FromRec(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height, struct TextureRec rec) {
	Texture_From(tex, id, x, y, width, height, rec.U1, rec.U2, rec.V1, rec.V2);
}

void Texture_From(struct Texture* tex, GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u1, Real32 u2, Real32 v1, Real32 v2) {
	tex->ID = id;
	tex->X = x; tex->Y = y; 
	tex->Width = width; tex->Height = height;

	tex->U1 = u1; tex->V1 = v1;
	tex->U2 = u2; tex->V2 = v2;
}

void Texture_MakeInvalid(struct Texture* tex) {
	struct Texture empty = { 0 }; *tex = empty;
}

void Texture_Render(struct Texture* tex) {
	Gfx_BindTexture(tex->ID);
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Draw2DTexture(tex, white);
}

void Texture_RenderShaded(struct Texture* tex, PackedCol shadeCol) {
	Gfx_BindTexture(tex->ID);
	GfxCommon_Draw2DTexture(tex, shadeCol);
}
