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

int Gfx_MaxTexWidth, Gfx_MaxTexHeight;
float Gfx_MinZNear;
bool Gfx_LostContext;
bool Gfx_Mipmaps, Gfx_CustomMipmapsLevels;
struct Matrix Gfx_View, Gfx_Projection;

static char Gfx_ApiBuffer[7][STRING_SIZE];
String Gfx_ApiInfo[7] = {
	String_FromArray(Gfx_ApiBuffer[0]), String_FromArray(Gfx_ApiBuffer[1]),
	String_FromArray(Gfx_ApiBuffer[2]), String_FromArray(Gfx_ApiBuffer[3]),
	String_FromArray(Gfx_ApiBuffer[4]), String_FromArray(Gfx_ApiBuffer[5]),
	String_FromArray(Gfx_ApiBuffer[6]),
};

GfxResourceID Gfx_defaultIb;
GfxResourceID Gfx_quadVb, Gfx_texVb;
ScheduledTaskCallback Gfx_LostContextFunction;

static int gfx_strideSizes[2] = { 16, 24 };
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
	Gfx_LostContext = true;
	Platform_Log1("Lost graphics context: %c", reason);

	Event_RaiseVoid(&GfxEvents_ContextLost);
	Gfx_FreeDefaultResources();
}

void Gfx_RecreateContext(void) {
	Gfx_LostContext = false;
	Platform_LogConst("Recreating graphics context");

	Event_RaiseVoid(&GfxEvents_ContextRecreated);
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
	Gfx_LoadMatrix(MATRIX_PROJECTION, &Gfx_Projection);
	Gfx_LoadMatrix(MATRIX_VIEW, &Gfx_View);

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
	if (Gfx_CustomMipmapsLevels) {
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
static D3DFORMAT d3d9_viewFormat, d3d9_depthFormat;

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
	task.Callback = Gfx_LostContextFunction;

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
		d3d9_viewFormat = viewFormats[i];
		res = IDirect3D9_CheckDeviceType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, true);
		if (!res) break;
	}
	if (i == count) Logger_Abort("Unable to create a back buffer with sufficient precision.");

	count = Array_Elems(depthFormats);
	for (i = 0; i < count; i++) {
		d3d9_depthFormat = depthFormats[i];
		res = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, d3d9_depthFormat);
		if (!res) break;
	}
	if (i == count) Logger_Abort("Unable to create a depth buffer with sufficient precision.");
}

static void D3D9_FillPresentArgs(int width, int height, D3DPRESENT_PARAMETERS* args) {
	args->AutoDepthStencilFormat = d3d9_depthFormat;
	args->BackBufferWidth  = width;
	args->BackBufferHeight = height;
	args->BackBufferFormat = d3d9_viewFormat;
	args->BackBufferCount  = 1;
	args->EnableAutoDepthStencil = true;
	args->PresentationInterval   = gfx_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	args->SwapEffect = D3DSWAPEFFECT_DISCARD;
	args->Windowed   = true;
}

static void D3D9_RecreateDevice(void) {
	D3DPRESENT_PARAMETERS args = { 0 };
	D3D9_FillPresentArgs(Game_Width, Game_Height, &args);

	while (IDirect3DDevice9_Reset(device, &args) == D3DERR_DEVICELOST) {
		D3D9_LoopUntilRetrieved();
	}

	D3D9_SetDefaultRenderStates();
	D3D9_RestoreRenderStates();
	Gfx_RecreateContext();
}

void Gfx_Init(void) {
	Gfx_MinZNear = 0.05f;
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

	Gfx_MaxTexWidth  = caps.MaxTextureWidth;
	Gfx_MaxTexHeight = caps.MaxTextureHeight;

	Gfx_CustomMipmapsLevels = true;
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

		Bitmap_Create(&mipmap, width, height, cur);
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
	if (Gfx_Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		if (res) Logger_Abort2(res, "D3D9_EnableMipmaps");
	}
}

void Gfx_DisableMipmaps(void) {
	if (Gfx_Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		if (res) Logger_Abort2(res, "D3D9_DisableMipmaps");
	}
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedColUnion d3d9_fogCol;
static float d3d9_fogDensity = -1.0f, d3d9_fogEnd = -1.0f;
static D3DFOGMODE d3d9_fogMode = D3DFOG_NONE;

static bool d3d9_alphaTesting, d3d9_alphaBlending;
static int d3d9_alphaTestRef;
static D3DCMPFUNC d3d9_alphaTestFunc = D3DCMP_ALWAYS;
static D3DBLEND d3d9_srcBlendFunc = D3DBLEND_ONE, d3d9_dstBlendFunc = D3DBLEND_ZERO;

static PackedColUnion d3d9_clearCol;
static bool d3d9_depthTesting, d3d9_depthWriting;
static D3DCMPFUNC d3d9_depthTestFunc = D3DCMP_LESSEQUAL;

void Gfx_SetFaceCulling(bool enabled) {
	D3DCULL mode = enabled ? D3DCULL_CW : D3DCULL_NONE;
	D3D9_SetRenderState(D3DRS_CULLMODE, mode, "D3D9_SetFaceCulling");
}

void Gfx_SetFog(bool enabled) {
	if (gfx_fogEnabled == enabled) return;
	gfx_fogEnabled = enabled;

	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGENABLE, enabled, "D3D9_SetFog");
}

void Gfx_SetFogCol(PackedCol col) {
	if (PackedCol_Equals(col, d3d9_fogCol.C)) return;
	d3d9_fogCol.C = col;

	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGCOLOR, d3d9_fogCol.Raw, "D3D9_SetFogColour");
}

void Gfx_SetFogDensity(float value) {
	union IntAndFloat raw;
	if (value == d3d9_fogDensity) return;
	d3d9_fogDensity = value;

	if (Gfx_LostContext) return;
	raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGDENSITY, raw.u, "D3D9_SetFogDensity");
}

void Gfx_SetFogEnd(float value) {
	union IntAndFloat raw;
	if (value == d3d9_fogEnd) return;
	d3d9_fogEnd = value;

	if (Gfx_LostContext) return;
	raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGEND, raw.u, "D3D9_SetFogEnd");
}

void Gfx_SetFogMode(FogFunc func) {
	static D3DFOGMODE modes[3] = { D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2 };
	D3DFOGMODE mode = modes[func];
	if (mode == d3d9_fogMode) return;

	d3d9_fogMode = mode;
	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGTABLEMODE, mode, "D3D9_SetFogMode");
}

void Gfx_SetAlphaTest(bool enabled) {
	if (d3d9_alphaTesting == enabled) return;
	d3d9_alphaTesting = enabled;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE, enabled, "D3D9_SetAlphaTest");
}

void Gfx_SetAlphaTestFunc(CompareFunc func, float refValue) {
	d3d9_alphaTestFunc = d3d9_compareFuncs[func];
	D3D9_SetRenderState(D3DRS_ALPHAFUNC, d3d9_alphaTestFunc, "D3D9_SetAlphaTest_Func");
	d3d9_alphaTestRef = (int)(refValue * 255);
	D3D9_SetRenderState2(D3DRS_ALPHAREF, d3d9_alphaTestRef,  "D3D9_SetAlphaTest_Ref");
}

void Gfx_SetAlphaBlending(bool enabled) {
	if (d3d9_alphaBlending == enabled) return;
	d3d9_alphaBlending = enabled;
	D3D9_SetRenderState(D3DRS_ALPHABLENDENABLE, enabled, "D3D9_SetAlphaBlending");
}

void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
	static D3DBLEND funcs[6] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA };

	d3d9_srcBlendFunc = funcs[srcFunc];
	D3D9_SetRenderState(D3DRS_SRCBLEND,   d3d9_srcBlendFunc, "D3D9_SetAlphaBlendFunc_Src");
	d3d9_dstBlendFunc = funcs[dstFunc];
	D3D9_SetRenderState2(D3DRS_DESTBLEND, d3d9_dstBlendFunc, "D3D9_SetAlphaBlendFunc_Dst");
}

void Gfx_SetAlphaArgBlend(bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	ReturnCode res  = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
	if (res) Logger_Abort2(res, "D3D9_SetAlphaArgBlend");
}

void Gfx_ClearCol(PackedCol col) { d3d9_clearCol.C = col; }
void Gfx_SetColWriteMask(bool r, bool g, bool b, bool a) {
	DWORD channels = (r ? 1u : 0u) | (g ? 2u : 0u) | (b ? 4u : 0u) | (a ? 8u : 0u);
	D3D9_SetRenderState(D3DRS_COLORWRITEENABLE, channels, "D3D9_SetColourWrite");
}

void Gfx_SetDepthTest(bool enabled) {
	d3d9_depthTesting = enabled;
	D3D9_SetRenderState(D3DRS_ZENABLE, enabled, "D3D9_SetDepthTest");
}

void Gfx_SetDepthTestFunc(CompareFunc func) {
	d3d9_depthTestFunc = d3d9_compareFuncs[func];
	D3D9_SetRenderState(D3DRS_ZFUNC, d3d9_alphaTestFunc, "D3D9_SetDepthTestFunc");
}

void Gfx_SetDepthWrite(bool enabled) {
	d3d9_depthWriting = enabled;
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
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE,   d3d9_alphaTesting,  "D3D9_AlphaTest");
	D3D9_SetRenderState2(D3DRS_ALPHABLENDENABLE, d3d9_alphaBlending, "D3D9_AlphaBlend");
	D3D9_SetRenderState2(D3DRS_ALPHAFUNC,        d3d9_alphaTestFunc, "D3D9_AlphaTestFunc");
	D3D9_SetRenderState2(D3DRS_ALPHAREF,         d3d9_alphaTestRef,  "D3D9_AlphaRefFunc");
	D3D9_SetRenderState2(D3DRS_SRCBLEND,         d3d9_srcBlendFunc,  "D3D9_AlphaSrcBlend");
	D3D9_SetRenderState2(D3DRS_DESTBLEND,        d3d9_dstBlendFunc,  "D3D9_AlphaDstBlend");

	D3D9_SetRenderState2(D3DRS_FOGENABLE, gfx_fogEnabled,  "D3D9_Fog");
	D3D9_SetRenderState2(D3DRS_FOGCOLOR,  d3d9_fogCol.Raw, "D3D9_FogColor");
	raw.f = d3d9_fogDensity;
	D3D9_SetRenderState2(D3DRS_FOGDENSITY, raw.u,          "D3D9_FogDensity");
	raw.f = d3d9_fogEnd;
	D3D9_SetRenderState2(D3DRS_FOGEND, raw.u,              "D3D9_FogEnd");
	D3D9_SetRenderState2(D3DRS_FOGTABLEMODE, d3d9_fogMode, "D3D9_FogMode");

	D3D9_SetRenderState2(D3DRS_ZFUNC,        d3d9_depthTestFunc, "D3D9_DepthTestFunc");
	D3D9_SetRenderState2(D3DRS_ZENABLE,      d3d9_depthTesting,  "D3D9_DepthTest");
	D3D9_SetRenderState2(D3DRS_ZWRITEENABLE, d3d9_depthWriting,  "D3D9_DepthWrite");
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
		Event_RaiseVoid(&GfxEvents_LowVRAMDetected);
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

	if (Gfx_LostContext) return;
	res = IDirect3DDevice9_SetTransform(device, matrix_modes[type], matrix);
	if (res) Logger_Abort2(res, "D3D9_LoadMatrix");
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	ReturnCode res;
	if (type == MATRIX_TEXTURE) {
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	}

	if (Gfx_LostContext) return;
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
ReturnCode Gfx_TakeScreenshot(struct Stream* output, int width, int height) {
	IDirect3DSurface9* backbuffer = NULL;
	IDirect3DSurface9* temp = NULL;
	ReturnCode res;

	res = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	if (res) goto finished;
	res = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &temp, NULL);
	if (res) goto finished; /* TODO: For DX 8 use IDirect3DDevice8::CreateImageSurface */
	res = IDirect3DDevice9_GetRenderTargetData(device, backbuffer, temp);
	if (res) goto finished;

	D3DLOCKED_RECT rect;
	res = IDirect3DSurface9_LockRect(temp, &rect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
	if (res) goto finished;
	{
		Bitmap bmp; Bitmap_Create(&bmp, width, height, rect.pBits);
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
	ReturnCode res = IDirect3DDevice9_Clear(device, 0, NULL, flags, d3d9_clearCol.Raw, 1.0f, 0);
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
const char* D3D9_StrFlags(void) {
	if (createFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) return "Hardware";
	if (createFlags & D3DCREATE_MIXED_VERTEXPROCESSING)    return "Mixed";
	if (createFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) return "Software";
	return "(none)";
}

const char* D3D9_StrFormat(D3DFORMAT format) {
	switch (format) {
	case D3DFMT_D32:     return "D32";
	case D3DFMT_D24X8:   return "D24X8";
	case D3DFMT_D24S8:   return "D24S8";
	case D3DFMT_D24X4S4: return "D24X4S4";
	case D3DFMT_D16:     return "D16";
	case D3DFMT_D15S1:   return "D15S1";

	case D3DFMT_X8R8G8B8: return "X8R8G8B8";
	case D3DFMT_R8G8B8:   return "R8G8B8";
	case D3DFMT_R5G6B5:   return "R5G6B5";
	case D3DFMT_X1R5G5B5: return "X1R5G5B5";
	}
	return "(unknown)";
}

static float d3d9_totalMem;
void Gfx_MakeApiInfo(void) {
	D3DADAPTER_IDENTIFIER9 adapter = { 0 };
	IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &adapter);
	d3d9_totalMem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);

	String_AppendConst(&Gfx_ApiInfo[0],"-- Using Direct3D9 --");
	String_Format1(&Gfx_ApiInfo[1], "Adapter: %c",         adapter.Description);
	String_Format1(&Gfx_ApiInfo[2], "Processing mode: %c", D3D9_StrFlags());
	Gfx_UpdateApiInfo();
	String_Format2(&Gfx_ApiInfo[4], "Max texture size: (%i, %i)", &Gfx_MaxTexWidth, &Gfx_MaxTexHeight);
	String_Format1(&Gfx_ApiInfo[5], "Depth buffer format: %c",    D3D9_StrFormat(d3d9_depthFormat));
	String_Format1(&Gfx_ApiInfo[6], "Back buffer format: %c",     D3D9_StrFormat(d3d9_viewFormat));
}

void Gfx_UpdateApiInfo(void) {
	float mem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);
	Gfx_ApiInfo[3].length = 0;
	String_Format2(&Gfx_ApiInfo[3], "Video memory: %f2 MB total, %f2 free", &d3d9_totalMem, &mem);
}

void Gfx_OnWindowResize(void) {
	Gfx_LoseContext(" (resizing window)");
	D3D9_RecreateDevice();
}
#endif


/*########################################################################################################################*
*----------------------------------------------------------OpenGL---------------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_D3D9
#ifdef CC_BUILD_WIN
#include <windows.h>
#endif

#ifdef CC_BUILD_OSX
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/* Extensions from later than OpenGL 1.1 */
#define GL_TEXTURE_MAX_LEVEL    0x813D
#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_BGRA_EXT             0x80E1

#ifdef CC_BUILD_GL11
static GfxResourceID gl_activeList;
#define gl_DYNAMICLISTID 1234567891
static void* gl_dynamicListData;
#else
/* Weak linked on OSX, so we don't need to use GetProcAddress */
#ifndef CC_BUILD_OSX
typedef void (APIENTRY *FUNC_GLBINDBUFFER) (GLenum target, GLuint buffer);
typedef void (APIENTRY *FUNC_GLDELETEBUFFERS) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *FUNC_GLGENBUFFERS) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY *FUNC_GLBUFFERDATA) (GLenum target, uintptr_t size, const GLvoid* data, GLenum usage);
typedef void (APIENTRY *FUNC_GLBUFFERSUBDATA) (GLenum target, uintptr_t offset, uintptr_t size, const GLvoid* data);
static FUNC_GLBINDBUFFER    glBindBuffer;
static FUNC_GLDELETEBUFFERS glDeleteBuffers;
static FUNC_GLGENBUFFERS    glGenBuffers;
static FUNC_GLBUFFERDATA    glBufferData;
static FUNC_GLBUFFERSUBDATA glBufferSubData;
#endif
#endif

static int gl_compare[8] = { GL_ALWAYS, GL_NOTEQUAL, GL_NEVER, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER };
typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gl_setupVBFunc;
static GL_SetupVBRangeFunc gl_setupVBRangeFunc;

#ifndef CC_BUILD_GL11
static void GL_CheckVboSupport(void) {
	const static String vboExt = String_FromConst("GL_ARB_vertex_buffer_object");
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));
	String version    = String_FromReadonly(glGetString(GL_VERSION));

	int major = (int)(version.buffer[0] - '0'); /* x.y. (and so forth) */
	int minor = (int)(version.buffer[2] - '0');

	/* Supported in core since 1.5 */
	if ((major > 1) || (major == 1 && minor >= 5)) {
		/* TODO: Do we still need to think about ARB functions on OSX? */
#ifndef CC_BUILD_OSX
		glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBuffer");
		glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffers");
		glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffers");
		glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferData");
		glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubData");
	} else if (String_CaselessContains(&extensions, &vboExt)) {
		glBindBuffer    = (FUNC_GLBINDBUFFER)GLContext_GetAddress("glBindBufferARB");
		glDeleteBuffers = (FUNC_GLDELETEBUFFERS)GLContext_GetAddress("glDeleteBuffersARB");
		glGenBuffers    = (FUNC_GLGENBUFFERS)GLContext_GetAddress("glGenBuffersARB");
		glBufferData    = (FUNC_GLBUFFERDATA)GLContext_GetAddress("glBufferDataARB");
		glBufferSubData = (FUNC_GLBUFFERSUBDATA)GLContext_GetAddress("glBufferSubDataARB");
#endif
	} else {
		Logger_Abort("Only OpenGL 1.1 supported.\r\n\r\n" \
			"Compile the game with CC_BUILD_GL11, or ask on the classicube forums for it");
	}
}
#endif

void Gfx_Init(void) {
	struct GraphicsMode mode;
	GraphicsMode_MakeDefault(&mode);
	GLContext_Init(&mode);

	Gfx_MinZNear = 0.1f;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx_MaxTexWidth);
	Gfx_MaxTexHeight = Gfx_MaxTexWidth;

#ifndef CC_BUILD_GL11
	Gfx_CustomMipmapsLevels = true;
	GL_CheckVboSupport();
#endif
	Gfx_InitDefaultResources();

	glHint(GL_FOG_HINT, GL_NICEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

void Gfx_Free(void) {
	Gfx_FreeDefaultResources();
	GLContext_Free();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void GL_DoMipmaps(GfxResourceID texId, int x, int y, Bitmap* bmp, bool partial) {
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
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, cur);
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
		if (Gfx_CustomMipmapsLevels) {
			int lvls = Gfx_MipmapsLevels(bmp->Width, bmp->Height);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->Width, bmp->Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp->Scan0);

	if (mipmaps) GL_DoMipmaps(texId, 0, 0, bmp, false);
	return texId;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, Bitmap* part, bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->Width, part->Height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, part->Scan0);
	if (mipmaps) GL_DoMipmaps(texId, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	glBindTexture(GL_TEXTURE_2D, texId);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	if (!texId || *texId == GFX_NULL) return;
	glDeleteTextures(1, texId);
	*texId = GFX_NULL;
}

void Gfx_SetTexturing(bool enabled) { gl_Toggle(GL_TEXTURE_2D); }
void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gl_lastFogCol;
static float gl_lastFogEnd = -1, gl_lastFogDensity = -1;
static int gl_lastFogMode = -1;
static PackedCol gl_lastClearCol;

void Gfx_SetFog(bool enabled) {
	gfx_fogEnabled = enabled;
	gl_Toggle(GL_FOG);
}

void Gfx_SetFogCol(PackedCol col) {
	float rgba[4];
	if (PackedCol_Equals(col, gl_lastFogCol)) return;

	rgba[0] = col.R / 255.0f; rgba[1] = col.G / 255.0f;
	rgba[2] = col.B / 255.0f; rgba[3] = col.A / 255.0f;

	glFogfv(GL_FOG_COLOR, rgba);
	gl_lastFogCol = col;
}

void Gfx_SetFogDensity(float value) {
	if (value == gl_lastFogDensity) return;
	glFogf(GL_FOG_DENSITY, value);
	gl_lastFogDensity = value;
}

void Gfx_SetFogEnd(float value) {
	if (value == gl_lastFogEnd) return;
	glFogf(GL_FOG_END, value);
	gl_lastFogEnd = value;
}

void Gfx_SetFogMode(FogFunc func) {
	static GLint modes[3] = { GL_LINEAR, GL_EXP, GL_EXP2 };
	if (func == gl_lastFogMode) return;

	glFogi(GL_FOG_MODE, modes[func]);
	gl_lastFogMode = func;
}

void Gfx_SetFaceCulling(bool enabled) { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaTest(bool enabled) { gl_Toggle(GL_ALPHA_TEST); }
void Gfx_SetAlphaTestFunc(CompareFunc func, float value) {
	glAlphaFunc(gl_compare[func], value);
}

void Gfx_SetAlphaBlending(bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaBlendFunc(BlendFunc srcFunc, BlendFunc dstFunc) {
	static GLenum funcs[6] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };
	glBlendFunc(funcs[srcFunc], funcs[dstFunc]);
}
void Gfx_SetAlphaArgBlend(bool enabled) { }

void Gfx_ClearCol(PackedCol col) {
	if (PackedCol_Equals(col, gl_lastClearCol)) return;
	glClearColor(col.R / 255.0f, col.G / 255.0f, col.B / 255.0f, col.A / 255.0f);
	gl_lastClearCol = col;
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


/*########################################################################################################################*
*---------------------------------------------------Vertex/Index buffers--------------------------------------------------*
*#########################################################################################################################*/
#ifndef CC_BUILD_GL11
static GfxResourceID GL_GenAndBind(GLenum target) {
	GfxResourceID id;
	glGenBuffers(1, &id);
	glBindBuffer(target, id);
	return id;
}

GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	GfxResourceID id = GL_GenAndBind(GL_ARRAY_BUFFER);
	uint32_t size    = maxVertices * gfx_strideSizes[fmt];
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	return id;
}

GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count) {
	GfxResourceID id = GL_GenAndBind(GL_ARRAY_BUFFER);
	uint32_t size    = count * gfx_strideSizes[fmt];
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	return id;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) {
	GfxResourceID id = GL_GenAndBind(GL_ELEMENT_ARRAY_BUFFER);
	uint32_t size    = indicesCount * 2;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	return id;
}

void Gfx_BindVb(GfxResourceID vb) { glBindBuffer(GL_ARRAY_BUFFER, vb); }
void Gfx_BindIb(GfxResourceID ib) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib); }

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (!vb || *vb == GFX_NULL) return;
	glDeleteBuffers(1, vb);
	*vb = GFX_NULL;
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	if (!ib || *ib == GFX_NULL) return;
	glDeleteBuffers(1, ib);
	*ib = GFX_NULL;
}
#else
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) { return gl_DYNAMICLISTID;  }
GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat fmt, int count) {
	/* We need to setup client state properly when building the list */
	int curFormat = gfx_batchFormat, stride;
	Gfx_SetVertexFormat(fmt);

	GfxResourceID list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	count &= ~0x01; /* Need to get rid of the 1 extra element, see comment in chunk mesh builder for why */

	uint16_t indices[GFX_MAX_INDICES];
	Gfx_MakeIndices(indices, ICOUNT(count));
	stride = gfx_strideSizes[fmt];

	glVertexPointer(3, GL_FLOAT, stride, vertices);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, (void*)((uint8_t*)vertices + 12));
	if (fmt == VERTEX_FORMAT_P3FT2FC4B) {
		glTexCoordPointer(2, GL_FLOAT, stride, (void*)((uint8_t*)vertices + 16));
	}

	glDrawElements(GL_TRIANGLES, ICOUNT(count), GL_UNSIGNED_SHORT, indices);
	glEndList();
	Gfx_SetVertexFormat(curFormat);
	return list;
}

GfxResourceID Gfx_CreateIb(void* indices, int indicesCount) { return GFX_NULL; }
void Gfx_BindVb(GfxResourceID vb) { gl_activeList = vb; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }

void Gfx_DeleteVb(GfxResourceID* vb) {
	if (!vb || *vb == GFX_NULL) return;
	if (*vb != gl_DYNAMICLISTID) glDeleteLists(*vb, 1);
	*vb = GFX_NULL;
}
#endif

void GL_SetupVbPos3fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fC4b), (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fC4b), (void*)12);
}

void GL_SetupVbPos3fTex2fCol4b(void) {
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)0);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fT2fC4b), (void*)12);
	glTexCoordPointer(2, GL_FLOAT,      sizeof(VertexP3fT2fC4b), (void*)16);
}

void GL_SetupVbPos3fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fC4b);
	glVertexPointer(3, GL_FLOAT,          sizeof(VertexP3fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fC4b), (void*)(offset + 12));
}

void GL_SetupVbPos3fTex2fCol4b_Range(int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3,  GL_FLOAT,         sizeof(VertexP3fT2fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE,   sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_batchFormat) return;

	if (gfx_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	gfx_batchFormat = fmt;
	gfx_batchStride = gfx_strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_P3FT2FC4B) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		gl_setupVBFunc      = GL_SetupVbPos3fTex2fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fTex2fCol4b_Range;
	} else {
		gl_setupVBFunc      = GL_SetupVbPos3fCol4b;
		gl_setupVBRangeFunc = GL_SetupVbPos3fCol4b_Range;
	}
}

#ifndef CC_BUILD_GL11
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	uint32_t size = vCount * gfx_batchStride;
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gl_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	gl_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	gl_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	uint32_t offset = startVertex * (uint32_t)sizeof(VertexP3fT2fC4b);
	glVertexPointer(3, GL_FLOAT,        sizeof(VertexP3fT2fC4b), (void*)(offset));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexP3fT2fC4b), (void*)(offset + 12));
	glTexCoordPointer(2, GL_FLOAT,      sizeof(VertexP3fT2fC4b), (void*)(offset + 16));
	glDrawElements(GL_TRIANGLES,        ICOUNT(verticesCount),   GL_UNSIGNED_SHORT, NULL);
}
#else
void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	gl_activeList = gl_DYNAMICLISTID;
	gl_dynamicListData = vertices;
}

static void GL_V16(VertexP3fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glVertex3f(v.X, v.Y, v.Z);
}

static void GL_V24(VertexP3fT2fC4b v) {
	glColor4ub(v.Col.R, v.Col.G, v.Col.B, v.Col.A);
	glTexCoord2f(v.U, v.V);
	glVertex3f(v.X, v.Y, v.Z);
}

static void GL_DrawDynamicTriangles(int verticesCount, int startVertex) {
	int i;
	glBegin(GL_TRIANGLES);
	
	if (gfx_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)gl_dynamicListData;
		for (i = startVertex; i < startVertex + verticesCount; i += 4) {
			GL_V24(ptr[i + 0]); GL_V24(ptr[i + 1]); GL_V24(ptr[i + 2]);
			GL_V24(ptr[i + 2]); GL_V24(ptr[i + 3]); GL_V24(ptr[i + 0]);
		}
	} else {
		VertexP3fC4b* ptr = (VertexP3fC4b*)gl_dynamicListData;
		for (i = startVertex; i < startVertex + verticesCount; i += 4) {
			GL_V16(ptr[i + 0]); GL_V16(ptr[i + 1]); GL_V16(ptr[i + 2]);
			GL_V16(ptr[i + 2]); GL_V16(ptr[i + 3]); GL_V16(ptr[i + 0]);
		}
	}
	glEnd();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	int i;
	glBegin(GL_LINES);
	
	if (gfx_batchFormat == VERTEX_FORMAT_P3FT2FC4B) {
		VertexP3fT2fC4b* ptr = (VertexP3fT2fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) { GL_V24(ptr[i + 0]); GL_V24(ptr[i + 1]); }
	} else {
		VertexP3fC4b* ptr = (VertexP3fC4b*)gl_dynamicListData;
		for (i = 0; i < verticesCount; i += 2) { GL_V16(ptr[i + 0]); GL_V16(ptr[i + 1]); }
	}
	glEnd();
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); } 
	else { GL_DrawDynamicTriangles(verticesCount, startVertex); }
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	if (gl_activeList != gl_DYNAMICLISTID) { glCallList(gl_activeList); } 
	else { GL_DrawDynamicTriangles(verticesCount, 0); }
}

static GfxResourceID gl_lastPartialList;
void Gfx_DrawIndexedVb_TrisT2fC4b(int verticesCount, int startVertex) {
	/* TODO: This renders the whole map, bad performance!! FIX FIX */
	if (gl_activeList == gl_lastPartialList) return;
	glCallList(gl_activeList);
	gl_lastPartialList = gl_activeList;
}
#endif


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
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

void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_OrthographicOffCenter(matrix, 0.0f, width, height, 0.0f, -10000.0f, 10000.0f);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix) {
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static int GL_SelectRow(Bitmap* bmp, int y) { return (bmp->Height - 1) - y; }
ReturnCode Gfx_TakeScreenshot(struct Stream* output, int width, int height) {
	Bitmap bmp;
	ReturnCode res;

	Bitmap_Allocate(&bmp, width, height);
	glReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp.Scan0);

	res = Png_Encode(&bmp, output, GL_SelectRow, false);
	Mem_Free(bmp.Scan0);
	return res;
}

static bool nv_mem;
void Gfx_MakeApiInfo(void) {
	const static String memExt = String_FromConst("GL_NVX_gpu_memory_info");
	String extensions = String_FromReadonly(glGetString(GL_EXTENSIONS));	
	int depthBits;

	nv_mem = String_CaselessContains(&extensions, &memExt);
	glGetIntegerv(GL_DEPTH_BITS, &depthBits);

	String_AppendConst(&Gfx_ApiInfo[0],"-- Using OpenGL --");
	String_Format1(&Gfx_ApiInfo[1], "Vendor: %c",     glGetString(GL_VENDOR));
	String_Format1(&Gfx_ApiInfo[2], "Renderer: %c",   glGetString(GL_RENDERER));
	String_Format1(&Gfx_ApiInfo[3], "GL version: %c", glGetString(GL_VERSION));
	/* Memory usage line goes here */
	String_Format2(&Gfx_ApiInfo[5], "Max texture size: (%i, %i)", &Gfx_MaxTexWidth, &Gfx_MaxTexHeight);
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
#ifdef CC_BUILD_GL11
	gl_activeList = NULL;
#endif
}

void Gfx_OnWindowResize(void) {
	glViewport(0, 0, Game_Width, Game_Height);
	GLContext_Update();
}
#endif
