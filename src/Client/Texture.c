#include "Texture.h"
#include "GraphicsCommon.h"
#include "GraphicsAPI.h"

Texture Texture_FromOrigin(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u2, Real32 v2) {
	return Texture_From(id, x, y, width, height, 0, u2, 0, v2);
}

Texture Texture_FromRec(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	TextureRec rec) {
	return Texture_From(id, x, y, width, height, rec.U1, rec.U2, rec.V1, rec.V2);
}

Texture Texture_From(GfxResourceID id, Int32 x, Int32 y, Int32 width, Int32 height,
	Real32 u1, Real32 u2, Real32 v1, Real32 v2) {
	Texture tex;
	tex.ID = id;
	tex.X = (Int16)x; tex.Y = (Int16)y;
	tex.Width = (Int16)width; tex.Height = (Int16)height;

	tex.U1 = u1; tex.V1 = v1;
	tex.U2 = u2; tex.V2 = v2;
	return tex;
}

Texture Texture_MakeInvalid(void) {
	Texture tex;
	tex.ID = 0;
	tex.X = 0; tex.Y = 0; tex.Width = 0; tex.Height = 0;
	tex.U1 = 0.0f; tex.U2 = 0.0f; tex.V1 = 0.0f; tex.V2 = 0.0f;
	return tex;
}

void Texture_Render(Texture* tex) {
	Gfx_BindTexture(tex->ID);
	PackedCol white = PACKEDCOL_WHITE;
	GfxCommon_Draw2DTexture(tex, white);
}

void Texture_RenderShaded(Texture* tex, PackedCol shadeCol) {
	Gfx_BindTexture(tex->ID);
	GfxCommon_Draw2DTexture(tex, shadeCol);
}