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

static PackedCol* colorBuffer;
static PackedCol clearColor;
static cc_bool colWrite = true;

static float* depthBuffer;
static void* gfx_vertices;
static cc_bool depthTest  = true;
static cc_bool depthWrite = true;
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
	Gfx.MaxTexWidth  = 4096;
	Gfx.MaxTexHeight = 4096;
	Gfx.Created      = true;
	
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
		
void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = texId;

	curTexture   = tex;
	curTexPixels = tex->pixels;
	curTexWidth  = tex->width;
	curTexHeight = tex->height;

	texWidthMask  = (1 << Math_ilog2(tex->width))  - 1;
	texHeightMask = (1 << Math_ilog2(tex->height)) - 1;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}
		
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)Mem_Alloc(2 + bmp->width * bmp->height, 4, "Texture");

	tex->width  = bmp->width;
	tex->height = bmp->height;
	CopyTextureData(tex->pixels, bmp->width * 4, bmp, rowWidth << 2);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	cc_uint32* dst = (tex->pixels + x) + y * tex->width;
	CopyTextureData(dst, tex->width * 4, part, rowWidth << 2);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
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

void Gfx_ClearBuffers(GfxBuffers buffers) {
	int i, size = fb_width * fb_height;

	if (buffers & GFX_BUFFER_COLOR) {
		for (i = 0; i < size; i++) colorBuffer[i] = clearColor;
	}
	if (buffers & GFX_BUFFER_DEPTH) {
		for (i = 0; i < size; i++) depthBuffer[i] = 100000000.0f;
	}
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
static struct Matrix _view, _proj, mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
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
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For pos FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
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

static void TransformVertex(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;

	struct Vec4 coord;
	coord.x = pos->x * mvp.row1.x + pos->y * mvp.row2.x + pos->z * mvp.row3.x + mvp.row4.x;
	coord.y = pos->x * mvp.row1.y + pos->y * mvp.row2.y + pos->z * mvp.row3.y + mvp.row4.y;
	coord.z = pos->x * mvp.row1.z + pos->y * mvp.row2.z + pos->z * mvp.row3.z + mvp.row4.z;
	coord.w = pos->x * mvp.row1.w + pos->y * mvp.row2.w + pos->z * mvp.row3.w + mvp.row4.w;

	float invW = 1.0f / coord.w;
	vertex->x = vp_hwidth  * (1 + coord.x * invW);
	vertex->y = vp_hheight * (1 - coord.y * invW);
	vertex->z = coord.z * invW;
	vertex->w = invW;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		vertex->c = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		vertex->u = (v->U + texOffsetX) * invW;
		vertex->v = (v->V + texOffsetY) * invW;
		vertex->c = v->Col;
	}
}

// Ensure it's inlined, whereas Math_FloorF might not be
static CC_INLINE int FastFloor(float value) {
	int valueI = (int)value;
	return valueI > value ? valueI - 1 : valueI;
}

static void DrawTriangle(Vertex* frag1, Vertex* frag2, Vertex* frag3) {
	int x1 = (int)frag1->x, y1 = (int)frag1->y;
	int x2 = (int)frag2->x, y2 = (int)frag2->y;
	int x3 = (int)frag3->x, y3 = (int)frag3->y;
	int minX = min(x1, min(x2, x3));
	int minY = min(y1, min(y2, y3));
	int maxX = max(x1, max(x2, x3));
	int maxY = max(y1, max(y2, y3));

	// TODO backface culling
	if (faceCulling) {
		// https://gamedev.stackexchange.com/questions/203694/how-to-make-backface-culling-work-correctly-in-both-orthographic-and-perspective
		int sign = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
		if (sign > 0) return;
	}

	// Reject triangles completely outside
	if ((minX < 0 && maxX < 0) || (minX > fb_maxX && maxX > fb_maxX)) return;
	if ((minY < 0 && maxY < 0) || (minY > fb_maxY && maxY > fb_maxY)) return;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	// NOTE: W in frag variables below is actually 1/W 
	float factor = 1.0f / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	float w1 = frag1->w, w2 = frag2->w, w3 = frag3->w;
	
	// TODO proper clipping
	if (w1 <= 0 || w2 <= 0 || w3 <= 0) return;

	float z1 = frag1->z, z2 = frag2->z, z3 = frag3->z;
	float u1 = frag1->u, u2 = frag2->u, u3 = frag3->u;
	float v1 = frag1->v, v2 = frag2->v, v3 = frag3->v;
	PackedCol color = frag1->c;
	
	for (int y = minY; y <= maxY; y++) {
		float yy = y + 0.5f;
		for (int x = minX; x <= maxX; x++) {
			float xx = x + 0.5f;
			
			float ic0 = ((y2 - y3) * (xx - x3) + (x3 - x2) * (yy - y3)) * factor;
			if (ic0 < 0 || ic0 > 1) continue;
			float ic1 = ((y3 - y1) * (xx - x3) + (x1 - x3) * (yy - y3)) * factor;
			if (ic1 < 0 || ic1 > 1) continue;
			float ic2 = 1.0f - ic0 - ic1;
			if (ic2 < 0 || ic2 > 1) continue;

			int index = y * fb_width + x;
			float w = 1 / (ic0 * w1 + ic1 * w2 + ic2 * w3);
			float z = (ic0 * z1 + ic1 * z2 + ic2 * z3) * w;

			if (depthTest && (z < 0 || z > depthBuffer[index])) continue;
			if (!colWrite) {
				if (depthWrite) depthBuffer[index] = z;
				continue;
			}

			int R, G, B, A;
			if (gfx_format == VERTEX_FORMAT_TEXTURED) {
				float u = (ic0 * u1 + ic1 * u2 + ic2 * u3) * w;
				float v = (ic0 * v1 + ic1 * v2 + ic2 * v3) * w;
				int texX = ((int)(Math_AbsF(u - FastFloor(u)) * curTexWidth )) & texWidthMask;
				int texY = ((int)(Math_AbsF(v - FastFloor(v)) * curTexHeight)) & texHeightMask;
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


			if (gfx_alphaBlend) {
				BitmapCol dst = colorBuffer[index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = (R * A) / 255 + (dstR * (255 - A)) / 255;
				G = (G * A) / 255 + (dstG * (255 - A)) / 255;
				B = (B * A) / 255 + (dstB * (255 - A)) / 255;
			}
			if (gfx_alphaTest && A < 0x80) continue;

			if (depthWrite) depthBuffer[index] = z;
			colorBuffer[index] = BitmapCol_Make(R, G, B, 0xFF);
		}
	}
}

void DrawQuads(int startVertex, int verticesCount) {
	Vertex vertices[4];
	int j = startVertex;

	// 4 vertices = 1 quad = 2 triangles
	for (int i = 0; i < verticesCount / 4; i++, j += 4)
	{
		TransformVertex(j + 0, &vertices[0]);
		TransformVertex(j + 1, &vertices[1]);
		TransformVertex(j + 2, &vertices[2]);
		TransformVertex(j + 3, &vertices[3]);

		DrawTriangle(&vertices[0], &vertices[1], &vertices[2]);
		DrawTriangle(&vertices[2], &vertices[3], &vertices[0]);
	}
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) { } /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	DrawQuads(startVertex, verticesCount);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawQuads(0, verticesCount);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawQuads(startVertex, verticesCount);
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	Bitmap_Init(bmp, fb_width, fb_height, colorBuffer);
	return Png_Encode(&bmp, output, NULL, false, NULL);
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_BeginFrame(void) { }

void Gfx_EndFrame(void) {
	Rect2D r = { 0, 0, fb_width, fb_height };
	Window_DrawFramebuffer(r, &fb_bmp);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	if (depthBuffer) DestroyBuffers();

	fb_bmp.width  = fb_width  = Game.Width;
	fb_bmp.height = fb_height = Game.Height;

	vp_hwidth  = fb_width  / 2.0f;
	vp_hheight = fb_height / 2.0f;

	fb_maxX = fb_width  - 1;
	fb_maxY = fb_height - 1;

	Window_AllocFramebuffer(&fb_bmp);
	depthBuffer = Mem_Alloc(fb_width * fb_height, 4, "depth buffer");
	colorBuffer = fb_bmp.scan0;
}

void Gfx_SetViewport(int x, int y, int w, int h) { }

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	String_Format1(info, "-- Using software (%i bit) --\n", &pointerSize);
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
