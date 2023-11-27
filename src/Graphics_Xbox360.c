#include "Core.h"
#ifdef CC_BUILD_XBOX360
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <xenos/xe.h>

static struct XenosDevice device;
static struct XenosDevice* xe;

void Gfx_Create(void) {
	xe = &device;
	Xe_Init(xe);
	
	struct XenosSurface *fb = Xe_GetFramebufferSurface(xe);
	Xe_SetRenderTarget(xe, fb);

	Gfx.Created      = true;
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
}

static void Gfx_FreeState(void) { 
	FreeDefaultResources();
}

static void Gfx_RestoreState(void) {
	Gfx_SetFaceCulling(false);
	InitDefaultResources();
	gfx_format = -1;

	Xe_SetAlphaFunc(xe, XE_CMP_GREATER);
	Xe_SetAlphaRef(xe,  0.5f);
	Xe_SetDestBlend(xe, XE_BLEND_SRCALPHA);
	Xe_SetSrcBlend(xe,  XE_BLEND_INVSRCALPHA);
	Xe_SetZFunc(xe,     XE_CMP_LESSEQUAL);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void SetTextureData(struct XenosSurface* xtex, struct Bitmap* bmp) {
	void* dst = Xe_Surface_LockRect(xe, xtex, 0, 0, bmp->width, bmp->height, XE_LOCK_WRITE);
	cc_uint32 size = Bitmap_DataSize(bmp->width, bmp->height);
	
	Mem_Copy(dst, bmp->scan0, size);
	
	Xe_Surface_Unlock(xe, xtex);
}

static void SetTexturePartData(struct XenosSurface* xtex, int x, int y, const struct Bitmap* bmp, int rowWidth, int lvl) {
	void* dst = Xe_Surface_LockRect(xe, xtex, x, y, bmp->width, bmp->height, XE_LOCK_WRITE);

	CopyTextureData(dst, bmp->width * 4, bmp, rowWidth << 2);
	
	Xe_Surface_Unlock(xe, xtex);
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	struct XenosSurface* xtex = Xe_CreateTexture(xe, bmp->width, bmp->height, 1, XE_FMT_8888, 0);
	SetTextureData(xtex, bmp);
	return xtex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	struct XenosSurface* xtex = (struct XenosSurface*)texId;
	SetTexturePartData(xtex, x, y, part, rowWidth, 0);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_BindTexture(GfxResourceID texId) {
	struct XenosSurface* xtex = (struct XenosSurface*)texId;
	Xe_SetTexture(xe, 0, xtex);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	struct XenosSurface* xtex = (struct XenosSurface*)(*texId);
	if (xtex) Xe_DestroyTexture(xe, xtex);
	*texId = NULL;
}

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool gfx_alphaTesting, gfx_alphaBlending;

void Gfx_SetFaceCulling(cc_bool enabled) {
	Xe_SetCullMode(xe, enabled ? XE_CULL_CW : XE_CULL_NONE);
}

void Gfx_SetFog(cc_bool enabled) {
	// TODO
}

void Gfx_SetFogCol(PackedCol color) {
	// TODO
}

void Gfx_SetFogDensity(float value) {
	// TODO
}

void Gfx_SetFogEnd(float value) {
	// TODO
}

void Gfx_SetFogMode(FogFunc func) {
	// TODO
}

void Gfx_SetAlphaTest(cc_bool enabled) {
	if (gfx_alphaTesting == enabled) return;
	gfx_alphaTesting = enabled;
	Xe_SetAlphaTestEnable(xe, enabled);
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	if (gfx_alphaBlending == enabled) return;
	gfx_alphaBlending = enabled;

	if (Gfx.LostContext) return;
	//IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, enabled);
	// TODO
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
	// TODO
}

void Gfx_ClearCol(PackedCol color) { 
	Xe_SetClearColor(xe, color);
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

void Gfx_SetDepthTest(cc_bool enabled) {
	Xe_SetZEnable(xe, enabled);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	Xe_SetZWrite(xe, enabled);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// TODO
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	int size = count * 2;
	struct XenosIndexBuffer* xib = Xe_CreateIndexBuffer(xe, size, XE_FMT_INDEX16);
	
	void* dst = Xe_IB_Lock(xe, xib, 0, size, XE_LOCK_WRITE);
	fillFunc((cc_uint16*)dst, count, obj);
	Xe_IB_Unlock(xe, xib);
}

void Gfx_BindIb(GfxResourceID ib) {
	struct XenosIndexBuffer* xib = (struct XenosIndexBuffer*)ib;
	Xe_SetIndices(xe, xib);
}

void Gfx_DeleteIb(GfxResourceID* ib) { 
	struct XenosIndexBuffer* xib = (struct XenosIndexBuffer*)(*ib);
	if (xib) Xe_DestroyIndexBuffer(xe, xib);
	*ib = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	int size = count * strideSizes[fmt];
	return Xe_CreateVertexBuffer(xe, size);
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)(*vb);
	if (xvb) Xe_DestroyVertexBuffer(xe, xvb);
	*vb = NULL;
}

void Gfx_BindVb(GfxResourceID vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	Xe_SetStreamSource(xe, 0, xvb, 0, gfx_stride);
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	int size = count * strideSizes[fmt];
	return Xe_VB_Lock(xe, xvb, 0, size, XE_LOCK_WRITE);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	struct XenosVertexBuffer* xvb = (struct XenosVertexBuffer*)vb;
	Xe_VB_Unlock(xe, xvb);
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Gfx_AllocStaticVb(fmt, maxVertices);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }

void Gfx_BindDynamicVb(GfxResourceID vb)    { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return Gfx_LockVb(vb, fmt, count);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb)  { Gfx_UnlockVb(vb); }


/*########################################################################################################################*
*-----------------------------------------------------Vertex rendering----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;

	if (fmt == VERTEX_FORMAT_COLOURED) {
		/* it's necessary to unbind the texture, otherwise the alpha from the last bound texture */
		/*  gets used - because D3DTSS_ALPHAOP texture stage state is still set to D3DTOP_SELECTARG1 */
		Xe_SetTexture(xe, 0, NULL);
		/*  IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, fmt == VERTEX_FORMAT_COLOURED ? D3DTOP_DISABLE : D3DTOP_MODULATE); */
		/*  IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, fmt == VERTEX_FORMAT_COLOURED ? D3DTOP_DISABLE : D3DTOP_SELECTARG1); */
		/* SetTexture(NULL) seems to be enough, not really required to call SetTextureStageState */
	}

	// TODO
}

void Gfx_DrawVb_Lines(int verticesCount) {
	/* NOTE: Skip checking return result for Gfx_DrawXYZ for performance */
	Xe_DrawPrimitive(xe, XE_PRIMTYPE_LINELIST, 0, verticesCount >> 1);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	Xe_DrawIndexedPrimitive(xe, XE_PRIMTYPE_TRIANGLELIST,
		0, 0, verticesCount, 0, verticesCount >> 1);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	Xe_DrawIndexedPrimitive(xe, XE_PRIMTYPE_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	Xe_DrawIndexedPrimitive(xe, XE_PRIMTYPE_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	// TODO
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	// TODO
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) {
	// TODO
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// TODO verify this
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z =  1.0f / (zNear - zFar);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = zNear / (zNear - zFar);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// TODO verify this
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = zFar / (zNear - zFar);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = (zNear * zFar) / (zNear - zFar);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_BeginFrame(void) { 
}

void Gfx_Clear(void) {
	// Xe_Clear is just a Resolve anyways
}

void Gfx_EndFrame(void) {
	Xe_Resolve(xe);
	Xe_Sync(xe);
	
	if (gfx_minFrameMs) LimitFPS();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using XBox 360 --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(void) {

}
#endif
