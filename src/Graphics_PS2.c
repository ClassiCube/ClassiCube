#include "Core.h"
#if defined CC_BUILD_PS2
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <packet.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_privileged.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>
#include <draw3d.h>

static void* gfx_vertices;
static framebuffer_t fb_color;
static zbuffer_t     fb_depth;
static float vp_hwidth, vp_hheight;

// double buffering
static packet_t* packets[2];
static packet_t* current;
static int context;
static qword_t* dma_tag;
static qword_t* q;

void Gfx_RestoreState(void) {
	InitDefaultResources();
}

void Gfx_FreeState(void) {
	FreeDefaultResources();
}

// TODO: Maybe move to Window backend and just initialise once ??
static void InitBuffers(void) {
	fb_color.width   = DisplayInfo.Width;
	fb_color.height  = DisplayInfo.Height;
	fb_color.mask    = 0;
	fb_color.psm     = GS_PSM_32;
	fb_color.address = graph_vram_allocate(fb_color.width, fb_color.height, fb_color.psm, GRAPH_ALIGN_PAGE);

	fb_depth.enable  = DRAW_ENABLE;
	fb_depth.mask    = 0;
	fb_depth.method  = ZTEST_METHOD_GREATER_EQUAL;
	fb_depth.zsm     = GS_ZBUF_32;
	fb_depth.address = graph_vram_allocate(fb_color.width, fb_color.height, fb_depth.zsm, GRAPH_ALIGN_PAGE);

	graph_initialize(fb_color.address, fb_color.width, fb_color.height, fb_color.psm, 0, 0);
}

static void InitDrawingEnv(void) {
	packet_t *packet = packet_init(20, PACKET_NORMAL);
	qword_t *q = packet->data;
	
	q = draw_setup_environment(q, 0, &fb_color, &fb_depth);
	// GS can render from 0 to 4096, so set primitive origin to centre of that
	q = draw_primitive_xyoffset(q, 0, 2048 - vp_hwidth, 2048 - vp_hheight);

	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

static void InitDMABuffers(void) {
	packets[0] = packet_init(10000, PACKET_NORMAL);
	packets[1] = packet_init(10000, PACKET_NORMAL);
}

static void FlipContext(void) {
	context ^= 1;
	current  = packets[context];
	
	dma_tag = current->data;
	// increment past the dmatag itself
	q = dma_tag + 1;
}

void Gfx_Create(void) {
	vp_hwidth  = DisplayInfo.Width  / 2;
	vp_hheight = DisplayInfo.Height / 2;
	
	InitBuffers();
	InitDrawingEnv();
	InitDMABuffers();
	
	context = 1;
	FlipContext();
	
	Gfx.MaxTexWidth  = 512;
	Gfx.MaxTexHeight = 512;
	Gfx.Created      = true;
	
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	// TODO
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	// TODO
}
		
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	// TODO
	return (void*)1;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	// TODO
}

void Gfx_SetTexturing(cc_bool enabled) { }
void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
static int clearR, clearG, clearB;
static cc_bool gfx_alphaTest;
static cc_bool gfx_depthTest;
static cc_bool stateDirty;

void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

// 
static void UpdateState(int context) {
	// TODO: toggle Enable instead of method ?
	int aMethod = gfx_alphaTest ? ATEST_METHOD_GREATER_EQUAL : ATEST_METHOD_ALLPASS;
	int dMethod = gfx_depthTest ? ZTEST_METHOD_GREATER_EQUAL : ZTEST_METHOD_ALLPASS;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE,  aMethod, 0x80, ATEST_KEEP_FRAMEBUFFER,
							   DRAW_DISABLE, DRAW_DISABLE,
							   DRAW_ENABLE,  dMethod), GS_REG_TEST + context);
	q++;
	
	stateDirty = false;
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	// TODO
}

void Gfx_SetAlphaTest(cc_bool enabled) {
	gfx_alphaTest = enabled;
	stateDirty    = true;
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	// TODO
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_Clear(void) {
	q = draw_disable_tests(q, 0, &fb_depth);
	q = draw_clear(q, 0, 2048.0f - fb_color.width / 2.0f, 2048.0f - fb_color.height / 2.0f,
					fb_color.width, fb_color.height, clearR, clearG, clearB);
	UpdateState(0);
}

void Gfx_ClearCol(PackedCol color) {
	clearR = PackedCol_R(color);
	clearG = PackedCol_G(color);
	clearB = PackedCol_B(color);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	gfx_depthTest = enabled;
	stateDirty    = true;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	// TODO
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) { }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	// TODO
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return Mem_TryAlloc(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb; 
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
	// TODO
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
	// TODO
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) {
	// TODO
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z = -2.0f / (zFar - zNear);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = -(zFar + zNear) / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear_ = zFar;
	float zFar_  = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For pos FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;
	// TODO: Check is Frustum culling needs changing for this

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = -(zFar_ + zNear_) / (zFar_ - zNear_);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = -(2.0f * zFar_ * zNear_) / (zFar_ - zNear_);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

typedef struct Vector4 { float X, Y, Z, W; } Vector4;

static cc_bool NotClipped(Vector4 pos) {
	return
		pos.X >= -pos.W && pos.X <= pos.W &&
		pos.Y >= -pos.W && pos.Y <= pos.W &&
		pos.Z >= -pos.W && pos.Z <= pos.W;
}

static Vector4 TransformVertex(struct VertexTextured* pos) {
	Vector4 coord;
	coord.X = pos->X * mvp.row1.X + pos->Y * mvp.row2.X + pos->Z * mvp.row3.X + mvp.row4.X;
	coord.Y = pos->X * mvp.row1.Y + pos->Y * mvp.row2.Y + pos->Z * mvp.row3.Y + mvp.row4.Y;
	coord.Z = pos->X * mvp.row1.Z + pos->Y * mvp.row2.Z + pos->Z * mvp.row3.Z + mvp.row4.Z;
	coord.W = pos->X * mvp.row1.W + pos->Y * mvp.row2.W + pos->Z * mvp.row3.W + mvp.row4.W;
	return coord;
}

#define VCopy(dst, src) dst.x = (vp_hwidth/2048) * (src.X / src.W); dst.y = (vp_hheight/2048) * (src.Y / src.W); dst.z = src.Z / src.W; dst.w = src.W;
//#define VCopy(dst, src) dst.x = vp_hwidth  * (1 + src.X / src.W); dst.y = vp_hheight * (1 - src.Y / src.W); dst.z = src.Z / src.W; dst.w = src.W;
#define CCopy(dst) dst.r = PackedCol_R(v->Col); dst.g = PackedCol_G(v->Col); dst.b = PackedCol_B(v->Col); dst.a = PackedCol_A(v->Col); dst.q = 1.0f;

static void DrawTriangle(Vector4 v0, Vector4 v1, Vector4 v2, struct VertexTextured* v) {
	vertex_f_t in_vertices[3];
	//Platform_Log4("X: %f3, Y: %f3, Z: %f3, W: %f3", &v0.X, &v0.Y, &v0.Z, &v0.W);	
	xyz_t out_vertices[3];
	color_t out_color[3];
	
	VCopy(in_vertices[0], v0);
	VCopy(in_vertices[1], v1);
	VCopy(in_vertices[2], v2);
	
	//Platform_Log4("   X: %f3, Y: %f3, Z: %f3, W: %f3", &in_vertices[0].x, &in_vertices[0].y, &in_vertices[0].z, &in_vertices[0].w);	
	CCopy(out_color[0]);
	CCopy(out_color[1]);
	CCopy(out_color[2]);
	
	prim_t prim;
	color_t color;

	// Define the triangle primitive we want to use.
	prim.type = PRIM_TRIANGLE;
	prim.shading = PRIM_SHADE_GOURAUD;
	prim.mapping = DRAW_DISABLE;
	prim.fogging = DRAW_DISABLE;
	prim.blending = DRAW_DISABLE;
	prim.antialiasing = DRAW_DISABLE;
	prim.mapping_type = PRIM_MAP_ST;
	prim.colorfix = PRIM_UNFIXED;

	// NOTE: not actually used
	color.r = 0x10;
	color.g = 0x10;
	color.b = 0x10;
	color.a = 0x10;
	color.q = 1.0f;
	
	draw_convert_xyz(out_vertices, 2048, 2048, 32, 3, in_vertices);
	
	// Draw the triangles using triangle primitive type.
	q = draw_prim_start(q, 0, &prim, &color);

	for(int i = 0; i < 3; i++)
	{
		q->dw[0] = out_color[i].rgbaq;
		q->dw[1] = out_vertices[i].xyz;
		q++;
	}

	q = draw_prim_end(q,2,DRAW_RGBAQ_REGLIST);
}

static void DrawTriangles(int verticesCount, int startVertex) {
	if (stateDirty) UpdateState(0);
	if (gfx_format == VERTEX_FORMAT_COLOURED) return;
	
	struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex;
	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		Vector4 V0 = TransformVertex(v + 0);
		Vector4 V1 = TransformVertex(v + 1);
		Vector4 V2 = TransformVertex(v + 2);
		Vector4 V3 = TransformVertex(v + 3);
		
		
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[0].X, &v[0].Y, &v[0].Z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[1].X, &v[1].Y, &v[1].Z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[2].X, &v[2].Y, &v[2].Z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[3].X, &v[3].Y, &v[3].Z);
	//Platform_LogConst(">>>>>>>>>>");
		
		if (NotClipped(V0) && NotClipped(V1) && NotClipped(V2)) {
			DrawTriangle(V0, V1, V2, v);
		}
		
		if (NotClipped(V2) && NotClipped(V3) && NotClipped(V0)) {
			DrawTriangle(V2, V3, V0, v);
		}
		
		//Platform_LogConst("-----");
	}
}

void Gfx_DrawVb_Lines(int verticesCount) { } /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	// TODO
	DrawTriangles(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawTriangles(verticesCount, 0);
	// TODO
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawTriangles(verticesCount, startVertex);
	// TODO
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_BeginFrame(void) { 
	//Platform_LogConst("--- Frame ---");
}

void Gfx_EndFrame(void) {
	q = draw_finish(q);
	
	// Fill out and then send DMA chain
	DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
	dma_wait_fast();
	dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
		
	draw_wait_finish();
	
	if (gfx_vsync) graph_wait_vsync();
	if (gfx_minFrameMs) LimitFPS();
	
	FlipContext();
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	String_Format1(info, "-- Using PS2 (%i bit) --\n", &pointerSize);
	String_Format2(info, "Max texture size: (%i x %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
