#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_SOFTGPU
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

static cc_bool faceCulling;
static int fb_width, fb_height; 
static struct Bitmap fb_bmp;
static float vp_hwidth, vp_hheight;
static int fb_maxX, fb_maxY;

static BitmapCol* colorBuffer;
static BitmapCol clearColor;
static cc_bool colWrite = true;
static int cb_stride;

static float* depthBuffer;
static cc_bool depthTest  = true;
static cc_bool depthWrite = true;
static int db_stride;

static void* gfx_vertices;
static GfxResourceID white_square;

void Gfx_RestoreState(void) {
	InitDefaultResources();

	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
#if defined CC_BUILD_32X || defined CC_BUILD_GBA
	Gfx.MaxTexWidth  = 16;
	Gfx.MaxTexHeight = 16;
#else
	Gfx.MaxTexWidth  = 4096;
	Gfx.MaxTexHeight = 4096;
#endif

	Gfx.Created      = true;
	Gfx.BackendType  = CC_GFX_BACKEND_SOFTGPU;
	
	Gfx_RestoreState();
}

static void DestroyBuffers(void) {
	Window_FreeFramebuffer(&fb_bmp);
	Mem_Free(depthBuffer);
	depthBuffer = NULL;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
	DestroyBuffers();
}


typedef struct CCTexture {
	int width, height;
	BitmapCol pixels[];
} CCTexture;

static CCTexture* curTexture;
static BitmapCol* curTexPixels;
static int curTexWidth, curTexHeight;
static int texWidthMask, texHeightMask;
static int texSinglePixel;
		
void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = texId;

	curTexture   = tex;
	curTexPixels = tex->pixels;
	curTexWidth  = tex->width;
	curTexHeight = tex->height;

	texWidthMask   = (1 << Math_ilog2(tex->width))  - 1;
	texHeightMask  = (1 << Math_ilog2(tex->height)) - 1;
	texSinglePixel = curTexWidth == 1 && curTexHeight == 1;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}
		
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)Mem_Alloc(2 + bmp->width * bmp->height, 4, "Texture");

	tex->width  = bmp->width;
	tex->height = bmp->height;
	CopyTextureData(tex->pixels, bmp->width * BITMAPCOLOR_SIZE,
					bmp, rowWidth * BITMAPCOLOR_SIZE);
	return tex;
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
}

static void ClearDepthBuffer(void) {
#ifndef SOFTGPU_DISABLE_ZBUFFER
	int i, size = fb_width * fb_height;
	for (i = 0; i < size; i++) depthBuffer[i] = 100000000.0f;
#endif
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	if (buffers & GFX_BUFFER_COLOR) ClearColorBuffer();
	if (buffers & GFX_BUFFER_DEPTH) ClearDepthBuffer();
}

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color);
	int G = PackedCol_G(color);
	int B = PackedCol_B(color);
	int A = PackedCol_A(color);

	clearColor = BitmapCol_Make(R, G, B, A);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depthTest = enabled;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depthWrite = enabled;
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
static float texOffsetX, texOffsetY;
static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&_mvp, &_view, &_proj);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_view = *view;
	_proj = *proj;

	Matrix_Mul(mvp, view, proj);
	_mvp  = *mvp;
}

void Gfx_EnableTextureOffset(float x, float y) {
	texOffsetX = x;
	texOffsetY = y;
}

void Gfx_DisableTextureOffset(void) {
	texOffsetX = 0;
	texOffsetY = 0;
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
typedef struct Vector3 { float x, y, z; } Vector3;
typedef struct Vector2 { float x, y; } Vector2;
typedef struct Vertex_ {
	float x, y, z, w;
	float u, v;
	PackedCol c;
} Vertex;

static void TransformVertex2D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;
	vertex->x = pos->x;
	vertex->y = pos->y;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		vertex->u = 0.0f;
		vertex->v = 0.0f;
		vertex->c = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		vertex->u = v->U;
		vertex->v = v->V;
		vertex->c = v->Col;
	}
}

static int TransformVertex3D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;

	vertex->x = pos->x * _mvp.row1.x + pos->y * _mvp.row2.x + pos->z * _mvp.row3.x + _mvp.row4.x;
	vertex->y = pos->x * _mvp.row1.y + pos->y * _mvp.row2.y + pos->z * _mvp.row3.y + _mvp.row4.y;
	vertex->z = pos->x * _mvp.row1.z + pos->y * _mvp.row2.z + pos->z * _mvp.row3.z + _mvp.row4.z;
	vertex->w = pos->x * _mvp.row1.w + pos->y * _mvp.row2.w + pos->z * _mvp.row3.w + _mvp.row4.w;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		vertex->u = 0.0f;
		vertex->v = 0.0f;
		vertex->c = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		vertex->u = (v->U + texOffsetX);
		vertex->v = (v->V + texOffsetY);
		vertex->c = v->Col;
	}
	return vertex->z >= 0.0f;
}

static void ViewportVertex3D(Vertex* vertex) {
	float invW = 1.0f / vertex->w;

	vertex->x = vp_hwidth  * (1 + vertex->x * invW);
	vertex->y = vp_hheight * (1 - vertex->y * invW);
	vertex->z = vertex->z * invW;
	vertex->w = invW;

	vertex->u *= invW;
	vertex->v *= invW;
}

// Ensure it's inlined, whereas Math_FloorF might not be
static CC_INLINE int FastFloor(float value) {
	int valueI = (int)value;
	return valueI > value ? valueI - 1 : valueI;
}

static void DrawSprite2D(Vertex* V0, Vertex* V1, Vertex* V2) {
	PackedCol vColor = V0->c;
	int minX = (int)V0->x;
	int minY = (int)V0->y;
	int maxX = (int)V1->x;
	int maxY = (int)V2->y;

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	int begTX = (int)(V0->u * curTexWidth);
	int begTY = (int)(V0->v * curTexHeight);
	int delTX = (int)(V1->u * curTexWidth)  - begTX;
	int delTY = (int)(V2->v * curTexHeight) - begTY;

	int width = maxX - minX, height = maxY - minY;

	int fast =  delTX == width && delTY == height && 
				(begTX + delTX < curTexWidth ) && 
				(begTY + delTY < curTexHeight);

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	for (int y = minY; y <= maxY; y++) 
	{
		int texY = fast ? (begTY + (y - minY)) : (((begTY + delTY * (y - minY) / height)) & texHeightMask);
		for (int x = minX; x <= maxX; x++) 
		{
			int texX = fast ? (begTX + (x - minX)) : (((begTX + delTX * (x - minX) / width)) & texWidthMask);
			int texIndex = texY * curTexWidth + texX;

			BitmapCol color = curTexPixels[texIndex];
			int R, G, B, A;

			A = BitmapCol_A(color);
			if (gfx_alphaBlend && A == 0) continue;
			int cb_index = y * cb_stride + x;

			if (gfx_alphaBlend && A != 255) {
				BitmapCol dst = colorBuffer[cb_index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = BitmapCol_R(color);
				G = BitmapCol_G(color);
				B = BitmapCol_B(color);

				R = (R * A + dstR * (255 - A)) >> 8;
				G = (G * A + dstG * (255 - A)) >> 8;
				B = (B * A + dstB * (255 - A)) >> 8;
				color = BitmapCol_Make(R, G, B, 0xFF);
			}

			if (vColor != PACKEDCOL_WHITE) {
				int r1 = PackedCol_R(vColor), r2 = BitmapCol_R(color);
				R = ( r1 * r2 ) >> 8;
				int g1 = PackedCol_G(vColor), g2 = BitmapCol_G(color);
				G = ( g1 * g2 ) >> 8;
				int b1 = PackedCol_B(vColor), b2 = BitmapCol_B(color);
				B = ( b1 * b2 ) >> 8;

				color = BitmapCol_Make(R, G, B, 0xFF);
			}

			colorBuffer[cb_index] = color;
		}
	}
}

#define edgeFunction(ax,ay, bx,by, cx,cy) (((bx) - (ax)) * ((cy) - (ay)) - ((by) - (ay)) * ((cx) - (ax)))

static void DrawTriangle2D(Vertex* V0, Vertex* V1, Vertex* V2) {
	int x0 = (int)V0->x, y0 = (int)V0->y;
	int x1 = (int)V1->x, y1 = (int)V1->y;
	int x2 = (int)V2->x, y2 = (int)V2->y;
	int minX = min(x0, min(x1, x2));
	int minY = min(y0, min(y1, y2));
	int maxX = max(x0, max(x1, x2));
	int maxY = max(y0, max(y1, y2));

	int area = edgeFunction(x0,y0, x1,y1, x2,y2);
	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);
	float factor = 1.0f / area;

	float u0 = V0->u * curTexWidth,  u1 = V1->u * curTexWidth,  u2 = V2->u * curTexWidth;
	float v0 = V0->v * curTexHeight, v1 = V1->v * curTexHeight, v2 = V2->v * curTexHeight;
	PackedCol color = V0->c;
	
	// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
	// Essentially these are the deltas of edge functions between X/Y and X/Y + 1 (i.e. one X/Y step)
	int dx01  = y0 - y1, dy01 = x1 - x0;
	int dx12  = y1 - y2, dy12 = x2 - x1;
	int dx20  = y2 - y0, dy20 = x0 - x2;

	float bc0_start = edgeFunction(x1,y1, x2,y2, minX+0.5f,minY+0.5f);
	float bc1_start = edgeFunction(x2,y2, x0,y0, minX+0.5f,minY+0.5f);
	float bc2_start = edgeFunction(x0,y0, x1,y1, minX+0.5f,minY+0.5f);

	for (int y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
	{
		float bc0 = bc0_start;
		float bc1 = bc1_start;
		float bc2 = bc2_start;

		for (int x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
		{
			float ic0 = bc0 * factor;
			float ic1 = bc1 * factor;
			float ic2 = bc2 * factor;

			if (ic0 < 0 || ic1 < 0 || ic2 < 0) continue;
			int cb_index = y * cb_stride + x;

			int R, G, B, A;
			if (gfx_format == VERTEX_FORMAT_TEXTURED) {
				float u = ic0 * u0 + ic1 * u1 + ic2 * u2;
				float v = ic0 * v0 + ic1 * v1 + ic2 * v2;
				int texX = ((int)u) & texWidthMask;
				int texY = ((int)v) & texHeightMask;
				int texIndex = texY * curTexWidth + texX;

				BitmapCol tColor = curTexPixels[texIndex];
				int a1 = PackedCol_A(color), a2 = BitmapCol_A(tColor);
				A = ( a1 * a2 ) >> 8;
				int r1 = PackedCol_R(color), r2 = BitmapCol_R(tColor);
				R = ( r1 * r2 ) >> 8;
				int g1 = PackedCol_G(color), g2 = BitmapCol_G(tColor);
				G = ( g1 * g2 ) >> 8;
				int b1 = PackedCol_B(color), b2 = BitmapCol_B(tColor);
				B = ( b1 * b2 ) >> 8;
			} else {
				R = PackedCol_R(color);
				G = PackedCol_G(color);
				B = PackedCol_B(color);
				A = PackedCol_A(color);
			}

			if (gfx_alphaTest && A < 0x80) continue;
			if (gfx_alphaBlend && A == 0)  continue;

			if (gfx_alphaBlend && A != 255) {
				BitmapCol dst = colorBuffer[cb_index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = (R * A + dstR * (255 - A)) >> 8;
				G = (G * A + dstG * (255 - A)) >> 8;
				B = (B * A + dstB * (255 - A)) >> 8;
			}

			colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
		}
	}
}

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
	int x0 = (int)V0->x, y0 = (int)V0->y;
	int x1 = (int)V1->x, y1 = (int)V1->y;
	int x2 = (int)V2->x, y2 = (int)V2->y;
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

	// NOTE: W in frag variables below is actually 1/W 
	float factor = 1.0f / area;
	float w0 = V0->w, w1 = V1->w, w2 = V2->w;
	
	// TODO proper clipping
	if (w0 <= 0 || w1 <= 0 || w2 <= 0) {
		return;
	}

	float z0 = V0->z, z1 = V1->z, z2 = V2->z;
	float u0 = V0->u, u1 = V1->u, u2 = V2->u;
	float v0 = V0->v, v1 = V1->v, v2 = V2->v;
	PackedCol color = V0->c;
	
	// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
	// Essentially these are the deltas of edge functions between X/Y and X/Y + 1 (i.e. one X/Y step)
	int dx01  = y0 - y1, dy01 = x1 - x0;
	int dx12  = y1 - y2, dy12 = x2 - x1;
	int dx20  = y2 - y0, dy20 = x0 - x2;

	float bc0_start = edgeFunction(x1,y1, x2,y2, minX+0.5f,minY+0.5f);
	float bc1_start = edgeFunction(x2,y2, x0,y0, minX+0.5f,minY+0.5f);
	float bc2_start = edgeFunction(x0,y0, x1,y1, minX+0.5f,minY+0.5f);

	int R, G, B, A;
	int a1, r1, g1, b1;
	int a2, r2, g2, b2;
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;

	if (!texturing) {
		R = PackedCol_R(color);
		G = PackedCol_G(color);
		B = PackedCol_B(color);
		A = PackedCol_A(color);
	} else if (texSinglePixel) {
		/* Don't need to calculate complicated texturing in this case */
		MultiplyColors(color, curTexPixels[0]);
		texturing = false;
	}

	for (int y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
	{
		float bc0 = bc0_start;
		float bc1 = bc1_start;
		float bc2 = bc2_start;

		for (int x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
		{
			float ic0 = bc0 * factor;
			float ic1 = bc1 * factor;
			float ic2 = bc2 * factor;
			if (ic0 < 0 || ic1 < 0 || ic2 < 0) continue;
			int db_index = y * db_stride + x;

			float w = 1 / (ic0 * w0 + ic1 * w1 + ic2 * w2);
			float z = (ic0 * z0 + ic1 * z1 + ic2 * z2) * w;

#ifndef SOFTGPU_DISABLE_ZBUFFER
			if (depthTest && (z < 0 || z > depthBuffer[db_index])) continue;
			if (!colWrite) {
				if (depthWrite) depthBuffer[db_index] = z;
				continue;
			}
#else
			if (!colWrite) continue;
#endif

			if (texturing) {
				float u = (ic0 * u0 + ic1 * u1 + ic2 * u2) * w;
				float v = (ic0 * v0 + ic1 * v1 + ic2 * v2) * w;
				int texX = ((int)(Math_AbsF(u - FastFloor(u)) * curTexWidth )) & texWidthMask;
				int texY = ((int)(Math_AbsF(v - FastFloor(v)) * curTexHeight)) & texHeightMask;

				int texIndex = texY * curTexWidth + texX;
				BitmapCol tColor = curTexPixels[texIndex];

				MultiplyColors(color, tColor);
			}

			if (gfx_alphaTest && A < 0x80) continue;
#ifndef SOFTGPU_DISABLE_ZBUFFER
			if (depthWrite) depthBuffer[db_index] = z;
#endif
			int cb_index = y * cb_stride + x;
			
			if (!gfx_alphaBlend) {
				colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
				continue;
			}

			BitmapCol dst = colorBuffer[cb_index];
			int dstR = BitmapCol_R(dst);
			int dstG = BitmapCol_G(dst);
			int dstB = BitmapCol_B(dst);

			int finR = (R * A + dstR * (255 - A)) >> 8;
			int finG = (G * A + dstG * (255 - A)) >> 8;
			int finB = (B * A + dstB * (255 - A)) >> 8;
			colorBuffer[cb_index] = BitmapCol_Make(finR, finG, finB, 0xFF);
		}
	}
}

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

// https://github.com/behindthepixels/EDXRaster/blob/master/EDXRaster/Core/Clipper.h
static void ClipLine(Vertex* v1, Vertex* v2, Vertex* V) {
	float t  = Math_AbsF(v1->z / (v2->z - v1->z));
	float invt = 1.0f - t;
	
	V->x = invt * v1->x + t * v2->x;
	V->y = invt * v1->y + t * v2->y;
	//V->z = invt * v1->z + t * v2->z;
	V->z = 0.0f; // clipped against near plane anyways (I.e Z/W = 0 --> Z = 0)
	V->w = invt * v1->w + t * v2->w;
	
	V->u = invt * v1->u + t * v2->u;
	V->v = invt * v1->v + t * v2->v;
	V->c = v1->c;
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
	int j = startVertex;

	if (gfx_rendering2D && hints == DRAW_HINT_SPRITE) {
		// 4 vertices = 1 quad = 2 triangles
		for (int i = 0; i < verticesCount / 4; i++, j += 4)
		{
			TransformVertex2D(j + 0, &vertices[0]);
			TransformVertex2D(j + 1, &vertices[1]);
			TransformVertex2D(j + 2, &vertices[2]);

			DrawSprite2D(&vertices[0], &vertices[1], &vertices[2]);
		}
	} else if (gfx_rendering2D) {
		// 4 vertices = 1 quad = 2 triangles
		for (int i = 0; i < verticesCount / 4; i++, j += 4)
		{
			TransformVertex2D(j + 0, &vertices[0]);
			TransformVertex2D(j + 1, &vertices[1]);
			TransformVertex2D(j + 2, &vertices[2]);
			TransformVertex2D(j + 3, &vertices[3]);

			DrawTriangle2D(&vertices[0], &vertices[2], &vertices[1]);
			DrawTriangle2D(&vertices[2], &vertices[0], &vertices[3]);
		}
	} else {
		// 4 vertices = 1 quad = 2 triangles
		for (int i = 0; i < verticesCount / 4; i++, j += 4)
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

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawQuads(startVertex, verticesCount, DRAW_HINT_NONE);
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
	Rect2D r = { 0, 0, fb_width, fb_height };
	Window_DrawFramebuffer(r, &fb_bmp);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	if (depthBuffer) DestroyBuffers();

	fb_width   = Game.Width;
	fb_height  = Game.Height;

	Window_AllocFramebuffer(&fb_bmp, Game.Width, Game.Height);
	colorBuffer = fb_bmp.scan0;
	cb_stride   = fb_bmp.width;

#ifndef SOFTGPU_DISABLE_ZBUFFER
	depthBuffer = Mem_Alloc(fb_width * fb_height, 4, "depth buffer");
	db_stride   = fb_width;
#endif

	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor (0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	vp_hwidth  = w / 2.0f;
	vp_hheight = h / 2.0f;
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	/* TODO minX/Y */
	fb_maxX = x + w - 1;
	fb_maxY = y + h - 1;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	String_Format1(info, "-- Using software (%i bit) --\n", &pointerSize);
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
