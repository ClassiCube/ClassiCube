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
#include <malloc.h>

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

static GfxResourceID white_square;
static int primitive_type;

void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 16x16 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[16 * 16];
	Mem_Set(pixels, 0xFF, sizeof(pixels));
	Bitmap_Init(bmp, 16, 16, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
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

static qword_t* SetTextureWrapping(qword_t* q, int context) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_REPEAT, WRAP_REPEAT, 0, 0, 0, 0), 
					GS_REG_CLAMP + context);
	q++;
	return q;
}

static qword_t* SetTextureSampling(qword_t* q, int context) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	// TODO: should mipmapselect (first 0 after MIN_NEAREST) be 1?
	PACK_GIFTAG(q, GS_SET_TEX1(LOD_USE_K, 0, LOD_MAG_NEAREST, LOD_MIN_NEAREST, 0, 0, 0), 
					GS_REG_TEX1 + context);
	q++;
	return q;
}

static qword_t* SetAlphaBlending(qword_t* q, int context) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	// https://psi-rockin.github.io/ps2tek/#gsalphablending
	// Output = (((A - B) * C) >> 7) + D
	//        = (((src - dst) * alpha) >> 7) + dst
	//        =  (src * alpha - dst * alpha) / 128 + dst
	//        =  (src * alpha - dst * alpha) / 128 + dst * 128 / 128
	//        = ((src * alpha + dst * (128 - alpha)) / 128
	PACK_GIFTAG(q, GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST, BLEND_ALPHA_SOURCE,
								BLEND_COLOR_DEST, 0x80), GS_REG_ALPHA + context);
	q++;
	return q;
}

static void InitDrawingEnv(void) {
	packet_t *packet = packet_init(30, PACKET_NORMAL); // TODO: is 30 too much?
	qword_t *q = packet->data;
	
	q = draw_setup_environment(q, 0, &fb_color, &fb_depth);
	// GS can render from 0 to 4096, so set primitive origin to centre of that
	q = draw_primitive_xyoffset(q, 0, 2048 - vp_hwidth, 2048 - vp_hheight);

	q = SetTextureWrapping(q, 0);
	q = SetTextureSampling(q, 0);
	q = SetAlphaBlending(q,   0); // TODO has no effect ?
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF,packet->data,q - packet->data, 0, 0);
	dma_wait_fast();

	packet_free(packet);
}

// TODO: Find a better way than just increasing this hardcoded size
static void InitDMABuffers(void) {
	packets[0] = packet_init(50000, PACKET_NORMAL);
	packets[1] = packet_init(50000, PACKET_NORMAL);
}

static void FlipContext(void) {
	context ^= 1;
	current  = packets[context];
	
	dma_tag = current->data;
	// increment past the dmatag itself
	q = dma_tag + 1;
}

static int tex_offset;
void Gfx_Create(void) {
	vp_hwidth  = DisplayInfo.Width  / 2;
	vp_hheight = DisplayInfo.Height / 2;
	primitive_type = 0; // PRIM_POINT, which isn't used here
	
	InitBuffers();
	InitDrawingEnv();
	InitDMABuffers();
	tex_offset = graph_vram_allocate(256, 256, GS_PSM_32, GRAPH_ALIGN_BLOCK);
	
	context = 1;
	FlipContext();
	
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
	Gfx.MaxTexSize   = 512 * 512;
	Gfx.Created      = true;
	
	Gfx_RestoreState();
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture_ {
	cc_uint32 width, height;
	cc_uint32 log2_width, log2_height;
	cc_uint32 pad[(64 - 4)/4];
	cc_uint32 pixels[]; // aligned to 64 bytes (only need 16?)
} CCTexture;

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)memalign(16, 64 + size);
	
	tex->width       = bmp->width;
	tex->height      = bmp->height;
	tex->log2_width  = draw_log2(bmp->width);
	tex->log2_height = draw_log2(bmp->height);
	
	Mem_Copy(tex->pixels, bmp->scan0, size);
	return tex;
}

static void UpdateTextureBuffer(int context, texbuffer_t *texture, CCTexture* tex) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_TEX0(texture->address >> 6, texture->width >> 6, GS_PSM_32,
							   tex->log2_width, tex->log2_height, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE,
							   0, 0, CLUT_STORAGE_MODE1, 0, CLUT_NO_LOAD), GS_REG_TEX0 + context);
	q++;
}

static int BINDS;
void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;
	Platform_Log2("BIND: %i x %i", &tex->width, &tex->height);
	// TODO
	if (BINDS) return;
	BINDS = 1;
	
	texbuffer_t texbuf;
	texbuf.width   = max(256, tex->width);
	texbuf.address = tex_offset;
	
	// TODO terrible perf
	DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
	dma_wait_fast();
	dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
	//
	
	packet_t *packet = packet_init(200, PACKET_NORMAL);

	qword_t *Q = packet->data;

	Q = draw_texture_transfer(Q, tex->pixels, tex->width, tex->height, GS_PSM_32, tex_offset, max(256, tex->width));
	Q = draw_texture_flush(Q);

	dma_channel_send_chain(DMA_CHANNEL_GIF,packet->data, Q - packet->data, 0,0);
	dma_wait_fast();

	//packet_free(packet);
	
	// TODO terrible perf
	q = dma_tag + 1;
	UpdateTextureBuffer(0, &texbuf, tex);
	
	Platform_LogConst("=====");
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
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
static cc_bool gfx_alphaBlend;
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
	gfx_alphaBlend = enabled;
	// TODO update primitive state
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

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
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

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar_ + zNear_) / (zFar_ - zNear_);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar_ * zNear_) / (zFar_ - zNear_);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	// TODO update cached primitive state
}

typedef struct Vector4 { float x, y, z, w; } Vector4;

static cc_bool NotClipped(Vector4 pos) {
	// The code below clips to the viewport clip planes
	//  For e.g. X this is [2048 - vp_width / 2, 2048 + vp_width / 2]
	//  However the guard band itself ranges from 0 to 4096
	// To reduce need to clip, clip against guard band on X/Y axes instead
	/*return
		xAdj  >= -pos.w && xAdj  <= pos.w &&
		yAdj  >= -pos.w && yAdj  <= pos.w &&
		pos.z >= -pos.w && pos.z <= pos.w;*/	
		
	// Rescale clip planes to guard band extent:
	//  X/W * vp_hwidth <= vp_hwidth -- clipping against viewport
	//              X/W <= 1
	//              X   <= W
	//  X/W * vp_hwidth <= 2048      -- clipping against guard band
	//              X/W <= 2048 / vp_hwidth
	//              X * vp_hwidth / 2048 <= W
	float xAdj = pos.x * (vp_hwidth/2048);
	float yAdj = pos.y * (vp_hheight/2048);
	
	// X/W * vp_hwidth <= 2048
	// 
		
	// Clip X/Y to INSIDE the guard band regions
	// NOTE: This seems to result in lockup at end of frame
	return
		xAdj > -pos.w && xAdj < pos.w &&
		yAdj > -pos.w && yAdj < pos.w &&
		pos.z >= -pos.w && pos.z <= pos.w;
}

static Vector4 TransformVertex(struct VertexTextured* pos) {
	Vector4 coord;
	coord.x = pos->x * mvp.row1.x + pos->y * mvp.row2.x + pos->z * mvp.row3.x + mvp.row4.x;
	coord.y = pos->x * mvp.row1.y + pos->y * mvp.row2.y + pos->z * mvp.row3.y + mvp.row4.y;
	coord.z = pos->x * mvp.row1.z + pos->y * mvp.row2.z + pos->z * mvp.row3.z + mvp.row4.z;
	coord.w = pos->x * mvp.row1.w + pos->y * mvp.row2.w + pos->z * mvp.row3.w + mvp.row4.w;
	return coord;
}

//#define VCopy(dst, src) dst.x = vp_hwidth  * (1 + src.x / src.w); dst.y = vp_hheight * (1 - src.y / src.w); dst.z = src.z / src.w; dst.w = src.w;
static xyz_t FinishVertex(struct Vector4 src) {
	float x = (vp_hwidth/2048)  * (src.x / src.w);
	float y = (vp_hheight/2048) * (src.y / src.w);
	float z = src.z / src.w;
	
	int originX = ftoi4(2048);
	int originY = ftoi4(2048);
	unsigned int maxZ = 1 << (32 - 1); // TODO: half this? or << 24 instead?
	
	xyz_t xyz;
	xyz.x = (short)((x + 1.0f) *  originX);
	xyz.y = (short)((y + 1.0f) * -originY);
	xyz.z = (unsigned int)((z + 1.0f) * maxZ);
	return xyz;
}

static void DrawTriangle(Vector4 v0, Vector4 v1, Vector4 v2, struct VertexTextured* V0, struct VertexTextured* V1, struct VertexTextured* V2) {
	//Platform_Log4("X: %f3, Y: %f3, Z: %f3, W: %f3", &v0.x, &v0.y, &v0.z, &v0.w);	
	u64* dw;
	
	Vector4 verts[3] = { v0, v1, v2 };
	struct VertexTextured* v[] = { V0, V1, V2 };
	
	//Platform_Log4("   X: %f3, Y: %f3, Z: %f3, W: %f3", &in_vertices[0].x, &in_vertices[0].y, &in_vertices[0].z, &in_vertices[0].w);	
	
	// 3 "primitives" follow in the GIF packet (vertices in this case)
	// 2 registers per "primitive" (colour, position)
	PACK_GIFTAG(q, GIF_SET_TAG(3,1,0,0, GIF_FLG_REGLIST, 3), DRAW_STQ_REGLIST);
	q++;

	// TODO optimise
	// Add the "primitives" to the GIF packet
	dw = (u64*)q;
	for (int i = 0; i < 3; i++)
	{
		float Q   = 1.0f / verts[i].w;
		xyz_t xyz = FinishVertex(verts[i]);
		color_t color;
		texel_t texel;
		
		// See 'Colour Functions' https://psi-rockin.github.io/ps2tek/#gstextures
		// Essentially, colour blending is calculated as
		//   finalR = (vertexR * textureR) >> 7
		// However, this behaves contrary to standard expectations
		//  and results in final vertex colour being too bright
		//
		// For instance, if vertexR was white and textureR was grey:
		//   finalR = (255 * 127) / 128 = 255
		// White would be produced as the final colour instead of expected grey
		//
		// To counteract this, just divide all vertex colours by 2 first
		color.rgbaq = (v[i]->Col & 0xFEFEFEFE) >> 1;
		color.q     = Q;
		texel.u     = v[i]->U * Q;
		texel.v     = v[i]->V * Q;
		
		*dw++ = color.rgbaq;
		*dw++ = texel.uv;
		*dw++ = xyz.xyz;
	}
	dw++; // one more to even out number of doublewords
	q = dw;
}

static void DrawTriangles(int verticesCount, int startVertex) {
	if (stateDirty) UpdateState(0);
	if (gfx_format == VERTEX_FORMAT_COLOURED) return;
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;
	
	// TODO: only when vertex format changes ?
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_GOURAUD, texturing, DRAW_DISABLE,
							  gfx_alphaBlend, DRAW_DISABLE, PRIM_MAP_ST,
							  0, PRIM_UNFIXED), GS_REG_PRIM);
	q++;
	
	struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex;
	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		Vector4 V0 = TransformVertex(v + 0);
		Vector4 V1 = TransformVertex(v + 1);
		Vector4 V2 = TransformVertex(v + 2);
		Vector4 V3 = TransformVertex(v + 3);
		
		
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[0].x, &v[0].y, &v[0].z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[1].x, &v[1].y, &v[1].z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[2].x, &v[2].y, &v[2].z);
	//Platform_Log3("X: %f3, Y: %f3, Z: %f3", &v[3].x, &v[3].y, &v[3].z);
	//Platform_LogConst(">>>>>>>>>>");
		
		if (NotClipped(V0) && NotClipped(V1) && NotClipped(V2)) {
			DrawTriangle(V0, V1, V2, v + 0, v + 1, v + 2);
		}
		
		if (NotClipped(V2) && NotClipped(V3) && NotClipped(V0)) {
			DrawTriangle(V2, V3, V0, v + 2, v + 3, v + 0);
		}
		
		//Platform_LogConst("-----");
	}
}

// TODO: Can this be used? need to understand EOP more
static void SetPrimitiveType(int type) {
	if (primitive_type == type) return;
	primitive_type = type;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRIM(type, PRIM_SHADE_GOURAUD, DRAW_DISABLE, DRAW_DISABLE,
							  DRAW_DISABLE, DRAW_DISABLE, PRIM_MAP_ST,
							  0, PRIM_UNFIXED), GS_REG_PRIM);
	q++;
}

void Gfx_DrawVb_Lines(int verticesCount) {
	//SetPrimitiveType(PRIM_LINE);
} /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	//SetPrimitiveType(PRIM_TRIANGLE);
	DrawTriangles(verticesCount, startVertex);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	//SetPrimitiveType(PRIM_TRIANGLE);
	DrawTriangles(verticesCount, 0);
	// TODO
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	//SetPrimitiveType(PRIM_TRIANGLE);
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
	Platform_LogConst("--- Frame ---");
}

void Gfx_EndFrame(void) {
	BINDS = 0;
	Platform_LogConst("--- EF1 ---");
	q = draw_finish(q);
	Platform_LogConst("--- EF2 ---");
	
	// Fill out and then send DMA chain
	DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
	dma_wait_fast();
	dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
	Platform_LogConst("--- EF3 ---");
		
	draw_wait_finish();
	Platform_LogConst("--- EF4 ---");
	
	if (gfx_vsync) graph_wait_vsync();
	if (gfx_minFrameMs) LimitFPS();
	
	FlipContext();
	Platform_LogConst("--- EF5 ---");
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS2 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
