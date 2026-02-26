#define CC_DYNAMIC_VBS_ARE_STATIC
#include "../_GraphicsBase.h"
#include "../Errors.h"
#include "../Window.h"

#include <gx2/clear.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/draw.h>
#include <gx2/event.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/shaders.h>
#include <gx2/state.h>
#include <gx2/surface.h>
#include <gx2/swap.h>
#include <gx2/temp.h>
#include <gx2/utils.h>
#include <gx2r/draw.h>
#include <gx2r/mem.h>
#include <gx2r/buffer.h>
#include <whb/gfx.h>
#include <coreinit/memdefaultheap.h>
#include <malloc.h>

/*########################################################################################################################*
*------------------------------------------------------Fetch shaders------------------------------------------------------*
*#########################################################################################################################*/
#define VERTEX_OFFSET_POS     0
#define VERTEX_OFFSET_COLOR  12
#define VERTEX_OFFSET_COORDS 16

static const GX2AttribStream colour_attributes[] = {
	{ 0, 0, VERTEX_OFFSET_POS,   GX2_ATTRIB_FORMAT_FLOAT_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 
		0, GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1), GX2_ENDIAN_SWAP_DEFAULT },
	{ 1, 0, VERTEX_OFFSET_COLOR, GX2_ATTRIB_FORMAT_UNORM_8_8_8_8,  GX2_ATTRIB_INDEX_PER_VERTEX, 
		0, GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_W), GX2_ENDIAN_SWAP_DEFAULT }
};

static const GX2AttribStream texture_attributes[] = {
	{ 0, 0, VERTEX_OFFSET_POS,    GX2_ATTRIB_FORMAT_FLOAT_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 
		0, GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_1), GX2_ENDIAN_SWAP_DEFAULT },
	{ 1, 0, VERTEX_OFFSET_COLOR,  GX2_ATTRIB_FORMAT_UNORM_8_8_8_8,  GX2_ATTRIB_INDEX_PER_VERTEX, 
		0, GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_Z, GX2_SQ_SEL_W), GX2_ENDIAN_SWAP_DEFAULT },
	{ 2, 0, VERTEX_OFFSET_COORDS, GX2_ATTRIB_FORMAT_FLOAT_32_32,    GX2_ATTRIB_INDEX_PER_VERTEX, 
		0, GX2_SEL_MASK(GX2_SQ_SEL_X, GX2_SQ_SEL_Y, GX2_SQ_SEL_0, GX2_SQ_SEL_1), GX2_ENDIAN_SWAP_DEFAULT },
};
static GX2FetchShader colour_FS, texture_FS;

static void CompileFetchShader(GX2FetchShader* shader, const GX2AttribStream* attribs, int numAttribs) {
   uint32_t size = GX2CalcFetchShaderSizeEx(numAttribs,
                                            GX2_FETCH_SHADER_TESSELLATION_NONE,
                                            GX2_TESSELLATION_MODE_DISCRETE);
   void* program = memalign(GX2_SHADER_PROGRAM_ALIGNMENT, size);

   GX2InitFetchShaderEx(shader, program, numAttribs, attribs,
                        GX2_FETCH_SHADER_TESSELLATION_NONE,
                        GX2_TESSELLATION_MODE_DISCRETE);

   GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, program, size);
}

static void CompileFetchShaders(void) {
	CompileFetchShader(&colour_FS,  colour_attributes,  Array_Elems(colour_attributes));
	CompileFetchShader(&texture_FS, texture_attributes, Array_Elems(texture_attributes));
}


/*########################################################################################################################*
*---------------------------------------------------------Shaders---------------------------------------------------------*
*#########################################################################################################################*/
extern const uint8_t coloured_none_gsh[];
extern const uint8_t textured_none_gsh[];
extern const uint8_t textured_lin_gsh[];
extern const uint8_t textured_exp_gsh[];
extern const uint8_t offset_none_gsh[];
extern const uint8_t offset_lin_gsh[];
extern const uint8_t offset_exp_gsh[];

#define VS_UNI_OFFSET_MVP   0
#define VS_UNI_COUNT_MVP   16
#define VS_UNI_OFFSET_OFST 16
#define VS_UNI_COUNT_OFST   4

#define PS_UNI_OFFSET_LIN  0
#define PS_UNI_COUNT_LIN   4
#define PS_UNI_OFFSET_EXP  4
#define PS_UNI_COUNT_EXP   4

struct ShaderProgram {
	GX2VertexShader* vs;
	GX2PixelShader*  ps;
};

static struct ShaderProgram colour_PG, texture_PG[3], offset_PG[3];

static void LoadProgram(struct ShaderProgram* prog, const cc_uint8* gsh) {
	prog->vs = WHBGfxLoadGFDVertexShader(0, gsh);
	prog->ps = WHBGfxLoadGFDPixelShader(0,  gsh);
}

static GX2Sampler sampler;
static GfxResourceID white_square;
static struct ShaderProgram* cur_PG;
static int fog_func;

static void InitGfx(void) {
	CompileFetchShaders();
   	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_WRAP, GX2_TEX_XY_FILTER_MODE_POINT);
	
	LoadProgram(&colour_PG, coloured_none_gsh);

	LoadProgram(&texture_PG[0], textured_none_gsh);
	LoadProgram(&texture_PG[1], textured_lin_gsh);
	LoadProgram(&texture_PG[2], textured_exp_gsh);

	LoadProgram(&offset_PG[0], offset_none_gsh);
	LoadProgram(&offset_PG[1], offset_lin_gsh);
	LoadProgram(&offset_PG[2], offset_exp_gsh);
}

static struct Vec4 texOffset;

static int CalcPSIndex(void) {
	if (gfx_fogEnabled && fog_func == FOG_EXP)    return 2;
	if (gfx_fogEnabled && fog_func == FOG_LINEAR) return 1;

	return 0;
}

static void UpdateProgram(void) {
	struct ShaderProgram* prog;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		prog = &colour_PG;
	} else if (texOffset.x || texOffset.y) {
		prog = &offset_PG[CalcPSIndex()];
	} else {
		prog = &texture_PG[CalcPSIndex()];
	}

	cur_PG = prog;
	GX2SetPixelShader(prog->ps);
	GX2SetVertexShader(prog->vs);
}


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Create(void) {
	if (!Gfx.Created) InitGfx();
	
	Gfx.Created      = true;
	Gfx.MaxTexWidth  = 1024;
	Gfx.MaxTexHeight = 1024;
}

cc_bool Gfx_TryRestoreContext(void) {
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
}

static void Gfx_FreeState(void) { 
	FreeDefaultResources();
	Gfx_DeleteTexture(&white_square);
}

static void Gfx_RestoreState(void) {
	Gfx_SetFaceCulling(false);
	InitDefaultResources();
	gfx_format = -1;
	
	// 1x1 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static GX2Texture* pendingTex;

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	GX2Texture* tex = Mem_TryAllocCleared(1, sizeof(GX2Texture));
	if (!tex) return NULL;

	// TODO handle out of memory better
	int width = bmp->width, height = bmp->height;
	tex->surface.width    = width;
	tex->surface.height   = height;
	tex->surface.depth    = 1;
	tex->surface.dim      = GX2_SURFACE_DIM_TEXTURE_2D;
	tex->surface.format   = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
	tex->surface.tileMode = GX2_TILE_MODE_LINEAR_ALIGNED;
	tex->viewNumSlices    = 1;
	tex->compMap          = GX2_COMP_MAP(GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A);
	GX2CalcSurfaceSizeAndAlignment(&tex->surface);
	GX2InitTextureRegs(tex);

	tex->surface.image = MEMAllocFromDefaultHeapEx(tex->surface.imageSize, tex->surface.alignment);
	if (!tex->surface.image) { Mem_Free(tex); return NULL; }
  
	CopyPixels(tex->surface.image, tex->surface.pitch << 2, 
			   bmp->scan0,         rowWidth * BITMAPCOLOR_SIZE,
			   bmp->width,         bmp->height);
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, tex->surface.image, tex->surface.imageSize);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	GX2Texture* tex = (GX2Texture*)texId;	
	uint32_t* dst   = (uint32_t*)tex->surface.image + (y * tex->surface.pitch) + x;
	
	CopyPixels(dst,         tex->surface.pitch << 2, 
			   part->scan0, rowWidth * BITMAPCOLOR_SIZE,
			   part->width, part->height);
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, tex->surface.image, tex->surface.imageSize);
}

void Gfx_BindTexture(GfxResourceID texId) {
	if (!texId) texId = white_square;
	pendingTex = (GX2Texture*)texId;
	// Texture is bound to active shader, so might need to defer it in
	//  case a call to Gfx_BindTexture was called even though vertex format wasn't textured
	// TODO: Track as dirty uniform flag instead?
}

static void BindPendingTexture(void) {
	if (!pendingTex || cur_PG == &colour_PG) return;
	
	GX2SetPixelTexture(pendingTex, cur_PG->ps->samplerVars[0].location);
	GX2SetPixelSampler(&sampler,   cur_PG->ps->samplerVars[0].location);
 	pendingTex = NULL;
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GX2Texture* tex = (GX2Texture*)texId;
	if (tex == pendingTex) pendingTex = NULL;
	// TODO free memory ???
	*texId = NULL;
}

void Gfx_EnableMipmaps(void) { }  // TODO

void Gfx_DisableMipmaps(void) { } // TODO


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static float clearR, clearG, clearB;
static cc_bool depthWrite = true, depthTest = true;

static void UpdateDepthState(void) {
	GX2SetDepthOnlyControl(depthTest, depthWrite, GX2_COMPARE_FUNC_LEQUAL);
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, false, enabled);
}

void Gfx_SetFog(cc_bool enabled) {
	gfx_fogEnabled = enabled;
	UpdateProgram();
}

// xyz/rgb are colour, w/a is value
static struct Vec4 fogLin, fogExp;
static PackedCol fogColor;

void Gfx_SetFogCol(PackedCol color) {
	if (fogColor == color) return;
	fogColor = color;

	fogLin.x = fogExp.x = PackedCol_R(color) / 255.0f;
	fogLin.y = fogExp.y = PackedCol_G(color) / 255.0f;
	fogLin.z = fogExp.z = PackedCol_B(color) / 255.0f;

	GX2SetPixelUniformReg(PS_UNI_OFFSET_LIN, PS_UNI_COUNT_LIN, &fogLin);
	GX2SetPixelUniformReg(PS_UNI_OFFSET_EXP, PS_UNI_COUNT_EXP, &fogExp);
}

void Gfx_SetFogEnd(float value) {
	fogLin.w = 1.0f / value;
	GX2SetPixelUniformReg(PS_UNI_OFFSET_LIN, PS_UNI_COUNT_LIN, &fogLin);
}

#define LOG2_E 1.44269504089f
void Gfx_SetFogDensity(float value) {
	fogExp.w = -value * LOG2_E;
	GX2SetPixelUniformReg(PS_UNI_OFFSET_EXP, PS_UNI_COUNT_EXP, &fogExp);
}

void Gfx_SetFogMode(FogFunc func) {
	fog_func = func;
	UpdateProgram();
}

static void SetAlphaTest(cc_bool enabled) {
	GX2SetAlphaTest(enabled, GX2_COMPARE_FUNC_GEQUAL, 0.5f);
}

static void SetAlphaBlend(cc_bool enabled) {
	GX2SetBlendControl(GX2_RENDER_TARGET_0,
		GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
		true,
		GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
	GX2SetColorControl(GX2_LOGIC_OP_COPY, enabled, FALSE, TRUE);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
}

void Gfx_ClearColor(PackedCol color) {
	clearR = PackedCol_R(color) / 255.0f;
	clearG = PackedCol_G(color) / 255.0f;
	clearB = PackedCol_B(color) / 255.0f;
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depthTest = enabled;
	UpdateDepthState();
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depthWrite = enabled;
	UpdateDepthState();
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	GX2ChannelMask mask = 0;
	if (r) mask |= GX2_CHANNEL_MASK_R;
	if (g) mask |= GX2_CHANNEL_MASK_G;
	if (b) mask |= GX2_CHANNEL_MASK_B;
	if (a) mask |= GX2_CHANNEL_MASK_A;
	
	// TODO: use GX2SetColorControl to disable all writing ???
	GX2SetTargetChannelMasks(mask, 0,0,0, 0,0,0,0);
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
	return (void*)1; // don't need index buffers.. ?
}

void Gfx_BindIb(GfxResourceID ib) {
}

void Gfx_DeleteIb(GfxResourceID* ib) {
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	GX2RBuffer* buf = Mem_TryAllocCleared(1, sizeof(GX2RBuffer));
	if (!buf) return NULL;
	
	buf->flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_READ | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
	buf->elemSize  = strideSizes[fmt];
	buf->elemCount = count;
	
	// TODO calculate directly, avoid GX2R
	uint32_t align = GX2RGetBufferAlignment(buf->flags);
	uint32_t size  = GX2RGetBufferAllocationSize(buf);
		
	buf->buffer = MEMAllocFromDefaultHeapEx(size, align);
	if (buf->buffer) return buf;
	// Something went wrong ?? TODO
	Mem_Free(buf);
	return NULL;
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GX2RBuffer* buf = *vb;
	if (!buf) return;
	
	MEMFreeToDefaultHeap(buf->buffer);
	Mem_Free(buf);
	*vb = NULL;
}

void Gfx_BindVb(GfxResourceID vb) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	GX2SetAttribBuffer(0, buf->elemSize * buf->elemCount, buf->elemSize, buf->buffer);
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	return buf->buffer;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	GX2RBuffer* buf = (GX2RBuffer*)vb;
	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, buf->buffer, buf->elemSize * buf->elemCount);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex rendering----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
	
	GX2SetFetchShader(fmt == VERTEX_FORMAT_TEXTURED ? &texture_FS : &colour_FS);
	UpdateProgram();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_LINES, verticesCount, 0, 1);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, 0, 1);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	BindPendingTexture();
	GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, verticesCount, startVertex, 1);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj;
static struct Matrix _mvp CC_ALIGNED(64);

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;
	
	Matrix_Mul(&_mvp, &_view, &_proj);
	GX2SetVertexUniformReg(VS_UNI_OFFSET_MVP, VS_UNI_COUNT_MVP, &_mvp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	texOffset.x = x;
	texOffset.y = y;

	UpdateProgram();
	GX2SetVertexUniformReg(VS_UNI_OFFSET_OFST, VS_UNI_COUNT_OFST, &texOffset);
}

void Gfx_DisableTextureOffset(void) {
	texOffset.x = 0;
	texOffset.y = 0;
	UpdateProgram();
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// TODO verify this
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
	// TODO verify this
	float zNear = 0.1f;
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
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
	// TODO GX2SetSwapInterval(1);
}

void Gfx_BeginFrame(void) {
	WHBGfxBeginRender();
	WHBGfxBeginRenderTV();
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	WHBGfxClearColor(clearR, clearG, clearB, 1.0f);
}

static int drc_ticks;
static GfxResourceID drc_vb;
static void CreateDRCTest(void) {
	if (drc_vb) return;

	drc_vb = Gfx_CreateVb(VERTEX_FORMAT_COLOURED, 4);
	struct VertexColoured* data = (struct VertexColoured*)Gfx_LockVb(drc_vb, VERTEX_FORMAT_COLOURED, 4);

	data[0].x = -0.5f; data[0].y = -0.5f; data[0].z = 0.0f; data[0].Col = PACKEDCOL_WHITE;
	data[1].x =  0.5f; data[1].y = -0.5f; data[1].z = 0.0f; data[1].Col = PACKEDCOL_WHITE; 
	data[2].x =  0.5f; data[2].y =  0.5f; data[2].z = 0.0f; data[2].Col = PACKEDCOL_WHITE;
	data[3].x = -0.5f; data[3].y =  0.5f; data[3].z = 0.0f; data[3].Col = PACKEDCOL_WHITE;

	Gfx_UnlockVb(drc_vb);
}

void Gfx_EndFrame(void) {
	GX2ColorBuffer* buf = WHBGfxGetTVColourBuffer();
	GX2CopyColorBufferToScanBuffer(buf, GX2_SCAN_TARGET_TV);

	WHBGfxBeginRenderDRC();
	GX2CopyColorBufferToScanBuffer(buf, GX2_SCAN_TARGET_DRC);

	WHBGfxFinishRender();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Wii U --\n");
	PrintMaxTextureInfo(info);
}

void Gfx_OnWindowResize(void) {

}

void Gfx_SetViewport(int x, int y, int w, int h) {
	GX2SetViewport(x, y, w, h, 0.0f, 1.0f);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	GX2SetScissor( x, y, w, h);
}

void Gfx_3DS_SetRenderScreen1(enum Screen3DS screen) {
	GX2ContextState* tv_state  = WHBGfxGetTVContextState();
	GX2ContextState* drc_state = WHBGfxGetDRCContextState(); // TODO
	
	GX2SetContextState(screen == TOP_SCREEN ? tv_state : drc_state);
}
