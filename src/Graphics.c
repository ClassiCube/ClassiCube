#include "Graphics.h"
#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include "Funcs.h"
#include "Chat.h"
#include "Game.h"
#include "ExtMath.h"
#include "Event.h"
#include "Block.h"
#include "ExtMath.h"

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME

struct _GfxData Gfx;

static char Gfx_ApiBuffer[7][STRING_SIZE];
String Gfx_ApiInfo[7] = {
	String_FromArray(Gfx_ApiBuffer[0]), String_FromArray(Gfx_ApiBuffer[1]),
	String_FromArray(Gfx_ApiBuffer[2]), String_FromArray(Gfx_ApiBuffer[3]),
	String_FromArray(Gfx_ApiBuffer[4]), String_FromArray(Gfx_ApiBuffer[5]),
	String_FromArray(Gfx_ApiBuffer[6]),
};

GfxResourceID Gfx_defaultIb;
GfxResourceID Gfx_quadVb, Gfx_texVb;

const static int gfx_strideSizes[2] = { 16, 24 };
static int gfx_batchStride, gfx_batchFormat = -1;

static bool gfx_vsync, gfx_fogEnabled;
bool Gfx_GetFog(void) { return gfx_fogEnabled; }

/*########################################################################################################################*
*------------------------------------------------------Generic/Common-----------------------------------------------------*
*#########################################################################################################################*/
CC_NOINLINE static void Gfx_InitDefaultResources(void) {
	uint16_t indices[GFX_MAX_INDICES];
	Gfx_MakeIndices(indices, GFX_MAX_INDICES);
	Gfx_defaultIb = Gfx_CreateIb(indices, GFX_MAX_INDICES);

	Gfx_quadVb = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FC4B, 4);
	Gfx_texVb  = Gfx_CreateDynamicVb(VERTEX_FORMAT_P3FT2FC4B, 4);
}

CC_NOINLINE static void Gfx_FreeDefaultResources(void) {
	Gfx_DeleteVb(&Gfx_quadVb);
	Gfx_DeleteVb(&Gfx_texVb);
	Gfx_DeleteIb(&Gfx_defaultIb);
}

void Gfx_LoseContext(const char* reason) {
	Gfx.LostContext = true;
	Platform_Log1("Lost graphics context: %c", reason);

	Event_RaiseVoid(&GfxEvents.ContextLost);
	Gfx_FreeDefaultResources();
}

void Gfx_RecreateContext(void) {
	Gfx.LostContext = false;
	Platform_LogConst("Recreating graphics context");

	Event_RaiseVoid(&GfxEvents.ContextRecreated);
	Gfx_InitDefaultResources();
}


void Gfx_UpdateDynamicVb_Lines(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_Lines(vCount);
}

void Gfx_UpdateDynamicVb_IndexedTris(GfxResourceID vb, void* vertices, int vCount) {
	Gfx_SetDynamicVbData(vb, vertices, vCount);
	Gfx_DrawVb_IndexedTris(vCount);
}

void Gfx_Draw2DFlat(int x, int y, int width, int height, PackedCol col) {
	VertexP3fC4b verts[4];
	VertexP3fC4b v; v.Z = 0.0f; v.Col = col;

	v.X = (float)x;           v.Y = (float)y;            verts[0] = v;
	v.X = (float)(x + width);                            verts[1] = v;
	                          v.Y = (float)(y + height); verts[2] = v;
	v.X = (float)x;                                      verts[3] = v;

	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FC4B);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_quadVb, verts, 4);
}

void Gfx_Draw2DGradient(int x, int y, int width, int height, PackedCol top, PackedCol bottom) {
	VertexP3fC4b verts[4];
	VertexP3fC4b v; v.Z = 0.0f;

	v.X = (float)x;           v.Y = (float)y;            v.Col = top;    verts[0] = v;
	v.X = (float)(x + width);                                            verts[1] = v;
	                          v.Y = (float)(y + height); v.Col = bottom; verts[2] = v;
	v.X = (float)x;                                                      verts[3] = v;

	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FC4B);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_quadVb, verts, 4);
}

void Gfx_Draw2DTexture(const struct Texture* tex, PackedCol col) {
	VertexP3fT2fC4b texVerts[4];
	VertexP3fT2fC4b* ptr = texVerts;
	Gfx_Make2DQuad(tex, col, &ptr);
	Gfx_SetVertexFormat(VERTEX_FORMAT_P3FT2FC4B);
	Gfx_UpdateDynamicVb_IndexedTris(Gfx_texVb, texVerts, 4);
}

void Gfx_Make2DQuad(const struct Texture* tex, PackedCol col, VertexP3fT2fC4b** vertices) {
	float x1 = (float)tex->X, x2 = (float)(tex->X + tex->Width);
	float y1 = (float)tex->Y, y2 = (float)(tex->Y + tex->Height);
#ifdef CC_BUILD_D3D9
	/* NOTE: see "https://msdn.microsoft.com/en-us/library/windows/desktop/bb219690(v=vs.85).aspx", */
	/* i.e. the msdn article called "Directly Mapping Texels to Pixels (Direct3D 9)" for why we have to do this. */
	x1 -= 0.5f; x2 -= 0.5f;
	y1 -= 0.5f; y2 -= 0.5f;
#endif

	VertexP3fT2fC4b* ptr = *vertices;
	VertexP3fT2fC4b v; v.Z = 0.0f; v.Col = col;
	v.X = x1; v.Y = y1; v.U = tex->uv.U1; v.V = tex->uv.V1; ptr[0] = v;
	v.X = x2;           v.U = tex->uv.U2;                   ptr[1] = v;
	v.Y = y2;                             v.V = tex->uv.V2; ptr[2] = v;
	v.X = x1;           v.U = tex->uv.U1;                   ptr[3] = v;
	*vertices += 4;
}

static bool gfx_hadFog;
void Gfx_Mode2D(int width, int height) {
	struct Matrix ortho;
	Gfx_CalcOrthoMatrix((float)width, (float)height, &ortho);

	Gfx_LoadMatrix(MATRIX_PROJECTION, &ortho);
	Gfx_LoadIdentityMatrix(MATRIX_VIEW);

	Gfx_SetDepthTest(false);
	Gfx_SetAlphaBlending(true);
	gfx_hadFog = Gfx_GetFog();
	if (gfx_hadFog) Gfx_SetFog(false);
}

void Gfx_Mode3D(void) {
	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx.Projection);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx.View);

	Gfx_SetDepthTest(true);
	Gfx_SetAlphaBlending(false);
	if (gfx_hadFog) Gfx_SetFog(true);
}

void Gfx_MakeIndices(uint16_t* indices, int iCount) {
	int element = 0, i;

	for (i = 0; i < iCount; i += 6) {
		indices[0] = (uint16_t)(element + 0);
		indices[1] = (uint16_t)(element + 1);
		indices[2] = (uint16_t)(element + 2);

		indices[3] = (uint16_t)(element + 2);
		indices[4] = (uint16_t)(element + 3);
		indices[5] = (uint16_t)(element + 0);

		indices += 6; element += 4;
	}
}

void Gfx_SetupAlphaState(uint8_t draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(true);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(true);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(true);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(true);
}

void Gfx_RestoreAlphaState(uint8_t draw) {
	if (draw == DRAW_TRANSLUCENT)       Gfx_SetAlphaBlending(false);
	if (draw == DRAW_TRANSPARENT)       Gfx_SetAlphaTest(false);
	if (draw == DRAW_TRANSPARENT_THICK) Gfx_SetAlphaTest(false);
	if (draw == DRAW_SPRITE)            Gfx_SetAlphaTest(false);
}


/* Quoted from http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
   The short version: if you want your renderer to properly handle textures with alphas when using
   bilinear interpolation or mipmapping, you need to premultiply your PNG color data by their (unassociated) alphas. */
static BitmapCol Gfx_Average(BitmapCol p1, BitmapCol p2) {
	uint32_t a1, a2, aSum;
	uint32_t b1, g1, r1;
	uint32_t b2, g2, r2;
	BitmapCol ave;

	a1 = p1.A; a2 = p2.A;
	aSum = (a1 + a2);
	aSum = aSum > 0 ? aSum : 1; /* avoid divide by 0 below */

	/* Convert RGB to pre-multiplied form */
	b1 = p1.B * a1; g1 = p1.G * a1; r1 = p1.R * a1;
	b2 = p2.B * a2; g2 = p2.G * a2; r2 = p2.R * a2;

	/* https://stackoverflow.com/a/347376
	   We need to convert RGB back from the pre-multiplied average into normal form
	   ((r1 + r2) / 2) / ((a1 + a2) / 2)
	   but we just cancel out the / 2*/
	ave.B = (b1 + b2) / aSum;
	ave.G = (g1 + g2) / aSum;
	ave.R = (r1 + r2) / aSum;
	ave.A = aSum >> 1;
	return ave;
}

void Gfx_GenMipmaps(int width, int height, uint8_t* lvlScan0, uint8_t* scan0) {
	BitmapCol* baseSrc = (BitmapCol*)scan0;
	BitmapCol* baseDst = (BitmapCol*)lvlScan0;
	int srcWidth = width << 1;

	int x, y;
	for (y = 0; y < height; y++) {
		int srcY = (y << 1);
		BitmapCol* src0 = baseSrc + srcY * srcWidth;
		BitmapCol* src1 = src0    + srcWidth;
		BitmapCol* dst  = baseDst + y * width;

		for (x = 0; x < width; x++) {
			int srcX = (x << 1);
			BitmapCol src00 = src0[srcX], src01 = src0[srcX + 1];
			BitmapCol src10 = src1[srcX], src11 = src1[srcX + 1];

			/* bilinear filter this mipmap */
			BitmapCol ave0 = Gfx_Average(src00, src01);
			BitmapCol ave1 = Gfx_Average(src10, src11);
			dst[x] = Gfx_Average(ave0, ave1);
		}
	}
}

int Gfx_MipmapsLevels(int width, int height) {
	int lvlsWidth = Math_Log2(width), lvlsHeight = Math_Log2(height);
	if (Gfx.CustomMipmapsLevels) {
		int lvls = min(lvlsWidth, lvlsHeight);
		return min(lvls, 4);
	} else {
		return max(lvlsWidth, lvlsHeight);
	}
}

void Texture_Render(const struct Texture* tex) {
	PackedCol white = PACKEDCOL_WHITE;
	Gfx_BindTexture(tex->ID);
	Gfx_Draw2DTexture(tex, white);
}

void Texture_RenderShaded(const struct Texture* tex, PackedCol shadeCol) {
	Gfx_BindTexture(tex->ID);
	Gfx_Draw2DTexture(tex, shadeCol);
}


/*########################################################################################################################*
*--------------------------------------------------------Direct3D9--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_D3D9
/*#define D3D_DISABLE_9EX causes compile errors*/
#include <d3d9.h>
#include <d3d9caps.h>
#include <d3d9types.h>

static D3DCMPFUNC d3d9_compareFuncs[8] = { D3DCMP_ALWAYS, D3DCMP_NOTEQUAL, D3DCMP_NEVER, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER };
static DWORD d3d9_formatMappings[2] = { D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 };

static IDirect3D9* d3d;
static IDirect3DDevice9* device;
static DWORD createFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
static D3DFORMAT gfx_viewFormat, gfx_depthFormat;

#define D3D9_SetRenderState(state, value, name) \
ReturnCode res = IDirect3DDevice9_SetRenderState(device, state, value); if (res) Logger_Abort2(res, name);
#define D3D9_SetRenderState2(state, value, name) \
res = IDirect3DDevice9_SetRenderState(device, state, value);            if (res) Logger_Abort2(res, name);

static void D3D9_SetDefaultRenderStates(void);
static void D3D9_RestoreRenderStates(void);

static void D3D9_FreeResource(GfxResourceID* resource) {
	if (!resource || *resource == GFX_NULL) return;
	IUnknown* unk  = (IUnknown*)(*resource);
	ULONG refCount = unk->lpVtbl->Release(unk);
	*resource = GFX_NULL;
	if (refCount <= 0) return;

	uintptr_t addr = (uintptr_t)unk;
	Platform_Log2("D3D9 resource has %i outstanding references! ID 0x%x", &refCount, &addr);
}

static void D3D9_LoopUntilRetrieved(void) {
	struct ScheduledTask task;
	task.Interval = 1.0f / 60.0f;
	task.Callback = Gfx.LostContextFunction;

	while (true) {
		Thread_Sleep(16);
		ReturnCode code = IDirect3DDevice9_TestCooperativeLevel(device);
		if (code == D3DERR_DEVICENOTRESET) return;

		task.Callback(&task);
	}
}

static void D3D9_FindCompatibleFormat(void) {
	static D3DFORMAT depthFormats[6] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1 };
	static D3DFORMAT viewFormats[4]  = { D3DFMT_X8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 };
	ReturnCode res;
	int i, count = Array_Elems(viewFormats);

	for (i = 0; i < count; i++) {
		gfx_viewFormat = viewFormats[i];
		res = IDirect3D9_CheckDeviceType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, gfx_viewFormat, gfx_viewFormat, true);
		if (!res) break;
	}
	if (i == count) Logger_Abort("Unable to create a back buffer with sufficient precision.");

	count = Array_Elems(depthFormats);
	for (i = 0; i < count; i++) {
		gfx_depthFormat = depthFormats[i];
		res = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, gfx_viewFormat, gfx_viewFormat, gfx_depthFormat);
		if (!res) break;
	}
	if (i == count) Logger_Abort("Unable to create a depth buffer with sufficient precision.");
}

static void D3D9_FillPresentArgs(int width, int height, D3DPRESENT_PARAMETERS* args) {
	args->AutoDepthStencilFormat = gfx_depthFormat;
	args->BackBufferWidth  = width;
	args->BackBufferHeight = height;
	args->BackBufferFormat = gfx_viewFormat;
	args->BackBufferCount  = 1;
	args->EnableAutoDepthStencil = true;
	args->PresentationInterval   = gfx_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	args->SwapEffect = D3DSWAPEFFECT_DISCARD;
	args->Windowed   = true;
}

static void D3D9_RecreateDevice(void) {
	D3DPRESENT_PARAMETERS args = { 0 };
	D3D9_FillPresentArgs(Game.Width, Game.Height, &args);

	while (IDirect3DDevice9_Reset(device, &args) == D3DERR_DEVICELOST) {
		D3D9_LoopUntilRetrieved();
	}

	D3D9_SetDefaultRenderStates();
	D3D9_RestoreRenderStates();
	Gfx_RecreateContext();
}

void Gfx_Init(void) {
	Gfx.MinZNear = 0.05f;
	void* winHandle = Window_GetWindowHandle();
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3D9_FindCompatibleFormat();
	D3DPRESENT_PARAMETERS args = { 0 };
	D3D9_FillPresentArgs(640, 480, &args);
	ReturnCode res;

	/* Try to create a device with as much hardware usage as possible. */
	res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	if (res) {
		createFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}
	if (res) {
		createFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}
	if (res) Logger_Abort2(res, "Creating Direct3D9 device");

	D3DCAPS9 caps;
	res = IDirect3DDevice9_GetDeviceCaps(device, &caps);
	if (res) Logger_Abort2(res, "Getting Direct3D9 capabilities");

	Gfx.MaxTexWidth  = caps.MaxTextureWidth;
	Gfx.MaxTexHeight = caps.MaxTextureHeight;

	Gfx.CustomMipmapsLevels = true;
	D3D9_SetDefaultRenderStates();
	Gfx_InitDefaultResources();
}

void Gfx_Free(void) {
	Gfx_FreeDefaultResources();
	D3D9_FreeResource(&device);
	D3D9_FreeResource(&d3d);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void D3D9_SetTextureData(IDirect3DTexture9* texture, Bitmap* bmp, int lvl) {
	D3DLOCKED_RECT rect;
	ReturnCode res = IDirect3DTexture9_LockRect(texture, lvl, &rect, NULL, 0);
	if (res) Logger_Abort2(res, "D3D9_SetTextureData - Lock");

	uint32_t size = Bitmap_DataSize(bmp->Width, bmp->Height);
	Mem_Copy(rect.pBits, bmp->Scan0, size);

	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) Logger_Abort2(res, "D3D9_SetTextureData - Unlock");
}

static void D3D9_SetTexturePartData(IDirect3DTexture9* texture, int x, int y, Bitmap* bmp, int lvl) {
	RECT part;
	part.left = x; part.right = x + bmp->Width;
	part.top = y; part.bottom = y + bmp->Height;

	D3DLOCKED_RECT rect;
	ReturnCode res = IDirect3DTexture9_LockRect(texture, lvl, &rect, &part, 0);
	if (res) Logger_Abort2(res, "D3D9_SetTexturePartData - Lock");

	/* We need to copy scanline by scanline, as generally rect.stride != data.stride */
	uint8_t* src = (uint8_t*)bmp->Scan0;
	uint8_t* dst = (uint8_t*)rect.pBits;
	int yy;
	uint32_t stride = (uint32_t)bmp->Width * 4;

	for (yy = 0; yy < bmp->Height; yy++) {
		Mem_Copy(dst, src, stride);
		src += stride;
		dst += rect.Pitch;
	}

	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) Logger_Abort2(res, "D3D9_SetTexturePartData - Unlock");
}

static void D3D9_DoMipmaps(IDirect3DTexture9* texture, int x, int y, Bitmap* bmp, bool partial) {
	uint8_t* prev = bmp->Scan0;
	uint8_t* cur;
	Bitmap mipmap;

	int lvls = Gfx_MipmapsLevels(bmp->Width, bmp->Height);
	int lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)   width /= 2;
		if (height > 1) height /= 2;

		cur = Mem_Alloc(width * height, 4, "mipmaps");
		Gfx_GenMipmaps(width, height, cur, prev);

		Bitmap_Init(mipmap, width, height, cur);
		if (partial) {
			D3D9_SetTexturePartData(texture, x, y, &mipmap, lvl);
		} else {
			D3D9_SetTextureData(texture, &mipmap, lvl);
		}

		if (prev != bmp->Scan0) Mem_Free(prev);
		prev = cur;
	}
	if (prev != bmp->Scan0) Mem_Free(prev);
}

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps) {
	IDirect3DTexture9* tex;
	ReturnCode res;
	int mipmapsLevels = Gfx_MipmapsLevels(bmp->Width, bmp->Height);
	int levels = 1 + (mipmaps ? mipmapsLevels : 0);

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}

	if (managedPool) {
		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels,
			0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);
		if (res) Logger_Abort2(res, "D3D9_CreateTexture");

		D3D9_SetTextureData(tex, bmp, 0);
		if (mipmaps) D3D9_DoMipmaps(tex, 0, 0, bmp, false);
	} else {
		IDirect3DTexture9* sys;
		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels,
			0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, NULL);
		if (res) Logger_Abort2(res, "D3D9_CreateTexture - SystemMem");

		D3D9_SetTextureData(sys, bmp, 0);
		if (mipmaps) D3D9_DoMipmaps(sys, 0, 0, bmp, false);

		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels,
			0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
		if (res) Logger_Abort2(res, "D3D9_CreateTexture - GPU");

		res = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)sys, (IDirect3DBaseTexture9*)tex);
		if (res) Logger_Abort2(res, "D3D9_CreateTexture - Update");
		D3D9_FreeResource(&sys);
	}
	return tex;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps) {
	IDirect3DTexture9* texture = (IDirect3DTexture9*)texId;
	D3D9_SetTexturePartData(texture, x, y, part, 0);
	if (mipmaps) D3D9_DoMipmaps(texture, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	ReturnCode res = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9*)texId);
	if (res) Logger_Abort2(res, "D3D9_BindTexture");
}

void Gfx_DeleteTexture(GfxResourceID* texId) { D3D9_FreeResource(texId); }

void Gfx_SetTexturing(bool enabled) {
	if (enabled) return;
	ReturnCode res = IDirect3DDevice9_SetTexture(device, 0, NULL);
	if (res) Logger_Abort2(res, "D3D9_SetTexturing");
}

void Gfx_EnableMipmaps(void) {
	if (Gfx.Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		if (res) Logger_Abort2(res, "D3D9_EnableMipmaps");
	}
}

void Gfx_DisableMipmaps(void) {
	if (Gfx.Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		if (res) Logger_Abort2(res, "D3D9_DisableMipmaps");
	}
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedColUnion gfx_fogCol;
static float gfx_fogDensity = -1.0f, gfx_fogEnd = -1.0f;
static D3DFOGMODE gfx_fogMode = D3DFOG_NONE;

static bool gfx_alphaTesting, gfx_alphaBlending;
static int gfx_alphaTestRef;
static D3DCMPFUNC gfx_alphaTestFunc = D3DCMP_ALWAYS;
static D3DBLEND gfx_srcBlendFunc = D3DBLEND_ONE, gfx_dstBlendFunc = D3DBLEND_ZERO;

static PackedColUnion gfx_clearCol;
static bool gfx_depthTesting, gfx_depthWriting;
static D3DCMPFUNC gfx_depthTestFunc = D3DCMP_LESSEQUAL;

void Gfx_SetFaceCulling(bool enabled) {
	D3DCULL mode = enabled ? D3DCULL_CW : D3DCULL_NONE;
	D3D9_SetRenderState(D3DRS_CULLMODE, mode, "D3D9_SetFaceCulling");
}

void Gfx_SetFog(bool enabled) {
	if (gfx_fogEnabled == enabled) return;
	gfx_fogEnabled = enabled;

	if (Gfx.LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGENABLE, enabled, "D3D9_SetFog");
}

void Gfx_SetFogCol(PackedCol col) {
	if (PackedCol_Equals(col, gfx_fogCol.C)) return;
	gfx_fogCol.C = col;

	if (Gfx.LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGCOLOR, gfx_fogCol.Raw, "D3D9_SetFogColour");
}

void Gfx_SetFogDensity(float value) {
	union IntAndFloat raw;
	if (value == gfx_fogDensity) return;
	gfx_fogDensity = value;

	if (Gfx.LostContext) return;
	raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGDENSITY, raw.u, "D3D9_SetFogDensity");
}

void Gfx_SetFogEnd(float value) {
	union IntAndFloat raw;
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;

	if (Gfx.LostContext) return;
	raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGEND, raw.u, "D3D9_SetFogEnd");
}

void Gfx_SetFogMode(FogFunc func) {
	static D3DFOGMODE modes[3] = { D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2 };
	D3DFOGMODE mode = modes[func];
	if (mode == gfx_fogMode) return;

	gfx_fogMode = mode;
	if (Gfx.LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGTABLEMODE, mode, "D3D9_SetFogMode");
}

void Gfx_SetAlphaTest(bool enabled) {
	if (gfx_alphaTesting == enabled) return;
	gfx_alphaTesting = enabled;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE, enabled, "D3D9_SetAlphaTest");
}

void Gfx_SetAlphaTestFunc(CompareFunc func, float refValue) {
	gfx_alphaTestFunc = d3d9_compareFuncs[func];
	D3D9_SetRenderState(D3DRS_ALPHAFUNC, gfx_alphaTestFunc, "D3D9_SetAlphaTest_Func");
	gfx_alphaTestRef = (int)(refValue * 255);
	D3D9_SetRenderState2(D3DRS_ALPHAREF, gfx_alphaTestRef,  "D3D9_SetAlphaTest_Ref");
}

void Gfx_SetAlphaBlending(bool enabled) {
	if (gfx_alphaBlending == enabled) return;
	gfx_alphaBlending = enabled;
	D3D9_SetRenderState(D3DRS_ALPHABLENDENABLE, enabled, "D3D9_SetAlphaBlending");
}

void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
	static D3DBLEND funcs[6] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA };

	gfx_srcBlendFunc = funcs[srcFunc];
	D3D9_SetRenderState(D3DRS_SRCBLEND,   gfx_srcBlendFunc, "D3D9_SetAlphaBlendFunc_Src");
	gfx_dstBlendFunc = funcs[dstFunc];
	D3D9_SetRenderState2(D3DRS_DESTBLEND, gfx_dstBlendFunc, "D3D9_SetAlphaBlendFunc_Dst");
}

void Gfx_SetAlphaArgBlend(bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	ReturnCode res  = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
	if (res) Logger_Abort2(res, "D3D9_SetAlphaArgBlend");
}

void Gfx_ClearCol(PackedCol col) { gfx_clearCol.C = col; }
void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a) {
	DWORD channels = (r ? 1u : 0u) | (g ? 2u : 0u) | (b ? 4u : 0u) | (a ? 8u : 0u);
	D3D9_SetRenderState(D3DRS_COLORWRITEENABLE, channels, "D3D9_SetColourWrite");
}

void Gfx_SetDepthTest(bool enabled) {
	gfx_depthTesting = enabled;
	D3D9_SetRenderState(D3DRS_ZENABLE, enabled, "D3D9_SetDepthTest");
}

void Gfx_SetDepthTestFunc(CompareFunc func) {
	gfx_depthTestFunc = d3d9_compareFuncs[func];
	D3D9_SetRenderState(D3DRS_ZFUNC, gfx_alphaTestFunc, "D3D9_SetDepthTestFunc");
}

void Gfx_SetDepthWrite(bool enabled) {
	gfx_depthWriting = enabled;
	D3D9_SetRenderState(D3DRS_ZWRITEENABLE, enabled, "D3D9_SetDepthWrite");
}

static void D3D9_SetDefaultRenderStates(void) {
	Gfx_SetFaceCulling(false);
	gfx_batchFormat = -1;
	D3D9_SetRenderState(D3DRS_COLORVERTEX,        false, "D3D9_ColorVertex");
	D3D9_SetRenderState2(D3DRS_LIGHTING,          false, "D3D9_Lighting");
	D3D9_SetRenderState2(D3DRS_SPECULARENABLE,    false, "D3D9_SpecularEnable");
	D3D9_SetRenderState2(D3DRS_LOCALVIEWER,       false, "D3D9_LocalViewer");
	D3D9_SetRenderState2(D3DRS_DEBUGMONITORTOKEN, false, "D3D9_DebugMonitor");
}

static void D3D9_RestoreRenderStates(void) {
	union IntAndFloat raw;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE,   gfx_alphaTesting,  "D3D9_AlphaTest");
	D3D9_SetRenderState2(D3DRS_ALPHABLENDENABLE, gfx_alphaBlending, "D3D9_AlphaBlend");
	D3D9_SetRenderState2(D3DRS_ALPHAFUNC,        gfx_alphaTestFunc, "D3D9_AlphaTestFunc");
	D3D9_SetRenderState2(D3DRS_ALPHAREF,         gfx_alphaTestRef,  "D3D9_AlphaRefFunc");
	D3D9_SetRenderState2(D3DRS_SRCBLEND,         gfx_srcBlendFunc,  "D3D9_AlphaSrcBlend");
	D3D9_SetRenderState2(D3DRS_DESTBLEND,        gfx_dstBlendFunc,  "D3D9_AlphaDstBlend");

	D3D9_SetRenderState2(D3DRS_FOGENABLE, gfx_fogEnabled,  "D3D9_Fog");
	D3D9_SetRenderState2(D3DRS_FOGCOLOR,  gfx_fogCol.Raw, "gfx_fogColor");
	raw.f = gfx_fogDensity;
	D3D9_SetRenderState2(D3DRS_FOGDENSITY, raw.u,          "gfx_fogDensity");
	raw.f = gfx_fogEnd;
	D3D9_SetRenderState2(D3DRS_FOGEND, raw.u,              "gfx_fogEnd");
	D3D9_SetRenderState2(D3DRS_FOGTABLEMODE, gfx_fogMode, "D3D9_FogMode");

	D3D9_SetRenderState2(D3DRS_ZFUNC,        gfx_depthTestFunc, "D3D9_DepthTestFunc");
	D3D9_SetRenderState2(D3DRS_ZENABLE,      gfx_depthTesting,  "D3D9_DepthTest");
	D3D9_SetRenderState2(D3DRS_ZWRITEENABLE, gfx_depthWriting,  "D3D9_DepthWrite");
}


/*########################################################################################################################*
*---------------------------------------------------Vertex/Index buffers--------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	int size = maxVertices * gfx_strideSizes[fmt];
	IDirect3DVertexBuffer9* vbuffer;
	ReturnCode res = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		d3d9_formatMappings[fmt], D3DPOOL_DEFAULT, &vbuffer, NULL);
	if (res) Logger_Abort2(res, "D3D9_CreateDynamicVb");

	return vbuffer;
}

static void D3D9_SetVbData(IDirect3DVertexBuffer9* buffer, void* data, int size, const char* lockMsg, const char* unlockMsg, int lockFlags) {
	void* dst = NULL;
	ReturnCode res = IDirect3DVertexBuffer9_Lock(buffer, 0, size, &dst, lockFlags);
	if (res) Logger_Abort2(res, lockMsg);

	Mem_Copy(dst, data, size);
	res = IDirect3DVertexBuffer9_Unlock(buffer);
	if (res) Logger_Abort2(res, unlockMsg);
}

GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count) {
	int size = count * gfx_strideSizes[fmt];
	IDirect3DVertexBuffer9* vbuffer;
	ReturnCode res;

	for (;;) {
		res = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_WRITEONLY,
			d3d9_formatMappings[fmt], D3DPOOL_DEFAULT, &vbuffer, NULL);
		if (!res) break;

		if (res != D3DERR_OUTOFVIDEOMEMORY) Logger_Abort2(res, "D3D9_CreateVb");
		Event_RaiseVoid(&GfxEvents.LowVRAMDetected);
	}

	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_CreateVb - Lock", "D3D9_CreateVb - Unlock", 0);
	return vbuffer;
}

static void D3D9_SetIbData(IDirect3DIndexBuffer9* buffer, void* data, int size) {
	void* dst = NULL;
	ReturnCode res = IDirect3DIndexBuffer9_Lock(buffer, 0, size, &dst, 0);
	if (res) Logger_Abort2(res, "D3D9_CreateIb - Lock");

	Mem_Copy(dst, data, size);
	res = IDirect3DIndexBuffer9_Unlock(buffer);
	if (res) Logger_Abort2(res, "D3D9_CreateIb - Unlock");
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
	int size = indicesCount * 2;
	IDirect3DIndexBuffer9* ibuffer;
	ReturnCode res = IDirect3DDevice9_CreateIndexBuffer(device, size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibuffer, NULL);
	if (res) Logger_Abort2(res, "D3D9_CreateIb");

	D3D9_SetIbData(ibuffer, indices, size);
	return ibuffer;
}

void Gfx_BindVb(GfxResourceID vb) {
	IDirect3DVertexBuffer9* vbuffer = (IDirect3DVertexBuffer9*)vb;
	ReturnCode res = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, gfx_batchStride);
	if (res) Logger_Abort2(res, "D3D9_BindVb");
}

void Gfx_BindIb(GfxResourceID ib) {
	IDirect3DIndexBuffer9* ibuffer = (IDirect3DIndexBuffer9*)ib;
	ReturnCode res = IDirect3DDevice9_SetIndices(device, ibuffer);
	if (res) Logger_Abort2(res, "D3D9_BindIb");
}

void Gfx_DeleteVb(GfxResourceID* vb) { D3D9_FreeResource(vb); }
void Gfx_DeleteIb(GfxResourceID* ib) { D3D9_FreeResource(ib); }

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_batchFormat) return;
	gfx_batchFormat = fmt;

	ReturnCode res = IDirect3DDevice9_SetFVF(device, d3d9_formatMappings[fmt]);
	if (res) Logger_Abort2(res, "D3D9_SetBatchFormat");
	gfx_batchStride = gfx_strideSizes[fmt];
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	int size = vCount * gfx_batchStride;
	IDirect3DVertexBuffer9* vbuffer = (IDirect3DVertexBuffer9*)vb;
	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_SetDynamicVbData - Lock", "D3D9_SetDynamicVbData - Unlock", D3DLOCK_DISCARD);

	ReturnCode res = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, gfx_batchStride);
	if (res) Logger_Abort2(res, "D3D9_SetDynamicVbData - Bind");
}

void Gfx_DrawVb_Lines(int verticesCount) {
	ReturnCode res = IDirect3DDevice9_DrawPrimitive(device, D3DPT_LINELIST, 0, verticesCount >> 1);
	if (res) Logger_Abort2(res, "D3D9_DrawVb_Lines");
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		0, 0, verticesCount, 0, verticesCount >> 1);
	if (res) Logger_Abort2(res, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	if (res) Logger_Abort2(res, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	if (res) Logger_Abort2(res, "D3D9_DrawIndexedVb_TrisT2fC4b");
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static D3DTRANSFORMSTATETYPE matrix_modes[3] = { D3DTS_PROJECTION, D3DTS_VIEW, D3DTS_TEXTURE0 };

void Gfx_LoadMatrix(MatrixType type, struct Matrix* matrix) {
	ReturnCode res;
	if (type == MATRIX_TEXTURE) {
		matrix->Row2.X = matrix->Row3.X; /* NOTE: this hack fixes the texture movements. */
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	}

	if (Gfx.LostContext) return;
	res = IDirect3DDevice9_SetTransform(device, matrix_modes[type], matrix);
	if (res) Logger_Abort2(res, "D3D9_LoadMatrix");
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	ReturnCode res;
	if (type == MATRIX_TEXTURE) {
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	}

	if (Gfx.LostContext) return;
	res = IDirect3DDevice9_SetTransform(device, matrix_modes[type], &Matrix_Identity);
	if (res) Logger_Abort2(res, "D3D9_LoadIdentityMatrix");
}

#define d3d9_zN -10000.0f
#define d3d9_zF  10000.0f
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_OrthographicOffCenter(matrix, 0.0f, width, height, 0.0f, d3d9_zN, d3d9_zF);
	matrix->Row2.Z = 1.0f    / (d3d9_zN - d3d9_zF);
	matrix->Row3.Z = d3d9_zN / (d3d9_zN - d3d9_zF);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix) {
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
ReturnCode Gfx_TakeScreenshot(struct Stream* output) {
	IDirect3DSurface9* backbuffer = NULL;
	IDirect3DSurface9* temp = NULL;
	D3DSURFACE_DESC desc;
	Bitmap bmp;
	ReturnCode res;

	res = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	if (res) goto finished;
	res = IDirect3DSurface9_GetDesc(backbuffer, &desc);
	if (res) goto finished;

	res = IDirect3DDevice9_CreateOffscreenPlainSurface(device, desc.Width, desc.Height, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &temp, NULL);
	if (res) goto finished; /* TODO: For DX 8 use IDirect3DDevice8::CreateImageSurface */
	res = IDirect3DDevice9_GetRenderTargetData(device, backbuffer, temp);
	if (res) goto finished;

	D3DLOCKED_RECT rect;
	res = IDirect3DSurface9_LockRect(temp, &rect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
	if (res) goto finished;
	{
		Bitmap_Init(bmp, desc.Width, desc.Height, rect.pBits);
		res = Png_Encode(&bmp, output, NULL, false);
		if (res) { IDirect3DSurface9_UnlockRect(temp); goto finished; }
	}
	res = IDirect3DSurface9_UnlockRect(temp);
	if (res) goto finished;

finished:
	D3D9_FreeResource(&backbuffer);
	D3D9_FreeResource(&temp);
	return res;
}

void Gfx_SetVSync(bool value) {
	if (gfx_vsync == value) return;
	gfx_vsync = value;

	Gfx_LoseContext(" (toggling VSync)");
	D3D9_RecreateDevice();
}

void Gfx_BeginFrame(void) { IDirect3DDevice9_BeginScene(device); }
void Gfx_Clear(void) {
	DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	ReturnCode res = IDirect3DDevice9_Clear(device, 0, NULL, flags, gfx_clearCol.Raw, 1.0f, 0);
	if (res) Logger_Abort2(res, "D3D9_Clear");
}

void Gfx_EndFrame(void) {
	IDirect3DDevice9_EndScene(device);
	ReturnCode res = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
	if (!res) return;
	if (res != D3DERR_DEVICELOST) Logger_Abort2(res, "D3D9_EndFrame");

	/* TODO: Make sure this actually works on all graphics cards.*/
	Gfx_LoseContext(" (Direct3D9 device lost)");
	D3D9_LoopUntilRetrieved();
	D3D9_RecreateDevice();
}

bool Gfx_WarnIfNecessary(void) { return false; }
static const char* D3D9_StrFlags(void) {
	if (createFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) return "Hardware";
	if (createFlags & D3DCREATE_MIXED_VERTEXPROCESSING)    return "Mixed";
	if (createFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) return "Software";
	return "(none)";
}

static const int D3D9_DepthBufferBts(D3DFORMAT format) {
	switch (format) {
	case D3DFMT_D32:     return 32;
	case D3DFMT_D24X8:   return 24;
	case D3DFMT_D24S8:   return 24;
	case D3DFMT_D24X4S4: return 24;
	case D3DFMT_D16:     return 16;
	case D3DFMT_D15S1:   return 15;
	}
	return 0;
}

static float gfx_totalMem;
void Gfx_MakeApiInfo(void) {
	D3DADAPTER_IDENTIFIER9 adapter = { 0 };
	int pointerSize = sizeof(void*) * 8;
	int depthBits   = D3D9_DepthBufferBts(gfx_depthFormat);

	IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &adapter);
	gfx_totalMem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);

	String_Format1(&Gfx_ApiInfo[0], "-- Using Direct3D9 (%i bit) --", &pointerSize);
	String_Format1(&Gfx_ApiInfo[1], "Adapter: %c",         adapter.Description);
	String_Format1(&Gfx_ApiInfo[2], "Processing mode: %c", D3D9_StrFlags());
	Gfx_UpdateApiInfo();
	String_Format2(&Gfx_ApiInfo[4], "Max texture size: (%i, %i)", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	String_Format1(&Gfx_ApiInfo[5], "Depth buffer bits: %i", &depthBits);
}

void Gfx_UpdateApiInfo(void) {
	float mem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);
	Gfx_ApiInfo[3].length = 0;
	String_Format2(&Gfx_ApiInfo[3], "Video memory: %f2 MB total, %f2 free", &gfx_totalMem, &mem);
}

void Gfx_OnWindowResize(void) {
	Gfx_LoseContext(" (resizing window)");
	D3D9_RecreateDevice();
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------OpenGL---------------------------------------------------------*
*#########################################################################################################################*/
/* The OpenGL backend is a bit verbose, since it's really 3 backends in one:
 * - OpenGL 1.1 (completely lacking GPU, fallbacks to say Windows built-in software rasteriser)
 * - OpenGL 1.5 or OpenGL 1.2 + GL_ARB_vertex_buffer_object (default desktop backend)
 * - OpenGL 2.0 (alternative modern-ish backend)
*/
#ifndef CC_BUILD_D3D9
#if defined CC_BUILD_WIN
#include <windows.h>
#include <GL/gl.h>
#elif defined CC_BUILD_OSX
#include <OpenGL/gl.h>
#elif defined CC_BUILD_GLMODERN
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif

#if defined CC_BUILD_GL11
static GLuint gfx_activeList;
#define gl_DYNAMICLISTID 1234567891
static void* gfx_dynamicListData;
static uint16_t gl_indices[GFX_MAX_INDICES];
#elif defined CC_BUILD_GLMODERN
#define _glBindBuffer(t,b)        glBindBuffer(t,b)
#define _glDeleteBuffers(n,b)     glDeleteBuffers(n,b)
#define _glGenBuffers(n,b)        glGenBuffers(n,b)
#define _glBufferData(t,s,d,u)    glBufferData(t,s,d,u)
#define _glBufferSubData(t,o,s,d) glBufferSubData(t,o,s,d)
#else
/* Not present in some gl.h which only have up to OpenGL 1.1 */
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8

typedef void (APIENTRY *FUNC_GLBINDBUFFER) (GLenum target, GLuint buffer);
typedef void (APIENTRY *FUNC_GLDELETEBUFFERS) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *FUNC_GLGENBUFFERS) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY *FUNC_GLBUFFERDATA) (GLenum target, uintptr_t size, const GLvoid* data, GLenum usage);
typedef void (APIENTRY *FUNC_GLBUFFERSUBDATA) (GLenum target, uintptr_t offset, uintptr_t size, const GLvoid* data);
static FUNC_GLBINDBUFFER    _glBindBuffer;
static FUNC_GLDELETEBUFFERS _glDeleteBuffers;
static FUNC_GLGENBUFFERS    _glGenBuffers;
static FUNC_GLBUFFERDATA    _glBufferData;
static FUNC_GLBUFFERSUBDATA _glBufferSubData;
#endif

#define GL_TEXTURE_MAX_LEVEL 0x813D
#ifndef CC_BUILD_WEB
#define PIXEL_FORMAT 0x80E1 /* GL_BGRA_EXT */
#else
#define PIXEL_FORMAT GL_RGBA
#endif
static int gl_compare[8] = { GL_ALWAYS, GL_NOTEQUAL, GL_NEVER, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER };

typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;

static void GL_CheckSupport(void);
static void GL_InitState(void);
void Gfx_Init(void) {
	struct GraphicsMode mode;
	GraphicsMode_MakeDefault(&mode);
	GLContext_Init(&mode);

	Gfx.MinZNear = 0.1f;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx.MaxTexWidth);
	Gfx.MaxTexHeight = Gfx.MaxTexWidth;

	GL_CheckSupport();
	GL_InitState();
	Gfx_InitDefaultResources();
}

void Gfx_Free(void) {
	Gfx_FreeDefaultResources();
	GLContext_Free();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_DoMipmaps(int x, int y, Bitmap* bmp, bool partial) {
	uint8_t* prev = bmp->Scan0;
	uint8_t* cur;

	int lvls = Gfx_MipmapsLevels(bmp->Width, bmp->Height);
	int lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)  width /= 2;
		if (height > 1) height /= 2;

		cur = Mem_Alloc(width * height, 4, "mipmaps");
		Gfx_GenMipmaps(width, height, cur, prev);

		if (partial) {
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, cur);
		}

		if (prev != bmp->Scan0) Mem_Free(prev);
		prev = cur;
	}
	if (prev != bmp->Scan0) Mem_Free(prev);
}

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps) {
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (Gfx.CustomMipmapsLevels) {
			int lvls = Gfx_MipmapsLevels(bmp->Width, bmp->Height);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->Width, bmp->Height, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, bmp->Scan0);

	if (mipmaps) Gfx_DoMipmaps(0, 0, bmp, false);
	return texId;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->Width, part->Height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, part->Scan0);
	if (mipmaps) Gfx_DoMipmaps(x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	if (!texId || *texId == GFX_NULL) return;
	GLuint id = (GLuint)(*texId);
	glDeleteTextures(1, &id);
	*texId = GFX_NULL;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearCol, gfx_fogCol;
static float gfx_fogEnd = -1, gfx_fogDensity = -1;
static int gfx_fogMode  = -1;

void Gfx_SetFaceCulling(bool enabled) { gl_Toggle(GL_CULL_FACE); }

void Gfx_SetAlphaBlending(bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
	static GLenum funcs[6] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
	glBlendFunc(funcs[srcFunc], funcs[dstFunc]);
}
void Gfx_SetAlphaArgBlend(bool enabled) { }

void Gfx_ClearCol(PackedCol col) {
	if (PackedCol_Equals(col, gfx_clearCol)) return;
	glClearColor(col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f);
	gfx_clearCol = col;
}

void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a) {
	glColorMask(r, g, b, a);
}

void Gfx_SetDepthWrite(bool enabled) {
	glDepthMask(enabled);
}

void Gfx_SetDepthTest(bool enabled) { gl_Toggle(GL_DEPTH_TEST); }
void Gfx_SetDepthTestFunc(CompareFunc func) {
	glDepthFunc(gl_compare[func]);
}

void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_OrthographicOffCenter(matrix, 0.0f, width, height, 0.0f, -10000.0f, 10000.0f);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix) {
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


/*########################################################################################################################*
*---------------------------------------------------Vertex/Index buffers--------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
static GLuint GL_GenAndBind(GLenum target) {
	GLuint id;
	_glGenBuffers(1, &id);
	_glBindBuffer(target, id);
	return id;
}

GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	GLuint id     = GL_GenAndBind(GL_ARRAY_BUFFER);
	uint32_t size = maxVertices * gfx_strideSizes[fmt];
	_glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	return id;
}

GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count) {
	GLuint id     = GL_GenAndBind(GL_ARRAY_BUFFER);
	uint32_t size = count * gfx_strideSizes[fmt];
	_glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	return id;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
	GLuint id     = GL_GenAndBind(GL_ELEMENT_ARRAY_BUFFER);
	uint32_t size = indicesCount * 2;
	_glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	return id;
}

void Gfx_BindVb(GfxResourceID vb) { _glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb); }
void Gfx_BindIb(GfxResourceID ib) { _glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)ib); }

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (!vb || *vb == GFX_NULL) return;
	GLuint id = (GLuint)(*vb);
	_glDeleteBuffers(1, &id);
	*vb = GFX_NULL;
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	if (!ib || *ib == GFX_NULL) return;
	GLuint id = (GLuint)(*ib);
	_glDeleteBuffers(1, &id);
	*ib = GFX_NULL;
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	uint32_t size = vCount * gfx_batchStride;
	_glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb);
	_glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}
#endif


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static int GL_SelectRow(Bitmap* bmp, int y) { return (bmp->Height - 1) - y; }
ReturnCode Gfx_TakeScreenshot(struct Stream* output) {
	Bitmap bmp;
	ReturnCode res;
	GLint vp[4];
	
	glGetIntegerv(GL_VIEWPORT, vp); /* { x, y, width, height } */
	Bitmap_Allocate(&bmp, vp[2], vp[3]);
	glReadPixels(0, 0, vp[2], vp[3], PIXEL_FORMAT, GL_UNSIGNED_BYTE, bmp.Scan0);

	res = Png_Encode(&bmp, output, GL_SelectRow, false);
	Mem_Free(bmp.Scan0);
	return res;
}

static bool nv_mem;
void Gfx_MakeApiInfo(void) {
	const static String memExt = String_FromConst("GL_NVX_gpu_memory_info");
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	int depthBits, pointerSize = sizeof(void*) * 8;

	nv_mem = String_CaselessContains(&extensions, &memExt);
	glGetIntegerv(GL_DEPTH_BITS, &depthBits);

	String_Format1(&Gfx_ApiInfo[0], "-- Using OpenGL (%i bit) --", &pointerSize);
	String_Format1(&Gfx_ApiInfo[1], "Vendor: %c",     glGetString(GL_VENDOR));
	String_Format1(&Gfx_ApiInfo[2], "Renderer: %c",   glGetString(GL_RENDERER));
	String_Format1(&Gfx_ApiInfo[3], "GL version: %c", glGetString(GL_VERSION));
	/* Memory usage line goes here */
	String_Format2(&Gfx_ApiInfo[5], "Max texture size: (%i, %i)", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	String_Format1(&Gfx_ApiInfo[6], "Depth buffer bits: %i", &depthBits);
}

void Gfx_UpdateApiInfo(void) {
	int totalKb = 0, curKb = 0;
	float total, cur;

	if (!nv_mem) return;
	glGetIntegerv(0x9048, &totalKb);
	glGetIntegerv(0x9049, &curKb);
	if (totalKb <= 0 || curKb <= 0) return;

	total = totalKb / 1024.0f; cur = curKb / 1024.0f;
	Gfx_ApiInfo[4].length = 0;
	String_Format2(&Gfx_ApiInfo[4], "Video memory: %f2 MB total, %f2 free", &total, &cur);
}

bool Gfx_WarnIfNecessary(void) {
	const static String intel = String_FromConst("Intel");
	String renderer = String_FromReadonly(glGetString(GL_RENDERER));

#ifdef CC_BUILD_GL11
	Chat_AddRaw("&cYou are using the very outdated OpenGL backend.");
	Chat_AddRaw("&cAs such you may experience poor performance.");
	Chat_AddRaw("&cIt is likely you need to install video card drivers.");
#endif
	if (!String_ContainsString(&renderer, &intel)) return false;

	Chat_AddRaw("&cIntel graphics cards are known to have issues with the OpenGL build.");
	Chat_AddRaw("&cVSync may not work, and you may see disappearing clouds and map edges.");
	Chat_AddRaw("&cFor Windows, try downloading the Direct3D 9 build instead.");
	return true;
}

void Gfx_SetVSync(bool value) {
	if (gfx_vsync == value) return;
	gfx_vsync = value;
	GLContext_SetVSync(value);
}

void Gfx_BeginFrame(void) { }
void Gfx_Clear(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Gfx_EndFrame(void) {
	GLContext_SwapBuffers();
}

void Gfx_OnWindowResize(void) {
	glViewport(0, 0, Game.Width, Game.Height);
	GLContext_Update();
}

/*########################################################################################################################*
*------------------------------------------------------OpenGL modern------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_GLMODERN
#define FTR_TEXTURE_UV (1 << 0)
#define FTR_ALPHA_TEST (1 << 1)
#define FTR_TEX_MATRIX (1 << 2)
#define FTR_LINEAR_FOG (1 << 3)
#define FTR_DENSIT_FOG (1 << 4)
#define FTR_HASANY_FOG (FTR_LINEAR_FOG | FTR_DENSIT_FOG)

#define UNI_MVP_MATRIX (1 << 0)
#define UNI_TEX_MATRIX (1 << 1)
#define UNI_FOG_COL    (1 << 2)
#define UNI_FOG_END    (1 << 3)
#define UNI_FOG_DENS   (1 << 4)

/* cached uniforms (cached for multiple programs */
static struct Matrix _view, _proj, _tex, _mvp;
static bool gfx_alphaTest, gfx_texTransform;

/* shader programs (emulate fixed function) */
static struct GLShader {
	int Features;     /* what features are enabled for this shader */
	int Uniforms;     /* which associated uniforms need to be resent to GPU */
	GLuint Program;   /* OpenGL program ID (0 if not yet compiled) */
	int Locations[5]; /* location of uniforms (not constant) */
} shaders[6 * 3] = {
	/* no fog */
	{ 0              },
	{ 0              | FTR_ALPHA_TEST },
	{ FTR_TEXTURE_UV },
	{ FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_TEXTURE_UV | FTR_TEX_MATRIX },
	{ FTR_TEXTURE_UV | FTR_TEX_MATRIX | FTR_ALPHA_TEST },
	/* linear fog */
	{ FTR_LINEAR_FOG | 0              },
	{ FTR_LINEAR_FOG | 0              | FTR_ALPHA_TEST },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_TEX_MATRIX },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_TEX_MATRIX | FTR_ALPHA_TEST },
	/* density fog */
	{ FTR_DENSIT_FOG | 0              },
	{ FTR_DENSIT_FOG | 0              | FTR_ALPHA_TEST },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_TEX_MATRIX },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_TEX_MATRIX | FTR_ALPHA_TEST },
};
static struct GLShader* gfx_activeShader;

/* Generates source code for a GLSL vertex shader, based on shader's flags */
static void Gfx_GenVertexShader(const struct GLShader* shader, String* dst) {
	int uv = shader->Features & FTR_TEXTURE_UV;
	int tm = shader->Features & FTR_TEX_MATRIX;

	String_AppendConst(dst,         "attribute vec3 in_pos;\n");
	String_AppendConst(dst,         "attribute vec4 in_col;\n");
	if (uv) String_AppendConst(dst, "attribute vec2 in_uv;\n");
	String_AppendConst(dst,         "varying vec4 out_col;\n");
	if (uv) String_AppendConst(dst, "varying vec2 out_uv;\n");
	String_AppendConst(dst,         "uniform mat4 mvp;\n");
	if (tm) String_AppendConst(dst, "uniform mat4 texMatrix;\n");

	String_AppendConst(dst,         "void main() {\n");
	String_AppendConst(dst,         "  gl_Position = mvp * vec4(in_pos, 1.0);\n");
	String_AppendConst(dst,         "  out_col = in_col;\n");
	if (uv) String_AppendConst(dst, "  out_uv  = in_uv;\n");
	/* TODO: Fix this dirty hack for clouds */
	if (tm) String_AppendConst(dst, "  out_uv = (texMatrix * vec4(out_uv,0.0,1.0)).xy;\n");
	String_AppendConst(dst,         "}");
}

/* Generates source code for a GLSL fragment shader, based on shader's flags */
static void Gfx_GenFragmentShader(const struct GLShader* shader, String* dst) {
	int uv = shader->Features & FTR_TEXTURE_UV;
	int al = shader->Features & FTR_ALPHA_TEST;
	int fl = shader->Features & FTR_LINEAR_FOG;
	int fd = shader->Features & FTR_DENSIT_FOG;
	int fm = shader->Features & FTR_HASANY_FOG;

	String_AppendConst(dst,         "precision highp float;\n");
	String_AppendConst(dst,         "varying vec4 out_col;\n");
	if (uv) String_AppendConst(dst, "varying vec2 out_uv;\n");
	if (uv) String_AppendConst(dst, "uniform sampler2D texImage;\n");
	if (fm) String_AppendConst(dst, "uniform vec4 fogCol;\n");
	if (fl) String_AppendConst(dst, "uniform float fogEnd;\n");
	if (fd) String_AppendConst(dst, "uniform float fogDensity;\n");

	String_AppendConst(dst,         "void main() {\n");
	if (uv) String_AppendConst(dst, "  vec4 col = texture2D(texImage, out_uv) * out_col;\n");
	else    String_AppendConst(dst, "  vec4 col = out_col;\n");
	if (al) String_AppendConst(dst, "  if (col.a < 0.5) discard;\n");
	if (fm) String_AppendConst(dst, "  float depth = gl_FragCoord.z / gl_FragCoord.w;\n");
	if (fl) String_AppendConst(dst, "  float f = clamp((fogEnd - depth) / fogEnd, 0.0, 1.0);\n");
	if (fd) String_AppendConst(dst, "  float f = clamp(exp(fogDensity * depth), 0.0, 1.0);\n");
	if (fm) String_AppendConst(dst, "  col = f * col + (1.0 - f) * fogCol;\n");
	String_AppendConst(dst,         "  gl_FragColor = col;\n");
	String_AppendConst(dst,         "}");
}

/* Tries to compile GLSL shader code. Aborts program on failure. */
static GLuint Gfx_CompileShader(GLenum type, const String* src) {
	GLint temp, shader;
	int len;

	shader = glCreateShader(type);
    if (!shader) Logger_Abort("Failed to create shader");
	len = src->length;

    glShaderSource(shader, 1, &src->buffer, &len);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &temp);

    if (temp) return shader;
	temp = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &temp);

	if (temp > 1) {
		char logInfo[2048];
		glGetShaderInfoLog(shader, 2047, NULL, logInfo);

		logInfo[2047] = '\0';
		Platform_LogConst(logInfo);
	}
	Logger_Abort("Failed to compile shader");
	return 0;
}

/* Tries to compile vertex and fragment shaders, then link into an OpenGL program. */
static void Gfx_CompileProgram(struct GLShader* shader) {
	char tmpBuffer[2048]; String tmp;
    GLuint vertex, fragment, program;
    GLint temp;

	String_InitArray(tmp, tmpBuffer);
	Gfx_GenVertexShader(shader, &tmp);
    vertex = Gfx_CompileShader(GL_VERTEX_SHADER, &tmp);

    tmp.length = 0;
    Gfx_GenFragmentShader(shader, &tmp);
    fragment = Gfx_CompileShader(GL_FRAGMENT_SHADER, &tmp);

    program  = glCreateProgram();
    if (!program) Logger_Abort("Failed to create program");
    shader->Program = program;

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &temp);

    if (temp) {
		glDetachShader(program, vertex);
		glDetachShader(program, fragment);

		glDeleteShader(vertex);
		glDeleteShader(fragment);

		shader->Locations[0] = glGetUniformLocation(program, "mvp");
		shader->Locations[1] = glGetUniformLocation(program, "texMatrix");
		shader->Locations[2] = glGetUniformLocation(program, "fogCol");
		shader->Locations[3] = glGetUniformLocation(program, "fogEnd");
		shader->Locations[4] = glGetUniformLocation(program, "fogDensity");
		return;
    }
	temp = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &temp);

	if (temp > 0) {
		glGetProgramInfoLog(program, 2047, NULL, tmpBuffer);
		tmpBuffer[2047] = '\0';
		Platform_LogConst(tmpBuffer);
	}
	Logger_Abort("Failed to compile program");
}

/* Marks a uniform as changed on all programs */
static void Gfx_DirtyUniform(int uniform) {
	int i;
	for (i = 0; i < Array_Elems(shaders); i++) {
		shaders[i].Uniforms |= uniform;
	}
}

/* Sends changes uniforms to the GPU for current program */
static void Gfx_ReloadUniforms(void) {
	struct GLShader* s = gfx_activeShader;

	if (s->Uniforms & UNI_MVP_MATRIX) {
		glUniformMatrix4fv(s->Locations[0], 1, false, (float*)&_mvp);
		s->Uniforms &= ~UNI_MVP_MATRIX;
	}
	if ((s->Uniforms & UNI_TEX_MATRIX) && (s->Features & FTR_TEX_MATRIX)) {
		glUniformMatrix4fv(s->Locations[1], 1, false, (float*)&_tex);
		s->Uniforms &= ~UNI_TEX_MATRIX;
	}
	if ((s->Uniforms & UNI_FOG_COL) && (s->Features & FTR_HASANY_FOG)) {
		glUniform4f(s->Locations[2], gfx_fogCol.R / 255.0f, gfx_fogCol.G / 255.0f, 
									 gfx_fogCol.B / 255.0f, gfx_fogCol.A / 255.0f);
		s->Uniforms &= ~UNI_FOG_COL;
	}
	if ((s->Uniforms & UNI_FOG_END) && (s->Features & FTR_LINEAR_FOG)) {
		glUniform1f(s->Locations[3], gfx_fogEnd);
		s->Uniforms &= ~UNI_FOG_END;
	}
	if ((s->Uniforms & UNI_FOG_DENS) && (s->Features & FTR_DENSIT_FOG)) {
		/* See https://docs.microsoft.com/en-us/previous-versions/ms537113(v%3Dvs.85) */
		/* The equation for EXP mode is exp(-density * z), so just negate density here */
		glUniform1f(s->Locations[4], -gfx_fogDensity);
		s->Uniforms &= ~UNI_FOG_DENS;
	}
}

/* Switches program to one that duplicates current fixed function state */
/* Compiles program and reloads uniforms if needed */
static void Gfx_SwitchProgram(void) {
	struct GLShader* shader;
	int index = 0;

	if (gfx_fogEnabled) {
		index += 6;                       /* linear fog */
		if (gfx_fogMode >= 1) index += 6; /* exp fog */
	}

	if (gfx_batchFormat == VERTEX_FORMAT_P3FT2FC4B) index += 2;
	if (gfx_texTransform) index += 2;
	if (gfx_alphaTest)    index += 1;


	shader = &shaders[index];
	if (shader == gfx_activeShader) { Gfx_ReloadUniforms(); return; }
	if (!shader->Program) Gfx_CompileProgram(shader);

	gfx_activeShader = shader;
	glUseProgram(shader->Program);
	Gfx_ReloadUniforms();
}

void Gfx_SetFog(bool enabled) { gfx_fogEnabled = enabled; Gfx_SwitchProgram(); }
void Gfx_SetFogCol(PackedCol col) {
	if (PackedCol_Equals(col, gfx_fogCol)) return;
	gfx_fogCol = col;
	Gfx_DirtyUniform(UNI_FOG_COL);
	Gfx_ReloadUniforms();
}

void Gfx_SetFogDensity(float value) {
	if (gfx_fogDensity == value) return;
	gfx_fogDensity = value;
	Gfx_DirtyUniform(UNI_FOG_DENS);
	Gfx_ReloadUniforms();
}

void Gfx_SetFogEnd(float value) {
	if (gfx_fogEnd == value) return;
	gfx_fogEnd = value;
	Gfx_DirtyUniform(UNI_FOG_END);
	Gfx_ReloadUniforms();
}

void Gfx_SetFogMode(FogFunc func) {
	if (gfx_fogMode == func) return;
	gfx_fogMode = func;
	Gfx_SwitchProgram();
}

void Gfx_SetTexturing(bool enabled) { }
void Gfx_SetAlphaTest(bool enabled) { gfx_alphaTest = enabled; Gfx_SwitchProgram(); }
void Gfx_SetAlphaTestFunc(CompareFunc func, float refValue) { }

void Gfx_LoadMatrix(MatrixType type, struct Matrix* matrix) {
	if (type == MATRIX_VIEW || type == MATRIX_PROJECTION) {
		if (type == MATRIX_VIEW)       _view = *matrix;
		if (type == MATRIX_PROJECTION) _proj = *matrix;

		Matrix_Mul(&_mvp, &_view, &_proj);
		Gfx_DirtyUniform(UNI_MVP_MATRIX);
		Gfx_ReloadUniforms();
	} else {
		_tex = *matrix;
		gfx_texTransform = true;
		Gfx_DirtyUniform(UNI_TEX_MATRIX);
		Gfx_SwitchProgram();
	}
}
void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (type == MATRIX_VIEW || type == MATRIX_PROJECTION) {
		Gfx_LoadMatrix(type, &Matrix_Identity);
	} else {
		gfx_texTransform = false;
		Gfx_SwitchProgram();
	}
}

static void GL_CheckSupport(void) { }
static void GL_InitState(void) {
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	Gfx_SwitchProgram();
}

static void GL_SetupVbPos3fCol4b(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(VertexP3fC4b), (void*)0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  sizeof(VertexP3fC4b), (void*)12);
}

static void GL_SetupVbPos3fTex2fCol4b(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  sizeof(VertexP3fT2fC4b), (void*)12);
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)16);
}

static void GL_SetupVbPos3fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fC4b);
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(VertexP3fC4b), (void*)(offset));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  sizeof(VertexP3fC4b), (void*)(offset + 12));
}

static void GL_SetupVbPos3fTex2fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)(offset));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_batchFormat) return;
	gfx_batchFormat = fmt;
	gfx_batchStride = gfx_strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_P3FT2FC4B) {
		glEnableVertexAttribArray(2);
		gfx_setupVBFunc      = GL_SetupVbPos3fTex2fCol4b;
		gfx_setupVBRangeFunc = GL_SetupVbPos3fTex2fCol4b_Range;
	} else {
		glDisableVertexAttribArray(2);
		gfx_setupVBFunc      = GL_SetupVbPos3fCol4b;
		gfx_setupVBRangeFunc = GL_SetupVbPos3fCol4b_Range;
	}
	Gfx_SwitchProgram();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	gfx_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)(offset));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
 }
#endif


/*########################################################################################################################*
*------------------------------------------------------OpenGL legacy------------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GLMODERN
void Gfx_SetFog(bool enabled) {
	gfx_fogEnabled = enabled;
	gl_Toggle(GL_FOG);
}

void Gfx_SetFogCol(PackedCol col) {
	float rgba[4];
	if (PackedCol_Equals(col, gfx_fogCol)) return;

	rgba[0] = col.R / 255.0f; rgba[1] = col.G / 255.0f;
	rgba[2] = col.B / 255.0f; rgba[3] = col.A / 255.0f;

	glFogfv(GL_FOG_COLOR, rgba);
	gfx_fogCol = col;
}

void Gfx_SetFogDensity(float value) {
	if (value == gfx_fogDensity) return;
	glFogf(GL_FOG_DENSITY, value);
	gfx_fogDensity = value;
}

void Gfx_SetFogEnd(float value) {
	if (value == gfx_fogEnd) return;
	glFogf(GL_FOG_END, value);
	gfx_fogEnd = value;
}

void Gfx_SetFogMode(FogFunc func) {
	static GLint modes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
	if (func == gfx_fogMode) return;

	glFogi(GL_FOG_MODE, modes[func]);
	gfx_fogMode = func;
}

void Gfx_SetTexturing(bool enabled) { gl_Toggle(GL_TEXTURE_2D); }
void Gfx_SetAlphaTest(bool enabled) { gl_Toggle(GL_ALPHA_TEST); }
void Gfx_SetAlphaTestFunc(CompareFunc func, float value) {
	glAlphaFunc(gl_compare[func], value);
}

static GLenum matrix_modes[3] = { GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE };
static int lastMatrix;

void Gfx_LoadMatrix(MatrixType type, struct Matrix* matrix) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadMatrixf((float*)matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (type != lastMatrix) { lastMatrix = type; glMatrixMode(matrix_modes[type]); }
	glLoadIdentity();
}

static void GL_InitState(void) {
	glHint(GL_FOG_HINT, GL_NICEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
#ifdef CC_BUILD_GL11
/* point to client side dynamic array */
#define VB_PTR ((uint8_t*)gfx_dynamicListData)
#define IB_PTR gl_indices
#else
/* no client side array, use vertex buffer object */
#define VB_PTR 0
#define IB_PTR NULL
#endif

static void GL_SetupVbPos3fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fC4b), (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fC4b), (void*)(VB_PTR + 12));
}

static void GL_SetupVbPos3fTex2fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + 0));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + 12));
	glTexCoordPointer(2, GL_FLOAT,      sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + 16));
}

static void GL_SetupVbPos3fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fC4b);
	glVertexPointer(3, GL_FLOAT,          sizeof(VertexP3fC4b), (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fC4b), (void*)(VB_PTR + offset + 12));
}

static void GL_SetupVbPos3fTex2fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3,  GL_FLOAT,         sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(VB_PTR + offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_batchFormat) return;
	gfx_batchFormat = fmt;
	gfx_batchStride = gfx_strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_P3FT2FC4B) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		gfx_setupVBFunc      = GL_SetupVbPos3fTex2fCol4b;
		gfx_setupVBRangeFunc = GL_SetupVbPos3fTex2fCol4b_Range;
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		gfx_setupVBFunc      = GL_SetupVbPos3fCol4b;
		gfx_setupVBRangeFunc = GL_SetupVbPos3fCol4b_Range;
	}
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
#ifdef CC_BUILD_GL11
	if (gfx_activeList != gl_DYNAMICLISTID) { glCallList(gfx_activeList); return; }
#endif
	gfx_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
#ifdef CC_BUILD_GL11
	if (gfx_activeList != gl_DYNAMICLISTID) { glCallList(gfx_activeList); return; }
#endif
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, IB_PTR);
}

#ifndef CC_BUILD_GL11
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT,      sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
	glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),   GL_UNSIGNED_SHORT, NULL);
}

static void GL_CheckSupport(void) {
	const static String vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	String version    = String_FromReadonly(glGetString(GL_VERSION));

	int major = (int)(version.buffer[0] - '0'); /* x.y. (and so forth) */
	int minor = (int)(version.buffer[2] - '0');

	/* Supported in core since 1.5 */
	if (major > 1 || (major == 1 && minor >= 5)) {
		_glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBuffer");
		_glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffers");
		_glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffers");
		_glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferData");
		_glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubData");
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		_glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBufferARB");
		_glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffersARB");
		_glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffersARB");
		_glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferDataARB");
		_glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubDataARB");
	} else {
		Logger_Abort("Only OpenGL 1.1 supported.\n\n" \
			"Compile the game with CC_BUILD_GL11, or ask on the classicube forums for it");
	}
	Gfx.CustomMipmapsLevels = true;
}
#else
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) { return gl_DYNAMICLISTID;  }
GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count) {
	/* Need to get rid of the 1 extra element, see comment in chunk mesh builder for why */
	count &= ~0x01;
	GLuint list = glGenLists(1);

	/* We need to setup client state properly when building the list */
	int curFormat = gfx_batchFormat;
	Gfx_SetVertexFormat(fmt);
	void* dyn_data     = gfx_dynamicListData;
	gfx_dynamicListData = vertices;

	glNewList(list, GL_COMPILE);
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(count), GL_UNSIGNED_SHORT, gl_indices);
	glEndList();

	Gfx_SetVertexFormat(curFormat);
	gfx_dynamicListData = dyn_data;
	return list;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) { return GFX_NULL; }
void Gfx_BindVb(GfxResourceID vb) { gfx_activeList = (GLuint)vb; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (!vb || *vb == GFX_NULL) return;
	if (*vb != gl_DYNAMICLISTID) glDeleteLists((GLuint)(*vb), 1);
	*vb = GFX_NULL;
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gfx_activeList      = gl_DYNAMICLISTID;
	gfx_dynamicListData = vertices;
}

static GLuint gl_lastPartialList;
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	/* TODO: This renders the whole map, bad performance!! FIX FIX */
	if (gfx_activeList == gl_lastPartialList) return;
	glCallList(gfx_activeList);
	gl_lastPartialList = gfx_activeList;
}

static void GL_CheckSupport(void) {
	Gfx_MakeIndices(gl_indices, GFX_MAX_INDICES);
}
#endif /* CC_BUILD_GL11 */
#endif /* !CC_BUILD_GLMODERN */
#endif
