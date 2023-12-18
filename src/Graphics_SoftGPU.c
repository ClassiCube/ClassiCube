#include "Core.h"
#if defined CC_BUILD_SOFTGPU
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"

static cc_bool faceCulling;
static int width, height; 
static struct Bitmap fb_bmp;
static float vp_hwidth, vp_hheight;
static int sc_maxX, sc_maxY;

static cc_bool alphaBlending;
static cc_bool alphaTest;

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
		
void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	CCTexture* tex = texId;

	curTexture   = tex;
	curTexPixels = tex->pixels;
	curTexWidth  = tex->width;
	curTexHeight = tex->height;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}
		
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	int size = bmp->width * bmp->height * 4;
	CCTexture* tex = (CCTexture*)Mem_Alloc(2 + bmp->width * bmp->height, 4, "Texture");

	tex->width  = bmp->width;
	tex->height = bmp->height;
	Mem_Copy(tex->pixels, bmp->scan0, size);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)texId;
	cc_uint32* dst = (tex->pixels + x) + y * tex->width;
	CopyTextureData(dst, tex->width * 4, part, rowWidth << 2);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_SetTexturing(cc_bool enabled) { }
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

void Gfx_SetAlphaTest(cc_bool enabled) {
	alphaTest = enabled;
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	alphaBlending = enabled;
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_Clear(void) {
	int i, size = width * height;

	for (i = 0; i < size; i++) colorBuffer[i] = clearColor;
	for (i = 0; i < size; i++) depthBuffer[i] = 1.0f;
}

void Gfx_ClearCol(PackedCol color) {
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

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) { }

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

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

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
typedef struct Vector4 { float X, Y, Z, W; } Vector4;
typedef struct Vector3 { float X, Y, Z; } Vector3;
typedef struct Vector2 { float X, Y; } Vector2;

static void TransformVertex(int index, Vector4* frag, Vector2* uv, PackedCol* color) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;

	Vector4 coord;
	coord.x = pos->x * mvp.row1.x + pos->y * mvp.row2.x + pos->z * mvp.row3.x + mvp.row4.x;
	coord.y = pos->x * mvp.row1.y + pos->y * mvp.row2.y + pos->z * mvp.row3.y + mvp.row4.y;
	coord.z = pos->x * mvp.row1.z + pos->y * mvp.row2.z + pos->z * mvp.row3.z + mvp.row4.z;
	coord.w = pos->x * mvp.row1.w + pos->y * mvp.row2.w + pos->z * mvp.row3.w + mvp.row4.w;

	frag->x = vp_hwidth  * (1 + coord.x / coord.w);
	frag->y = vp_hheight * (1 - coord.y / coord.w);
	frag->z = coord.z / coord.w;
	frag.w = 1.0f    / coord.w;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		*color = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		*color = v->Col;
		uv->x  = v->U + texOffsetX;
		uv->y  = v->V + texOffsetY;
	}
}
	
static int MultiplyColours(PackedCol vColor, BitmapCol tColor) {
	int a1 = PackedCol_A(vColor), a2 = BitmapCol_A(tColor);
	int a  = ( a1 * a2 ) / 255;
	int r1 = PackedCol_R(vColor), r2 = BitmapCol_R(tColor);
	int r  = ( r1 * r2 ) / 255;
	int g1 = PackedCol_G(vColor), g2 = BitmapCol_G(tColor);
	int g  = ( g1 * g2 ) / 255;
	int b1 = PackedCol_B(vColor), b2 = BitmapCol_B(tColor);
	int b  = ( b1 * b2 ) / 255;
	return PackedCol_Make(r, g, b, a);
}

static void DrawTriangle(Vector4 frag1, Vector4 frag2, Vector4 frag3,
						Vector2 uv1, Vector2 uv2, Vector2 uv3, PackedCol color) {
	int x1 = (int)frag1.x, y1 = (int)frag1.y;
	int x2 = (int)frag2.x, y2 = (int)frag2.y;
	int x3 = (int)frag3.x, y3 = (int)frag3.y;
	int minX = min(x1, min(x2, x3));
	int minY = min(y1, min(y2, y3));
	int maxX = max(x1, max(x2, x3));
	int maxY = max(y1, max(y2, y3));

	// TODO backface culling

	// Reject triangles completely outside
	if (minX < 0 && maxX < 0 || minX >= width  && maxX >= width ) return;
	if (minY < 0 && maxY < 0 || minY >= height && maxY >= height) return;

	// Perform scissoring
	//minX = max(minX, 0); maxX = min(maxX, sc_maxX);
	//minY = max(minY, 0); maxY = min(maxY, sc_maxY);
	// TODO why doesn't this work 

	// NOTE: W in frag variables below is actually 1/W 

	float factor = 1.0f / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			if (x < 0 || y < 0 || x >= width || y >= height) return;

			float ic0 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) * factor;
			if (ic0 < 0 || ic0 > 1) continue;
			float ic1 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) * factor;
			if (ic1 < 0 || ic1 > 1) continue;
			float ic2 = 1.0f - ic0 - ic1;
			if (ic2 < 0 || ic2 > 1) continue;

			int index = y * width + x;
			float w = 1 / (ic0 * frag1.w + ic1 * frag2.w + ic2 * frag3.w);

			if (depthTest && w <= depthBuffer[index]) continue;
			if (depthWrite) depthBuffer[index] = w;
			if (!colWrite)  continue;

			PackedCol fragColor = color;
			if (gfx_format == VERTEX_FORMAT_TEXTURED) {
				float u = (ic0 * uv1.x * frag1.w + ic1 * uv2.x * frag2.w + ic2 * uv3.x * frag3.w) * w;
				float v = (ic0 * uv1.y * frag1.w + ic1 * uv2.y * frag2.w + ic2 * uv3.y * frag3.w) * w;
				int texX = (int)(Math_AbsF(u - Math_Floor(u)) * curTexWidth);
				int texY = (int)(Math_AbsF(v - Math_Floor(v)) * curTexHeight);
				int texIndex = texY * curTexWidth + texX;

				fragColor = MultiplyColours(fragColor, curTexPixels[texIndex]);
			}

			int R = PackedCol_R(fragColor);
			int G = PackedCol_G(fragColor);
			int B = PackedCol_B(fragColor);
			int A = PackedCol_A(fragColor);

			if (alphaBlending) {
				PackedCol dst = colorBuffer[index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = (R * A) / 255 + (dstR * (255 - A)) / 255;
				G = (G * A) / 255 + (dstG * (255 - A)) / 255;
				B = (B * A) / 255 + (dstB * (255 - A)) / 255;
			}
			if (alphaTest && A < 0x80) continue;

			colorBuffer[index] = BitmapCol_Make(R, G, B, 0xFF);
		}
	}
}

void DrawQuads(int startVertex, int verticesCount) {
	Vector4 frag[4];
	Vector2 uv[4];
	PackedCol color[4];
	int j = startVertex;

	// 4 vertices = 1 quad = 2 triangles
	for (int i = 0; i < verticesCount / 4; i++, j += 4)
	{
		TransformVertex(j + 0, &frag[0], &uv[0], &color[0]);
		TransformVertex(j + 1, &frag[1], &uv[1], &color[1]);
		TransformVertex(j + 2, &frag[2], &uv[2], &color[2]);
		TransformVertex(j + 3, &frag[3], &uv[3], &color[3]);

		DrawTriangle(frag[0], frag[1], frag[2], 
					uv[0], uv[1], uv[2], color[0]);

		DrawTriangle(frag[2], frag[3], frag[0],
					uv[2], uv[3], uv[0], color[2]);
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
	return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_BeginFrame(void) { }

void Gfx_EndFrame(void) {
	Rect2D r = { 0, 0, width, height };
	Window_DrawFramebuffer(r);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	if (depthBuffer) DestroyBuffers();

	fb_bmp.width  = width  = Game.Width;
	fb_bmp.height = height = Game.Height;

	vp_hwidth  = width  / 2.0f;
	vp_hheight = height / 2.0f;

	sc_maxX = width  - 1;
	sc_maxY = height - 1;

	Window_AllocFramebuffer(&fb_bmp);
	depthBuffer = Mem_Alloc(width * height, 4, "depth buffer");
	colorBuffer = fb_bmp.scan0;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	String_Format1(info, "-- Using software (%i bit) --\n", &pointerSize);
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif