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

typedef struct Matrix VU0_MATRIX __attribute__((aligned(16)));
typedef struct Vec4   VU0_VECTOR __attribute__((aligned(16)));
typedef struct { int x, y, z, w; } VU0_IVECTOR __attribute__((aligned(16)));

static void* gfx_vertices;
extern framebuffer_t fb_colors[2];
extern zbuffer_t     fb_depth;
static VU0_VECTOR vp_origin, vp_scale;
static cc_bool stateDirty, formatDirty;
static framebuffer_t* fb_draw;
static framebuffer_t* fb_display;

static VU0_MATRIX mvp;
extern void LoadMvpMatrix(VU0_MATRIX* matrix);
extern void LoadClipScaleFactors(VU0_VECTOR* scale);
extern void LoadViewportOrigin(VU0_VECTOR* origin);
extern void LoadViewportScale(VU0_VECTOR* scale);

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

// TODO: Find a better way than just increasing this hardcoded size
static void InitDMABuffers(void) {
	packets[0] = packet_init(50000, PACKET_NORMAL);
	packets[1] = packet_init(50000, PACKET_NORMAL);
}

static void UpdateContext(void) {
	fb_display = &fb_colors[context];
	fb_draw    = &fb_colors[context ^ 1];

	current = packets[context];
	
	dma_tag = current->data;
	// increment past the dmatag itself
	q = dma_tag + 1;
}


static void SetTextureWrapping(int context) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_REPEAT, WRAP_REPEAT, 0, 0, 0, 0), 
					GS_REG_CLAMP + context);
	q++;
}

static void SetTextureSampling(int context) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	// TODO: should mipmapselect (first 0 after MIN_NEAREST) be 1?
	PACK_GIFTAG(q, GS_SET_TEX1(LOD_USE_K, 0, LOD_MAG_NEAREST, LOD_MIN_NEAREST, 0, 0, 0), 
					GS_REG_TEX1 + context);
	q++;
}

static void SetAlphaBlending(int context) {
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
}

static void InitDrawingEnv(void) {
	qword_t* beg = q;
	q = draw_setup_environment(q, 0, fb_draw, &fb_depth);
	// GS can render from 0 to 4096, so set primitive origin to centre of that
	q = draw_primitive_xyoffset(q, 0, 2048 - Game.Width / 2, 2048 - Game.Height / 2);

	SetTextureWrapping(0);
	SetTextureSampling(0);
	SetAlphaBlending(0); // TODO has no effect ?
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, beg, q - beg, 0, 0);
	dma_wait_fast();
	q = dma_tag + 1;
}

static int tex_offset;
void Gfx_Create(void) {
	primitive_type = 0; // PRIM_POINT, which isn't used here

	InitDMABuffers();
	context = 0;
	UpdateContext();
	
	stateDirty  = true;
	formatDirty = true;
	InitDrawingEnv();
	tex_offset = graph_vram_allocate(256, 256, GS_PSM_32, GRAPH_ALIGN_BLOCK);
	
// TODO maybe Min not actually needed?
	Gfx.MinTexWidth  = 4;
	Gfx.MinTexHeight = 4;	
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
	cc_uint32 pad[(64 - 16)/4];
	BitmapCol pixels[]; // aligned to 64 bytes (only need 16?)
} CCTexture;

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)memalign(16, 64 + size);
	
	tex->width       = bmp->width;
	tex->height      = bmp->height;
	tex->log2_width  = draw_log2(bmp->width);
	tex->log2_height = draw_log2(bmp->height);
	
	CopyTextureData(tex->pixels, bmp->width * BITMAPCOLOR_SIZE, 
					bmp, rowWidth * BITMAPCOLOR_SIZE);
	return tex;
}

static void UpdateTextureBuffer(int context, CCTexture* tex, unsigned buf_addr, unsigned buf_stride) {
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;

	PACK_GIFTAG(q, GS_SET_TEX0(buf_addr >> 6, buf_stride >> 6, GS_PSM_32,
							   tex->log2_width, tex->log2_height, TEXTURE_COMPONENTS_RGBA, TEXTURE_FUNCTION_MODULATE,
							   0, 0, CLUT_STORAGE_MODE1, 0, CLUT_NO_LOAD), GS_REG_TEX0 + context);
	q++;
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = (CCTexture*)texId;
	
	unsigned dst_addr   = tex_offset;
	// GS stores stride in 64 block units
	// (i.e. gs_stride = real_stride >> 6, so min stride is 64)
	unsigned dst_stride = max(64, tex->width);
	
	// TODO terrible perf
	DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
	dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
	dma_wait_fast();
	
	packet_t *packet = packet_init(200, PACKET_NORMAL);

	qword_t *Q = packet->data;

	Q = draw_texture_transfer(Q, tex->pixels, tex->width, tex->height, GS_PSM_32, dst_addr, dst_stride);
	Q = draw_texture_flush(Q);

	dma_channel_send_chain(DMA_CHANNEL_GIF,packet->data, Q - packet->data, 0,0);
	dma_wait_fast();

	packet_free(packet);
	
	// TODO terrible perf
	q = dma_tag + 1;
	UpdateTextureBuffer(0, tex, dst_addr, dst_stride);
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	BitmapCol* dst = (tex->pixels + x) + y * tex->width;

	CopyTextureData(dst, tex->width * BITMAPCOLOR_SIZE, 
					part, rowWidth  * BITMAPCOLOR_SIZE);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
static int clearR, clearG, clearB;
static cc_bool gfx_depthTest;

void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

static void UpdateState(int context) {
	// TODO: toggle Enable instead of method ?
	int aMethod = gfx_alphaTest ? ATEST_METHOD_GREATER_EQUAL : ATEST_METHOD_ALLPASS;
	int zMethod = gfx_depthTest ? ZTEST_METHOD_GREATER_EQUAL : ZTEST_METHOD_ALLPASS;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	// NOTE: Reference value is 0x40 instead of 0x80, since alpha values are halved compared to normal
	PACK_GIFTAG(q, GS_SET_TEST(DRAW_ENABLE,  aMethod, 0x40, ATEST_KEEP_ALL,
							   DRAW_DISABLE, DRAW_DISABLE,
							   DRAW_ENABLE,  zMethod), GS_REG_TEST + context);
	q++;
	
	stateDirty = false;
}

static void UpdateFormat(int context) {
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_PRIM(PRIM_TRIANGLE, PRIM_SHADE_GOURAUD, texturing, DRAW_DISABLE,
							  gfx_alphaBlend, DRAW_DISABLE, PRIM_MAP_ST,
							  context, PRIM_UNFIXED), GS_REG_PRIM);
	q++;
	
	formatDirty = false;
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	// TODO
}

static void SetAlphaTest(cc_bool enabled) {
	stateDirty = true;
}

static void SetAlphaBlend(cc_bool enabled) {
	formatDirty = true;
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO clear only some buffers
	q = draw_disable_tests(q, 0, &fb_depth);
	q = draw_clear(q, 0, 2048.0f - fb_colors[0].width / 2.0f, 2048.0f - fb_colors[0].height / 2.0f,
					fb_colors[0].width, fb_colors[0].height, clearR, clearG, clearB);

	UpdateState(0);
	UpdateFormat(0);
}

void Gfx_ClearColor(PackedCol color) {
	clearR = PackedCol_R(color);
	clearG = PackedCol_G(color);
	clearB = PackedCol_B(color);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	gfx_depthTest = enabled;
	stateDirty    = true;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	fb_depth.mask = !enabled;
	q = draw_zbuffer(q, 0, &fb_depth);
	fb_depth.mask = 0;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	unsigned mask = 0;
	if (!r) mask |= 0x000000FF;
	if (!g) mask |= 0x0000FF00;
	if (!b) mask |= 0x00FF0000;
	if (!a) mask |= 0xFF000000;

	fb_draw->mask = mask;
	q = draw_framebuffer(q, 0, fb_draw);
	fb_draw->mask = 0;
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
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
// Preprocess vertex buffers into optimised layout for PS2
static VertexFormat buf_fmt;
static int buf_count;

// Precalculate all the vertex data adjustment
static void PreprocessTexturedVertices(void) {
    struct VertexTextured* v = gfx_vertices;

    for (int i = 0; i < buf_count; i++, v++)
    {
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
		v->Col = (v->Col & 0xFEFEFEFE) >> 1;

		// Alpha blending divides by 128 instead of 256, so need to half alpha here
		//  so that alpha blending produces the expected results like normal GPUs
		int A = PackedCol_A(v->Col) >> 1;
		v->Col = (v->Col & ~PACKEDCOL_A_MASK) | (A << PACKEDCOL_A_SHIFT);
    }
}

static void PreprocessColouredVertices(void) {
    struct VertexColoured* v = gfx_vertices;

    for (int i = 0; i < buf_count; i++, v++)
    {
		// Alpha blending divides by 128 instead of 256, so need to half alpha here
		//  so that alpha blending produces the expected results like normal GPUs
		int A = PackedCol_A(v->Col) >> 1;
		v->Col = (v->Col & ~PACKEDCOL_A_MASK) | (A << PACKEDCOL_A_SHIFT);
    }
}

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
    buf_fmt   = fmt;
    buf_count = count;
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb;

    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices();
    } else {
        PreprocessColouredVertices();
    }
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return Gfx_LockVb(vb, fmt, count);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { Gfx_UnlockVb(vb); }

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
	// TODO	
	LoadMvpMatrix(&mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
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

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// Swap zNear/zFar since PS2 only supports GREATER or GEQUAL depth comparison modes
	float zNear_ = zFar;
	float zFar_  = 0.1f;
	float c = Cotangent(0.5f * fov);

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
typedef struct {
	u32 rgba;
	float q;
	float u;
	float v;
	xyz_t xyz;
} __attribute__((packed,aligned(8))) TexturedVertex;

typedef struct {
	u32 rgba;
	float q;
	xyz_t xyz;
} __attribute__((packed,aligned(8))) ColouredVertex;

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format  = fmt;
	gfx_stride  = strideSizes[fmt];
	formatDirty = true;
}

extern void ViewportTransform(VU0_VECTOR* src, VU0_IVECTOR* dst);
static xyz_t FinishVertex(VU0_VECTOR* src, float invW) {
	src->w = invW;
	VU0_IVECTOR tmp;
	ViewportTransform(src, &tmp);

	xyz_t xyz;
	xyz.x = (short)tmp.x;
	xyz.y = (short)tmp.y;
	xyz.z = tmp.z;
	return xyz;
}

static u64* DrawColouredTriangle(u64* dw, VU0_VECTOR* coords, 
								struct VertexColoured* V0, struct VertexColoured* V1, struct VertexColoured* V2) {
	ColouredVertex* dst = (ColouredVertex*)dw;
	float Q;

	// TODO optimise
	// Add the "primitives" to the GIF packet
	Q   = 1.0f / coords[0].w;
	dst[0].rgba  = V0->Col;
	dst[0].q     = Q;
	dst[0].xyz   = FinishVertex(&coords[0], Q);

	Q   = 1.0f / coords[1].w;
	dst[1].rgba  = V1->Col;
	dst[1].q     = Q;
	dst[1].xyz   = FinishVertex(&coords[1], Q);

	Q   = 1.0f / coords[2].w;
	dst[2].rgba  = V2->Col;
	dst[2].q     = Q;
	dst[2].xyz   = FinishVertex(&coords[2], Q);

	return dw + 6;
}

static u64* DrawTexturedTriangle(u64* dw, VU0_VECTOR* coords, 
								struct VertexTextured* V0, struct VertexTextured* V1, struct VertexTextured* V2) {
	TexturedVertex* dst = (TexturedVertex*)dw;
	float Q;

	// TODO optimise
	// Add the "primitives" to the GIF packet
	Q   = 1.0f / coords[0].w;
	dst[0].rgba  = V0->Col;
	dst[0].q     = Q;
	dst[0].u     = V0->U * Q;
	dst[0].v     = V0->V * Q;
	dst[0].xyz   = FinishVertex(&coords[0], Q);

	Q   = 1.0f / coords[1].w;
	dst[1].rgba  = V1->Col;
	dst[1].q     = Q;
	dst[1].u     = V1->U * Q;
	dst[1].v     = V1->V * Q;
	dst[1].xyz   = FinishVertex(&coords[1], Q);

	Q   = 1.0f / coords[2].w;
	dst[2].rgba  = V2->Col;
	dst[2].q     = Q;
	dst[2].u     = V2->U * Q;
	dst[2].v     = V2->V * Q;
	dst[2].xyz   = FinishVertex(&coords[2], Q);

	return dw + 9;
}

extern void TransformTexturedQuad(void* src, VU0_VECTOR* dst, VU0_VECTOR* tmp, int* clip_flags);
extern void TransformColouredQuad(void* src, VU0_VECTOR* dst, VU0_VECTOR* tmp, int* clip_flags);

static void DrawTexturedTriangles(int verticesCount, int startVertex) {
	struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex;
	qword_t* base = q;
	q++; // skip over GIF tag (will be filled in later)
	u64* dw = (u64*)q;

	unsigned numVerts = 0;
	VU0_VECTOR V[6], tmp;
	int clip[4];

	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		TransformTexturedQuad(v, V, &tmp, clip);
		
		if (((clip[0] | clip[1] | clip[2]) & 0x3F) == 0) {
			dw = DrawTexturedTriangle(dw, V, v + 0, v + 1, v + 2);
			numVerts += 3;
		}
		
		if (((clip[2] | clip[3] | clip[0]) & 0x3F) == 0) {
			dw = DrawTexturedTriangle(dw, V + 3, v + 2, v + 3, v + 0);
			numVerts += 3;
		}
	}

	if (numVerts == 0) {
		q--; // No GIF tag was added
	} else {
		if (numVerts & 1) dw++; // one more to even out number of doublewords
		q = (qword_t*)dw;

		// Fill GIF tag in now that know number of GIF "primitives" (aka vertices)
		// 2 registers per GIF "primitive" (colour, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1,0,0, GIF_FLG_REGLIST, 3), DRAW_STQ_REGLIST);
	}
}

static void DrawColouredTriangles(int verticesCount, int startVertex) {
	struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex;
	qword_t* base = q;
	q++; // skip over GIF tag (will be filled in later)
	u64* dw = (u64*)q;

	unsigned numVerts = 0;
	VU0_VECTOR V[6], tmp;
	int clip[4];

	for (int i = 0; i < verticesCount / 4; i++, v += 4)
	{
		TransformColouredQuad(v, V, &tmp, clip);
		
		if (((clip[0] | clip[1] | clip[2]) & 0x3F) == 0) {
			dw = DrawColouredTriangle(dw, V, v + 0, v + 1, v + 2);
			numVerts += 3;
		}
		
		if (((clip[2] | clip[3] | clip[0]) & 0x3F) == 0) {
			dw = DrawColouredTriangle(dw, V + 3, v + 2, v + 3, v + 0);
			numVerts += 3;
		}
	}

	if (numVerts == 0) {
		q--; // No GIF tag was added
	} else {
		q = (qword_t*)dw;

		// Fill GIF tag in now that know number of GIF "primitives" (aka vertices)
		// 2 registers per GIF "primitive" (colour, texture, position)
		PACK_GIFTAG(base, GIF_SET_TAG(numVerts, 1,0,0, GIF_FLG_REGLIST, 2), DRAW_RGBAQ_REGLIST);
	}
}

static void DrawTriangles(int verticesCount, int startVertex) {
	if (stateDirty)  UpdateState(0);
	if (formatDirty) UpdateFormat(0);

	if ((q - current->data) > 45000) {
		DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
		dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
		dma_wait_fast();
		q = dma_tag + 1;
		Platform_LogConst("Too much geometry!!!");
	}

	while (verticesCount)
	{
		int count = min(32000, verticesCount);

		if (gfx_format == VERTEX_FORMAT_COLOURED) {
			DrawColouredTriangles(count, startVertex);
		} else {
			DrawTexturedTriangles(count, startVertex);
		}
		verticesCount -= count; startVertex += count;
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

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
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

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_BeginFrame(void) { 
	//Platform_LogConst("--- Frame ---");
}

void Gfx_EndFrame(void) {
	//Platform_LogConst("--- EF1 ---");

	q = draw_finish(q);
	
	// Fill out and then send DMA chain
	DMATAG_END(dma_tag, (q - current->data) - 1, 0, 0, 0);
	dma_wait_fast();
	dma_channel_send_chain(DMA_CHANNEL_GIF, current->data, q - current->data, 0, 0);
	//Platform_LogConst("--- EF2 ---");
		
	draw_wait_finish();
	//Platform_LogConst("--- EF3 ---");
	
	if (gfx_vsync) graph_wait_vsync();
	
	context ^= 1;
	UpdateContext();

	// Double buffering
	q = draw_framebuffer(q, 0, fb_draw);
	graph_set_framebuffer_filtered(fb_display->address,
                                   fb_display->width,
                                   fb_display->psm, 0, 0);
	//Platform_LogConst("--- EF4 ---");
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor( 0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	VU0_VECTOR clip_scale;
	unsigned int maxZ = 1 << (24 - 1); // TODO: half this? or << 24 instead?

	vp_origin.x =  ftoi4(2048 - (x / 2));
	vp_origin.y = -ftoi4(2048 - (y / 2));
	vp_origin.z =  maxZ / 2.0f;
	LoadViewportOrigin(&vp_origin);

	vp_scale.x =  16 * (w / 2);
	vp_scale.y = -16 * (h / 2);
	vp_scale.z =  maxZ / 2.0f;
	LoadViewportScale(&vp_scale);

	float hwidth  = w / 2;
	float hheight = h / 2;
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
	
	clip_scale.x = hwidth  / 2048.0f;
	clip_scale.y = hheight / 2048.0f;
	clip_scale.z = 1.0f;
	clip_scale.w = 1.0f;
	
	LoadClipScaleFactors(&clip_scale);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	int context = 0;
	
	PACK_GIFTAG(q, GIF_SET_TAG(1,0,0,0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_SCISSOR(x, x+w-1, y,y+h-1), GS_REG_SCISSOR + context);
	q++;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using PS2 --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
