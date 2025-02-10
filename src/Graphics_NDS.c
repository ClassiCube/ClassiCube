#include "Core.h"
#ifdef CC_BUILD_NDS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"
#include <nds.h>
static int matrix_modes[] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Create(void) {
	Gfx_RestoreState();

	Gfx.MinTexWidth  =   8;
	Gfx.MinTexHeight =   8;
	Gfx.MaxTexWidth  = 256;
	Gfx.MaxTexHeight = 256;
	//Gfx.MaxTexSize   = 256 * 256;
	Gfx.Created	     = true;
	Gfx.Limitations  = GFX_LIMIT_VERTEX_ONLY_FOG;
	glInit();
	
	glClearColor(0, 15, 10, 31);
	glClearPolyID(63);
	GFX_ALPHA_TEST = 7; // Alpha threshold ranges from 0 to 15
	
	glEnable(GL_ANTIALIAS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_FOG);

	glClearDepth(GL_MAX_DEPTH);
	Gfx_SetViewport(0, 0, 256, 192);
	
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	vramSetBankC(VRAM_C_TEXTURE);
	vramSetBankD(VRAM_D_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	
	Gfx_SetFaceCulling(false);
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	vramSetBankA(VRAM_A_LCD);
	vramSetBankB(VRAM_B_LCD);
	vramSetBankC(VRAM_C_LCD);
	vramSetBankD(VRAM_D_LCD);
	vramSetBankE(VRAM_E_LCD);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Nintendo DS --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) { 
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	int x2 = x + w - 1;
	int y2 = y + h - 1;
	GFX_VIEWPORT = x | (y << 8) | (x2 << 16) | (y2 << 24);
}

void Gfx_SetScissor (int x, int y, int w, int h) { }

void Gfx_BeginFrame(void) {
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	// TODO
} 

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color) >> 3;
	int G = PackedCol_G(color) >> 3;
	int B = PackedCol_B(color) >> 3;
	glClearColor(R, G, B, 31);
}

void Gfx_EndFrame(void) {
	// W buffering is used for fog
	GFX_FLUSH = GL_WBUFFERING;
	// TODO not needed?
	swiWaitForVBlank();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static int texOffsetX, texOffsetY;

static void UpdateTextureMatrix(void) {
	int width  = 0;
	int height = 0;
	glGetInt(GL_GET_TEXTURE_WIDTH,  &width);
	glGetInt(GL_GET_TEXTURE_HEIGHT, &height);
	
	// Scale uvm to fit into texture size
	MATRIX_CONTROL  = 3;
	MATRIX_IDENTITY = 0;

	MATRIX_SCALE = width  << 6; // X scale
	MATRIX_SCALE = height << 6; // Y scale
	MATRIX_SCALE = 0;           // Z scale

	if (texOffsetX || texOffsetY) {
		MATRIX_TRANSLATE = (texOffsetX * width ); // X
		MATRIX_TRANSLATE = (texOffsetY * height); // Y
		MATRIX_TRANSLATE = 0;          // Z
	}

	MATRIX_CONTROL  = matrix_modes[lastMatrix];
}

void Gfx_EnableTextureOffset(float x, float y) {
	// Looks bad due to low uvm precision
	// TODO: Right for negative x/y ?
	// TODO speed probably isn't quite right
	texOffsetX = (int)(x * 32768) & (32768 - 1);
	texOffsetY = (int)(y * 32768) & (32768 - 1);
	UpdateTextureMatrix();
}

void Gfx_DisableTextureOffset(void) {
	texOffsetX = 0;
	texOffsetY = 0;
	UpdateTextureMatrix();
}

static int FindColorInPalette(cc_uint16* pal, int pal_size, cc_uint16 col) {
	if ((col >> 15) == 0) return 0;
	
	for (int i = 1; i < pal_size; i++) 
	{
		if (pal[i] == col) return i;
	}
	return -1;
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	cc_uint16* tmp = Mem_TryAlloc(bmp->width * bmp->height, 2);
	if (!tmp) return 0;

	// TODO: Only copy when rowWidth != bmp->width
	for (int y = 0; y < bmp->height; y++) {
		cc_uint16* src = bmp->scan0 + y * rowWidth;
		cc_uint16* dst = tmp		+ y * bmp->width;
		
		swiCopy(src, dst, bmp->width | COPY_MODE_HWORD);
	}
	
	// Palettize texture if possible
	int pal_size = 1;
	cc_uint16 tmp_palette[256];
	tmp_palette[0] = 0;
	
	for (int i = 0; i < bmp->width * bmp->height; i++) {
		cc_uint16 col = tmp[i];
	
		int idx = FindColorInPalette(tmp_palette, pal_size, col);
		
		if (idx == -1) {
			pal_size++;
			if (pal_size > 256) break;
			tmp_palette[pal_size - 1] = col;
		}
	}

	int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(0, textureID);
	char* tmp_chr = (char*)tmp;

	if (pal_size <= 4) {
		for (int i = 0; i < bmp->width * bmp->height; i++) 
		{
			cc_uint16 col = tmp[i];
			int idx = FindColorInPalette(tmp_palette, pal_size, col);
			
			if ((i & 3) == 0) {
				tmp_chr[i >> 2] = idx;
			} else {
				tmp_chr[i >> 2] |= idx << (2 * (i & 3));
			}
		}

		glTexImage2D(0, 0, GL_RGB4,  bmp->width, bmp->height, 0, 0, tmp);
		glColorTableEXT(0, 0, 4, 0, 0, tmp_palette);
	} else if (pal_size <= 16) {
		for (int i = 0; i < bmp->width * bmp->height; i++) 
		{
			cc_uint16 col = tmp[i];
			int idx = FindColorInPalette(tmp_palette, pal_size, col);
			
			if ((i & 1) == 0) {
				tmp_chr[i >> 1] = idx;
			} else {
				tmp_chr[i >> 1] |= idx << 4;
			}
		}

		glTexImage2D(0, 0, GL_RGB16,  bmp->width, bmp->height, 0, 0, tmp);
		glColorTableEXT(0, 0, 16, 0, 0, tmp_palette);
	} else if(pal_size <= 256) {
		for (int i = 0; i < bmp->width * bmp->height; i++) 
		{
			cc_uint16 col = tmp[i];			
			tmp_chr[i] = FindColorInPalette(tmp_palette, pal_size, col);
		}

		glTexImage2D(0, 0, GL_RGB256, bmp->width, bmp->height, 0, 0, tmp);
		glColorTableEXT(0, 0, 256, 0, 0, tmp_palette);
	} else {
		glTexImage2D(0, 0, GL_RGBA,   bmp->width, bmp->height, 0, 0, tmp);
	}
	
	glTexParameter(0, GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT);
	cc_uint16* vram_ptr = glGetTexturePointer(textureID);
	if (!vram_ptr) Platform_Log2("No VRAM for %i x %i texture", &bmp->width, &bmp->height);

	Mem_Free(tmp);
	return (void*)textureID;
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(0, (int)texId);
	UpdateTextureMatrix();
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	int texture = (int)texId;
	glBindTexture(0, texture);
	
	int width = 0;
	glGetInt(GL_GET_TEXTURE_WIDTH,  &width);
	cc_uint16* vram_ptr = glGetTexturePointer(texture);
	return;
	// TODO doesn't work without VRAM bank changing to LCD and back maybe??
	// (see what glTeximage2D does ??)
	
	for (int yy = 0; yy < part->height; yy++)
	{
		cc_uint16* dst = vram_ptr + width * (y + yy) + x;
		cc_uint16* src = part->scan0 + rowWidth * yy;
		
		for (int xx = 0; xx < part->width; xx++)
		{
			*dst++ = *src++;
		}
	}
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	int texture = (int)(*texId);
	if (texture) glDeleteTextures(1, &texture);
	*texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/

void Gfx_SetDepthWrite(cc_bool enabled) { }
void Gfx_SetDepthTest(cc_bool enabled)  { }

static void Gfx_FreeState(void) { FreeDefaultResources(); }
static void Gfx_RestoreState(void) {
	InitDefaultResources();
}

cc_bool Gfx_WarnIfNecessary(void) { return true; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;
	width  /= 64.0f; 
	height /= 64.0f; 

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
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
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
*----------------------------------------------------------Buffers--------------------------------------------------------*
*#########################################################################################################################*/
// Preprocess vertex buffers into optimised layout for DS
static VertexFormat buf_fmt;
static int buf_count;

static void* gfx_vertices;

struct DSTexturedVertex {
	vu32 command;
	vu32 rgb;
	vu32 uv;
	vu32 xy; vu32 z;
};

struct DSColouredVertex {
	vu32 command;
	vu32 rgb;
	vu32 xy; vu32 z;
};

// Precalculate all the expensive vertex data conversion,
//  so that actual drawing of them is as fast as possible
static void PreprocessTexturedVertices(void) {
	struct   VertexTextured* src = gfx_vertices;
	struct DSTexturedVertex* dst = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		struct VertexTextured v = *src;
		
		v16 x = floattov16(v.x / 64.0f);
		v16 y = floattov16(v.y / 64.0f);
		v16 z = floattov16(v.z / 64.0f);
	
		/*int uvX = (v.U * 256.0f + 0.5f); // 0.5f for rounding
		int uvY = (v.V * 256.0f + 0.5f);*/
		int uvX = ((int) (v.U * 1024.0f)) + 0x8000;
		int uvY = ((int) (v.V * 1024.0f)) + 0x8000;

		int r = PackedCol_R(v.Col);
		int g = PackedCol_G(v.Col);
		int b = PackedCol_B(v.Col);
		
		dst->command = FIFO_COMMAND_PACK(FIFO_NOP, FIFO_COLOR, FIFO_TEX_COORD, FIFO_VERTEX16);
		
		dst->rgb = ARGB16(1, r >> 3, g >> 3, b >> 3);
		
		dst->uv = TEXTURE_PACK(uvX, uvY);
		
		dst->xy = (y << 16) | (x & 0xFFFF);
		dst->z  = z;
	}
	
	DC_FlushRange(gfx_vertices, buf_count * sizeof(struct DSTexturedVertex));
}

static void PreprocessColouredVertices(void) {
	struct   VertexColoured* src = gfx_vertices;
	struct DSColouredVertex* dst = gfx_vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		struct VertexColoured v = *src;
		
		v16 x = floattov16(v.x / 64.0f);
		v16 y = floattov16(v.y / 64.0f);
		v16 z = floattov16(v.z / 64.0f);

		int r = PackedCol_R(v.Col);
		int g = PackedCol_G(v.Col);
		int b = PackedCol_B(v.Col);
		
		dst->command = FIFO_COMMAND_PACK(FIFO_NOP, FIFO_NOP, FIFO_COLOR, FIFO_VERTEX16);
		
		dst->rgb = ARGB16(1, r >> 3, g >> 3, b >> 3);
		
		dst->xy = (y << 16) | (x & 0xFFFF);
		dst->z  = z;
	}
	
	DC_FlushRange(gfx_vertices, buf_count * sizeof(struct DSColouredVertex));
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib)	{ }
void Gfx_DeleteIb(GfxResourceID* ib) { }


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
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static cc_bool skipRendering;
static cc_bool backfaceCull;

static cc_bool fogEnabled;
static FogFunc fogMode;
static float fogDensityEnd;

static void SetPolygonMode() {
	u32 fmt =
		POLY_ALPHA(31) | 
		(backfaceCull ? POLY_CULL_BACK : POLY_CULL_NONE) | 
		(fogEnabled ? POLY_FOG : 0) | 
		POLY_RENDER_FAR_POLYS | 
		POLY_RENDER_1DOT_POLYS;

	GFX_POLY_FORMAT = fmt;
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	backfaceCull = enabled;
	SetPolygonMode();
}

static void SetAlphaBlend(cc_bool enabled) {
	/*if (enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}*/
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

static void RecalculateFog() {
	if (fogMode == FOG_LINEAR) {
		int fogEnd = floattof32(fogDensityEnd);
		
		// Find shift value so that our fog end is
		//  inside maximum distance covered by fog table
		int shift = 10;
		while (shift > 0) {
			// why * 512? I dont know
			if (32 * (0x400 >> shift) * 512 >= fogEnd) break;
			shift--;
		}
		
		glFogShift(shift);
		GFX_FOG_OFFSET = 0;
		
		for (int i = 0; i < 32; i++) {
			int distance  = (i * 512 + 256) * (0x400 >> shift);
			int intensity = distance * 127 / fogEnd;
			if(intensity > 127) intensity = 127;
			
			GFX_FOG_TABLE[i] = intensity;
		}
		
		GFX_FOG_TABLE[31] = 127;
	} else {
		// TODO?
	}
}

void Gfx_SetFog(cc_bool enabled) {
	fogEnabled = enabled;
	SetPolygonMode();
}

void Gfx_SetFogCol(PackedCol color) {
	int r = PackedCol_R(color) >> 3;
	int g = PackedCol_G(color) >> 3;
	int b = PackedCol_B(color) >> 3;
	int a = 31;

	GFX_FOG_COLOR = RGB15(r, g, b) | (a << 16);
}

void Gfx_SetFogDensity(float value) {
	fogDensityEnd = value;
	RecalculateFog();
}

void Gfx_SetFogEnd(float value) {
	fogDensityEnd = value;
	RecalculateFog();
}

void Gfx_SetFogMode(FogFunc func) {
	fogMode = func;
	RecalculateFog();
}

static void SetAlphaTest(cc_bool enabled) {
	if (enabled) {
		//glEnable(GL_ALPHA_TEST);
	} else {
		//glDisable(GL_ALPHA_TEST);
	}
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	skipRendering = depthOnly;
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type != lastMatrix) { 
		lastMatrix	   = type; 
		MATRIX_CONTROL = matrix_modes[type]; 
	}
	
	// loads 4x4 identity matrix
	if (matrix == &Matrix_Identity) {
		MATRIX_IDENTITY = 0;
		return;
		// TODO still scale?
	}

	// loads 4x4 matrix from memory
	const float* src = (const float*)matrix;
	for (int i = 0; i < 4 * 4; i++)
	{
		MATRIX_LOAD4x4 = floattof32(src[i]);
	}

	// Vertex commands are signed 16 bit values, with 12 bits fractional
	//  aka only from -8.0 to 8.0
	// That's way too small to be useful, so counteract that by scaling down
	//  vertices and then scaling up the matrix multiplication
	if (type == MATRIX_VIEW) {
		MATRIX_SCALE = floattof32(64.0f); // X scale
		MATRIX_SCALE = floattof32(64.0f); // Y scale
		MATRIX_SCALE = floattof32(64.0f); // Z scale
	}
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}


/*########################################################################################################################*
*--------------------------------------------------------Rendering--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {
}

static void CallDrawList(void* list, u32 listSize) {
	// Based on libnds glCallList
	while (dmaBusy(0) || dmaBusy(1) || dmaBusy(2) || dmaBusy(3));
	dmaSetParams(0, list, (void*) &GFX_FIFO, DMA_FIFO | listSize);
	while (dmaBusy(0));
}

static void Draw_ColouredTriangles(int verticesCount, int startVertex) {
	glBindTexture(0, 0); // Disable texture
	GFX_BEGIN = GL_QUADS;
	CallDrawList(&((struct DSColouredVertex*) gfx_vertices)[startVertex], verticesCount * 4);
	GFX_END = 0;
}

static void Draw_TexturedTriangles(int verticesCount, int startVertex) {
	GFX_BEGIN = GL_QUADS;
	CallDrawList(&((struct DSTexturedVertex*) gfx_vertices)[startVertex], verticesCount * 5);
	GFX_END = 0;
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, startVertex);
	} else {
		Draw_ColouredTriangles(verticesCount, startVertex);
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gfx_format == VERTEX_FORMAT_TEXTURED) {
		Draw_TexturedTriangles(verticesCount, 0);
	} else {
		Draw_ColouredTriangles(verticesCount, 0);
	}
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (skipRendering) return;
	Draw_TexturedTriangles(verticesCount, startVertex);
}
#endif

