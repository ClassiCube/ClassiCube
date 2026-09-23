#define CC_DYNAMIC_VBS_ARE_STATIC
#define CC_SCRATCH_VBS_ARE_SHARED_DYNAMIC
#define OVERRIDE_BEGEND2D_FUNCTIONS
#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_SOFTMIN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

static cc_bool faceCulling;
static int fb_width, fb_height; 
static struct Bitmap fb_bmp;
static int vp_hwidth, vp_hheight;
static int fb_maxX, fb_maxY;

static BitmapCol* colorBuffer;
static BitmapCol clearColor;
static cc_bool colWrite = true;
static int cb_stride;

static void* gfx_vertices;
static GfxResourceID white_square;

static void Gfx_RestoreState(void) {
	InitDefaultResources();

	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	Gfx.Created      = true;
	Gfx.BackendType  = CC_GFX_BACKEND_SOFTGPU;
	Gfx.Limitations  = GFX_LIMIT_MINIMAL | GFX_LIMIT_WORLD_ONLY;
}

static void FreeFramebuffer(void) {
	Window_FreeFramebuffer(&fb_bmp);
	fb_width  = 0;
	fb_height = 0;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
	FreeFramebuffer();
}


/*########################################################################################################################*
*-------------------------------------------------------Fixed point-------------------------------------------------------*
*#########################################################################################################################*/
// 16.16 fixed point
#define FP_SHIFT 16

#define float_to_FP(x) ((int)((x) * FP_ONE))
#define FP_to_float(x) ((float)(x) / FP_ONE)
#define FP_to_int(x)   ((x) >> FP_SHIFT)
#define int_to_FP(x)   ((x) << FP_SHIFT)

#define FP_mul(a, b) (((cc_int64)(a) * (b)) >> FP_SHIFT)
#define FP_div(a, b) (((cc_int64)(a) << FP_SHIFT) / (b))

#define FP_ONE   int_to_FP(1)
#define FP_HALF  (FP_ONE >> 1)


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
typedef struct CCTexture {
	unsigned short width, height;
	BitmapCol pixels[];
} CCTexture;

static BitmapCol* curTexPixels;
static int curTexWidth,  curTexHeight;
static int texWidthMask, texHeightMask;
static int texUShift,    texVShift;
		
void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = texId;

	curTexPixels = tex->pixels;
	curTexWidth  = tex->width;
	curTexHeight = tex->height;

	int log2_width  = Math_ilog2(tex->width);
	int log2_height = Math_ilog2(tex->height);

	texWidthMask  = (1 << log2_width)  - 1;
	texHeightMask = (1 << log2_height) - 1;
	
	texUShift = FP_SHIFT - log2_width;
	texVShift = FP_SHIFT - log2_height;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}
		
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)Mem_TryAlloc(2 + bmp->width * bmp->height, BITMAPCOLOR_SIZE);
	if (!tex) return NULL;

	tex->width  = bmp->width;
	tex->height = bmp->height;

	CopyPixels(tex->pixels, bmp->width * BITMAPCOLOR_SIZE,
			   bmp->scan0,  rowWidth * BITMAPCOLOR_SIZE,
			   bmp->width,  bmp->height);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	BitmapCol* dst = (tex->pixels + x) + y * tex->width;

	CopyPixels(dst,         tex->width * BITMAPCOLOR_SIZE,
			   part->scan0, rowWidth   * BITMAPCOLOR_SIZE,
			   part->width, part->height);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*--------------------------------------------------------2D drawing-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Begin2D(int width, int height) {
	gfx_rendering2D = true;
	Gfx_SetAlphaBlending(true);
}

void Gfx_End2D(void) {
	gfx_rendering2D = false;
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)	{ }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value) 	{ }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) {
	faceCulling = enabled;
}

static void SetAlphaTest(cc_bool enabled) {
	/* Uses value from Gfx_SetAlphaTest */
}

static void SetAlphaBlend(cc_bool enabled) {
	/* Uses value from Gfx_SetAlphaBlending */
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void ClearColorBuffer(void) {
#ifdef CC_BUILD_GBA
	/* in mGBA, fast clear takes ~3ms compared to ~52ms of standard code below */
	extern void VRAM_FastClear(BitmapCol color);
	VRAM_FastClear(clearColor);
#else
	int i, x, y, size = fb_width * fb_height;

	if (cb_stride == fb_width) {
		for (i = 0; i < size; i++) colorBuffer[i] = clearColor;
	} else {
		/* Slower partial buffer clear */
		for (y = 0; y < fb_height; y++) {
			i = y * cb_stride;
			for (x = 0; x < fb_width; x++) {
				colorBuffer[i + x] = clearColor;
			}
		}
	}
#endif
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	if (buffers & GFX_BUFFER_COLOR) ClearColorBuffer();
}

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color);
	int G = PackedCol_G(color);
	int B = PackedCol_B(color);
	int A = PackedCol_A(color);

	clearColor = BitmapCol_Make(R, G, B, A);
}

void Gfx_SetDepthTest(cc_bool enabled) {
}

void Gfx_SetDepthWrite(cc_bool enabled) {
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	colWrite = !depthOnly;
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
struct FPVertexCommon   { int x, y, z; };
struct FPVertexColoured { int x, y, z; PackedCol c; };
struct FPVertexTextured { int x, y, z; PackedCol c; int u, v; };

static VertexFormat buf_fmt;
static int buf_count;

static void PreprocessTexturedVertices(void* vertices) {
	struct FPVertexTextured* dst = vertices;
	struct VertexTextured* src   = vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = float_to_FP(src->x);
		dst->y = float_to_FP(src->y);
		dst->z = float_to_FP(src->z);
		dst->u = float_to_FP(src->U);
		dst->v = float_to_FP(src->V);
	}
}

static void PreprocessColouredVertices(void* vertices) {
	struct FPVertexColoured* dst = vertices;
	struct VertexColoured* src   = vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = float_to_FP(src->x);
		dst->y = float_to_FP(src->y);
		dst->z = float_to_FP(src->z);
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
    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices(vb);
    } else {
        PreprocessColouredVertices(vb);
    }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
struct FPVec4   { int x, y, z, w; };
struct FPMatrix { struct FPVec4 row1, row2, row3, row4; };

static struct Matrix _view, _proj;
static struct FPMatrix _mvp;

static void SetMVP(const struct Matrix* mvp) {
	float* src = (float*)mvp;
	int*   dst = (int*)&_mvp;
	
	for (int i = 0; i < 4 * 4; i++)
	{
		dst[i] = float_to_FP(src[i]);
	}
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	struct Matrix mvp;
	Matrix_Mul(&mvp, &_view, &_proj);
	SetMVP(&mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_view = *view;
	_proj = *proj;

	Matrix_Mul(mvp, view, proj);
	SetMVP(mvp);
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO: implement? but clouds aren't drawn anyways
}

void Gfx_DisableTextureOffset(void) {
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z =  1.0f / (zNear - zFar);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = zNear / (zNear - zFar);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;

	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = zFar / (zNear - zFar);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear * zFar) / (zNear - zFar);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
typedef struct Vertex_ {
	cc_int64 x, y, z, w;
	int u, v;
	PackedCol c;
} Vertex;

static void TransformVertex2D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	struct FPVertexCommon* pos = (struct FPVertexCommon*)ptr;
	vertex->x = pos->x;
	vertex->y = pos->y;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct FPVertexColoured* v = (struct FPVertexColoured*)ptr;
		vertex->u = 0;
		vertex->v = 0;
		vertex->c = v->c;
	} else {
		struct FPVertexTextured* v = (struct FPVertexTextured*)ptr;
		vertex->u = v->u;
		vertex->v = v->v;
		vertex->c = v->c;
	}
}

static int TransformVertex3D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	struct FPVertexCommon* pos = (struct FPVertexCommon*)ptr;

	cc_int64 src_x = pos->x;
	cc_int64 src_y = pos->y;
	cc_int64 src_z = pos->z;
	cc_int64 src_w = FP_ONE; // TODO << shift instead?

	cc_int64 x = src_x * _mvp.row1.x + src_y * _mvp.row2.x + src_z * _mvp.row3.x + src_w * _mvp.row4.x;
	cc_int64 y = src_x * _mvp.row1.y + src_y * _mvp.row2.y + src_z * _mvp.row3.y + src_w * _mvp.row4.y;
	cc_int64 z = src_x * _mvp.row1.z + src_y * _mvp.row2.z + src_z * _mvp.row3.z + src_w * _mvp.row4.z;
	cc_int64 w = src_x * _mvp.row1.w + src_y * _mvp.row2.w + src_z * _mvp.row3.w + src_w * _mvp.row4.w;

	vertex->x = x >> FP_SHIFT;
	vertex->y = y >> FP_SHIFT;
	vertex->z = z >> FP_SHIFT;
	vertex->w = w >> FP_SHIFT;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct FPVertexColoured* v = (struct FPVertexColoured*)ptr;
		vertex->u = 0;
		vertex->v = 0;
		vertex->c = v->c;
	} else {
		struct FPVertexTextured* v = (struct FPVertexTextured*)ptr;
		vertex->u = v->u;
		vertex->v = v->v;
		vertex->c = v->c;
	}
	return z >= 0;
}

static void ViewportVertex3D(Vertex* vertex) {
	cc_int64 invW = FP_div(FP_ONE, vertex->w);

	cc_int64 x_ndc = FP_mul(vertex->x, invW);
	cc_int64 y_ndc = FP_mul(vertex->y, invW);
	cc_int64 z_ndc = FP_mul(vertex->z, invW);

	vertex->x = vp_hwidth  + FP_mul(x_ndc, vp_hwidth);
	vertex->y = vp_hheight - FP_mul(y_ndc, vp_hheight);
	vertex->z = z_ndc;
	vertex->w = invW;
}

static void DrawSprite2D(Vertex* V0, Vertex* V1, Vertex* V2) {
	PackedCol vColor = V0->c;
	int minX = FP_to_int(V0->x);
	int minY = FP_to_int(V0->y);
	int maxX = FP_to_int(V1->x);
	int maxY = FP_to_int(V2->y);

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	int begTX = (V0->u >> texUShift);
	int begTY = (V0->v >> texVShift);
	int delTX = (V1->u >> texUShift) - begTX;
	int delTY = (V2->v >> texVShift) - begTY;

	int width = maxX - minX, height = maxY - minY;
	if (width == 0) width = 1;
	if (height == 0) height = 1;

	int fast =  delTX == width && delTY == height && 
				(begTX + delTX < curTexWidth ) && 
				(begTY + delTY < curTexHeight);

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	int x, y;
	if (fast) {
		for (y = minY; y <= maxY; y++) 
		{
			int texY = begTY + (y - minY);
			for (x = minX; x <= maxX; x++) 
			{
				int texX = begTX + (x - minX);
				int texIndex = texY * curTexWidth + texX;
				BitmapCol color = curTexPixels[texIndex];
				#include "Graphics_SoftMin.sprite.i"
			}
		}
	} else if (delTX == 0 && delTY == 0) {
		int texY = begTY & texHeightMask;
		int texX = begTX & texWidthMask;
		int texIndex = texY * curTexWidth + texX;
		BitmapCol color = curTexPixels[texIndex];

		for (y = minY; y <= maxY; y++) 
		{
			for (x = minX; x <= maxX; x++) 
			{
				#include "Graphics_SoftMin.sprite.i"
			}
		}
	} else {
		for (y = minY; y <= maxY; y++) 
		{
			int texY = ((begTY + delTY * (y - minY) / height)) & texHeightMask;
			for (x = minX; x <= maxX; x++) 
			{
				int texX = ((begTX + delTX * (x - minX) / width)) & texWidthMask;
				int texIndex = texY * curTexWidth + texX;
				BitmapCol color = curTexPixels[texIndex];
				#include "Graphics_SoftMin.sprite.i"
			}
		}
	}
}

#define edgeFunction(ax,ay, bx,by, cx,cy) (((bx) - (ax)) * ((cy) - (ay)) - ((by) - (ay)) * ((cx) - (ax)))

#define MultiplyColors(vColor, tColor) \
	a1 = PackedCol_A(vColor); \
	a2 = BitmapCol_A(tColor); \
	A  = ( a1 * a2 ) >> 8;    \
\
	r1 = PackedCol_R(vColor); \
	r2 = BitmapCol_R(tColor); \
	R  = ( r1 * r2 ) >> 8;    \
\
	g1 = PackedCol_G(vColor); \
	g2 = BitmapCol_G(tColor); \
	G  = ( g1 * g2 ) >> 8;    \
\
	b1 = PackedCol_B(vColor); \
	b2 = BitmapCol_B(tColor); \
	B  = ( b1 * b2 ) >> 8;    \

static void DrawTriangle3D(Vertex* V0, Vertex* V1, Vertex* V2) {
	int x0 = FP_to_int(V0->x), y0 = FP_to_int(V0->y);
	int x1 = FP_to_int(V1->x), y1 = FP_to_int(V1->y);
	int x2 = FP_to_int(V2->x), y2 = FP_to_int(V2->y);
	int minX = min(x0, min(x1, x2));
	int minY = min(y0, min(y1, y2));
	int maxX = max(x0, max(x1, x2));
	int maxY = max(y0, max(y1, y2));

	int area = edgeFunction(x0,y0, x1,y1, x2,y2);
	if (faceCulling) {
		// https://gamedev.stackexchange.com/questions/203694/how-to-make-backface-culling-work-correctly-in-both-orthographic-and-perspective
		if (area < 0) return;
	}

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);
	
	// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
	// Essentially these are the deltas of edge functions between X/Y and X/Y + 1 (i.e. one X/Y step)
	int dx01 = y0 - y1, dy01 = x1 - x0;
	int dx12 = y1 - y2, dy12 = x2 - x1;
	int dx20 = y2 - y0, dy20 = x0 - x2;

	int bc0_start = edgeFunction(x1,y1, x2,y2, minX, minY);
	int bc1_start = edgeFunction(x2,y2, x0,y0, minX, minY);
	int bc2_start = edgeFunction(x0,y0, x1,y1, minX, minY);

	PackedCol color = V0->c;
	int R, G, B, A, x, y;
	int a1, r1, g1, b1;
	int a2, r2, g2, b2;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		R = PackedCol_R(color);
		G = PackedCol_G(color);
		B = PackedCol_B(color);
		A = PackedCol_A(color);
	} else {
		/* Always use a single pixel */
		int rawY0 = V0->v >> texVShift;
		int rawY1 = V1->v >> texVShift;

		int rawY = min(rawY0, rawY1);
		int texY = rawY & texHeightMask;
		MultiplyColors(color, curTexPixels[texY * curTexWidth]);
	}

	if (gfx_alphaTest && A == 0) return;
	

	if (!gfx_alphaBlend) {
		#define PIXEL_PLOT_FUNC(index, x, y) \
			colorBuffer[index] = color;

		color = BitmapCol_Make(R, G, B, 0xFF);
		#include "Graphics_SoftMin.tri.i"
		#undef PIXEL_PLOT_FUNC
	} else {
		// Hardcode for alpha of 128
		#define PIXEL_PLOT_FUNC(index, x, y) \
			BitmapCol dst = colorBuffer[index];  \
			int finR = (R + BitmapCol_R(dst)) >> 1; \
			int finG = (G + BitmapCol_G(dst)) >> 1; \
			int finB = (B + BitmapCol_B(dst)) >> 1; \
			colorBuffer[index] = BitmapCol_Make(finR, finG, finB, 0xFF);

		#include "Graphics_SoftMin.tri.i"
		#undef PIXEL_PLOT_FUNC
	}
}

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

// https://github.com/behindthepixels/EDXRaster/blob/master/EDXRaster/Core/Clipper.h
static void ClipLine(Vertex* v1, Vertex* v2, Vertex* V) {
	cc_int64 t  = FP_div(v1->z, v2->z - v1->z);
	if (t < 0) t = -t;
	
	V->x = v1->x + FP_mul(t, v2->x - v1->x);
	V->y = v1->y + FP_mul(t, v2->y - v1->y);
	//V->z = v1->z + FP_mul(t, v2->z - v1->z);
	V->z = 0; // clipped against near plane anyways (I.e Z/W = 0 --> Z = 0)
	V->w = v1->w + FP_mul(t, v2->w - v1->w);
	
	V->u = t < FP_HALF ? v1->u : v2->u;
	V->v = t < FP_HALF ? v1->v : v2->v;
	V->c = v1->c;//PackedCol_Make(255, 0, 0, 255);
}

// https://casual-effects.com/research/McGuire2011Clipping/clip.glsl
static void DrawClipped(int mask, Vertex* v0, Vertex* v1, Vertex* v2, Vertex* v3) {
	Vertex tmp[2];
	Vertex* a = &tmp[0];
	Vertex* b = &tmp[1];

    switch (mask) {
	case V0_VIS:
	{
		//		   v0
		//		  / |
		//       /   |
		// .....A....B...
		//    /      |
		//  v3--v2---v1
		ClipLine(v3, v0, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v0, a, b);
	}
    break;
	case V1_VIS:
	{
		//		   v1
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v0--v3---v2
		ClipLine(v0, v1, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, b, v1);
	} break;
	case V2_VIS:
	{
		//		   v2
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v1--v0---v3
		ClipLine(v1, v2, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, b, v2);
	} break;
	case V3_VIS:
	{
		//		   v3
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v2--v1---v0
		ClipLine(v2, v3, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);
		
		DrawTriangle3D(b, v3, a);
	}
	break;
	case V0_VIS | V1_VIS:
	{
		//    v0-----------v1
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v3-----v2
		ClipLine(v1, v2, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1, v0,  a);
		DrawTriangle3D(a,  v0,  b);
	} break;
	// case V0_VIS | V2_VIS: degenerate case that should never happen
	case V0_VIS | V3_VIS:
	{
		//    v3-----------v0
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v2-----v1
		ClipLine(v0, v1, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, v0,  b);
		DrawTriangle3D(b, v0, v3);
	} break;
	case V1_VIS | V2_VIS:
	{
		//    v1-----------v2
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v0-----v3
		ClipLine(v2, v3, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1,  b, v2);
		DrawTriangle3D(v2,  b,  a);
	} break;
	// case V1_VIS | V3_VIS: degenerate case that should never happen
	case V2_VIS | V3_VIS:
	{
		//    v2-----------v3
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v1-----v0
		ClipLine(v3, v0, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D( b,  a, v2);
		DrawTriangle3D(v2,  a, v3);
	} break;
	case V0_VIS | V1_VIS | V2_VIS:
	{
		//		  --v1--
		//    v0--      --v2
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v3
		// v1,v2,v0  v2,v0,A  v0,A,B
		ClipLine(v2, v3, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1, v0, v2);
		DrawTriangle3D(v2, v0,  a);
		DrawTriangle3D(v0,  b,  a);
	} break;
	case V0_VIS | V1_VIS | V3_VIS:
	{
		//		  --v0--
		//    v3--      --v1
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v2
		// v0,v1,v3  v1,v3,A  v3,A,B
		ClipLine(v1, v2, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v0, v3, v1);
		DrawTriangle3D(v1, v3,  a);
		DrawTriangle3D(v3,  b,  a);
	} break;
	case V0_VIS | V2_VIS | V3_VIS:
	{
		//		  --v3--
		//    v2--      --v0
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v1
		// v3,v0,v2  v0,v2,A  v2,A,B
		ClipLine(v0, v1, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v3, v2, v0);
		DrawTriangle3D(v0, v2,  a);
		DrawTriangle3D(v2,  b,  a);
	} break;
	case V1_VIS | V2_VIS | V3_VIS:
	{
		//		  --v2--
		//    v1--      --v3
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v0
		// v2,v3,v1  v3,v1,A  v1,A,B
		ClipLine(v3, v0, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v2, v1, v3);
		DrawTriangle3D(v3, v1,  a);
		DrawTriangle3D(v1,  b,  a);
	} break;
	}
}

void DrawQuads(int startVertex, int verticesCount, DrawHints hints) {
	Vertex vertices[4];
	int i, j = startVertex;

	if (gfx_rendering2D && (hints & (DRAW_HINT_SPRITE|DRAW_HINT_RECT))) {
		// 4 vertices = 1 quad = 2 triangles
		for (i = 0; i < verticesCount / 4; i++, j += 4)
		{
			TransformVertex2D(j + 0, &vertices[0]);
			TransformVertex2D(j + 1, &vertices[1]);
			TransformVertex2D(j + 2, &vertices[2]);

			DrawSprite2D(&vertices[0], &vertices[1], &vertices[2]);
		}
	} else if (gfx_rendering2D) {
		Platform_LogConst("2D triangle unsupported..");
	} else if (colWrite) {
		// 4 vertices = 1 quad = 2 triangles
		for (i = 0; i < verticesCount / 4; i++, j += 4)
		{
			int clip = TransformVertex3D(j + 0, &vertices[0]) << 0
					|  TransformVertex3D(j + 1, &vertices[1]) << 1
					|  TransformVertex3D(j + 2, &vertices[2]) << 2
					|  TransformVertex3D(j + 3, &vertices[3]) << 3;

			if (clip == 0) {
				// Quad entirely clipped
			} else if (clip == 0x0F) {
				// Quad entirely visible
				ViewportVertex3D(&vertices[0]);
				ViewportVertex3D(&vertices[1]);
				ViewportVertex3D(&vertices[2]);
				ViewportVertex3D(&vertices[3]);

				DrawTriangle3D(&vertices[0], &vertices[2], &vertices[1]);
				DrawTriangle3D(&vertices[2], &vertices[0], &vertices[3]);
			} else {
				// Quad partially visible
				DrawClipped(clip, &vertices[0], &vertices[1], &vertices[2], &vertices[3]);
			}
		}
	}
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) { } /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	DrawQuads(startVertex, verticesCount, hints);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawQuads(0, verticesCount, DRAW_HINT_NONE);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex, DrawHints hints) {
	DrawQuads(startVertex, verticesCount, hints);
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* CB_GetRow(struct Bitmap* bmp, int y, void* ctx) {
	return colorBuffer + cb_stride * y;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	Bitmap_Init(bmp, fb_width, fb_height, NULL);
	return Png_Encode(&bmp, output, CB_GetRow, false, NULL);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_BeginFrame(void) { }

void Gfx_EndFrame(void) {
    Rect2D r;
	r.x = 0;
	r.y = 0; 
	r.width  = fb_width;
	r.height = fb_height;
	
	Window_DrawFramebuffer(r, &fb_bmp);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(int width, int height) {
	if (fb_width || fb_height) FreeFramebuffer();

	fb_width  = width;
	fb_height = height;

	Window_AllocFramebuffer(&fb_bmp, width, height);
	colorBuffer = fb_bmp.scan0;
	cb_stride   = fb_bmp.width;

	Gfx_SetViewport(0, 0, width, height);
	Gfx_SetScissor (0, 0, width, height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	vp_hwidth  = float_to_FP(w / 2.0f);
	vp_hheight = float_to_FP(h / 2.0f);
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	/* TODO minX/Y */
	fb_maxX = x + w - 1;
	fb_maxY = y + h - 1;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using software --\n");
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
