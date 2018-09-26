#include "GraphicsAPI.h"
#if CC_BUILD_D3D9
#include "ErrorHandler.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsCommon.h"
#include "Funcs.h"
#include "Game.h"
#include "ExtMath.h"
#include "Bitmap.h"
#include "Event.h"

//#define D3D_DISABLE_9EX causes compile errors
#if CC_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#endif
#include <d3d9.h>
#include <d3d9caps.h>
#include <d3d9types.h>

Int32 Gfx_strideSizes[2] = GFX_STRIDE_SIZES;
D3DFORMAT d3d9_depthFormats[6] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1 };
D3DFORMAT d3d9_viewFormats[4] = { D3DFMT_X8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 };
D3DBLEND d3d9_blendFuncs[6] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA };
D3DCMPFUNC d3d9_compareFuncs[8] = { D3DCMP_ALWAYS, D3DCMP_NOTEQUAL, D3DCMP_NEVER, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER };
D3DFOGMODE d3d9_modes[3] = { D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2 };
UInt32 d3d9_formatMappings[2] = { D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 };

bool d3d9_vsync;
IDirect3D9* d3d;
IDirect3DDevice9* device;
D3DTRANSFORMSTATETYPE curMatrix;
DWORD createFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
D3DFORMAT d3d9_viewFormat, d3d9_depthFormat;

#define D3D9_SetRenderState(state, value, name) \
ReturnCode res = IDirect3DDevice9_SetRenderState(device, state, value); if (res) ErrorHandler_Fail2(res, name);
#define D3D9_SetRenderState2(state, value, name) \
res = IDirect3DDevice9_SetRenderState(device, state, value);            if (res) ErrorHandler_Fail2(res, name);


/* Forward declarations for these two functions. */
static void D3D9_SetDefaultRenderStates(void);
static void D3D9_RestoreRenderStates(void);

static void D3D9_FreeResource(GfxResourceID* resource) {
	if (resource == NULL || *resource == NULL) return;
	IUnknown* unk = (IUnknown*)(*resource);
	ULONG refCount = unk->lpVtbl->Release(unk);
	*resource = NULL;
	if (refCount <= 0) return;

	UInt64 addr = (UInt64)(unk);
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
	Int32 count = Array_Elems(d3d9_viewFormats);
	Int32 i;
	ReturnCode res;

	for (i = 0; i < count; i++) {
		d3d9_viewFormat = d3d9_viewFormats[i];
		res = IDirect3D9_CheckDeviceType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, true);
		if (!res) break;

		if (i == count - 1) {
			ErrorHandler_Fail("Unable to create a back buffer with sufficient precision.");
		}
	}

	count = Array_Elems(d3d9_depthFormats);
	for (i = 0; i < count; i++) {
		d3d9_depthFormat = d3d9_depthFormats[i];
		res = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, d3d9_depthFormat);
		if (!res) break;

		if (i == count - 1) {
			ErrorHandler_Fail("Unable to create a depth buffer with sufficient precision.");
		}
	}
}

static void D3D9_FillPresentArgs(Int32 width, Int32 height, D3DPRESENT_PARAMETERS* args) {
	args->AutoDepthStencilFormat = d3d9_depthFormat;
	args->BackBufferWidth = width;
	args->BackBufferHeight = height;
	args->BackBufferFormat = d3d9_viewFormat;
	args->BackBufferCount = 1;
	args->EnableAutoDepthStencil = true;
	args->PresentationInterval = d3d9_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	args->SwapEffect = D3DSWAPEFFECT_DISCARD;
	args->Windowed = true;
}

static void D3D9_RecreateDevice(void) {
	D3DPRESENT_PARAMETERS args = { 0 };
	D3D9_FillPresentArgs(Game_Width, Game_Height, &args);

	while (IDirect3DDevice9_Reset(device, &args) == D3DERR_DEVICELOST) {
		D3D9_LoopUntilRetrieved();
	}

	D3D9_SetDefaultRenderStates();
	D3D9_RestoreRenderStates();
	GfxCommon_RecreateContext();
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
	if (res) ErrorHandler_Fail2(res, "Creating Direct3D9 device");

	D3DCAPS9 caps;
	res = IDirect3DDevice9_GetDeviceCaps(device, &caps);
	if (res) ErrorHandler_Fail2(res, "Getting Direct3D9 capabilities");

	Gfx_MaxTexWidth  = caps.MaxTextureWidth;
	Gfx_MaxTexHeight = caps.MaxTextureHeight;

	Gfx_CustomMipmapsLevels = true;
	D3D9_SetDefaultRenderStates();
	GfxCommon_Init();
}

void Gfx_Free(void) { 
	GfxCommon_Free();
	D3D9_FreeResource(&device);
	D3D9_FreeResource(&d3d);
}

static void D3D9_SetTextureData(IDirect3DTexture9* texture, Bitmap* bmp, Int32 lvl) {
	D3DLOCKED_RECT rect;
	ReturnCode res = IDirect3DTexture9_LockRect(texture, lvl, &rect, NULL, 0);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetTextureData - Lock");

	UInt32 size = Bitmap_DataSize(bmp->Width, bmp->Height);
	Mem_Copy(rect.pBits, bmp->Scan0, size);

	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetTextureData - Unlock");
}

static void D3D9_SetTexturePartData(IDirect3DTexture9* texture, Int32 x, Int32 y, Bitmap* bmp, Int32 lvl) {
	RECT part;
	part.left = x; part.right = x + bmp->Width;
	part.top = y; part.bottom = y + bmp->Height;

	D3DLOCKED_RECT rect;
	ReturnCode res = IDirect3DTexture9_LockRect(texture, lvl, &rect, &part, 0);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetTexturePartData - Lock");

	/* We need to copy scanline by scanline, as generally rect.stride != data.stride */
	UInt8* src = (UInt8*)bmp->Scan0;
	UInt8* dst = (UInt8*)rect.pBits;
	Int32 yy;
	UInt32 stride = (UInt32)(bmp->Width) * BITMAP_SIZEOF_PIXEL;

	for (yy = 0; yy < bmp->Height; yy++) {
		Mem_Copy(dst, src, stride);
		src += stride; 
		dst += rect.Pitch;
	}

	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetTexturePartData - Unlock");
}

static void D3D9_DoMipmaps(IDirect3DTexture9* texture, Int32 x, Int32 y, Bitmap* bmp, bool partial) {
	UInt8* prev = bmp->Scan0;
	Int32 lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	Int32 lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2; 
		if (width > 1)   width /= 2; 
		if (height > 1) height /= 2;

		UInt8* cur = Mem_Alloc(width * height, BITMAP_SIZEOF_PIXEL, "mipmaps");
		GfxCommon_GenMipmaps(width, height, cur, prev);

		Bitmap mipmap;
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
	Int32 mipmapsLevels = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	Int32 levels = 1 + (mipmaps ? mipmapsLevels : 0);

	if (!Math_IsPowOf2(bmp->Width) || !Math_IsPowOf2(bmp->Height)) {
		ErrorHandler_Fail("Textures must have power of two dimensions");
	}

	if (managedPool) {
		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 
			0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);
		if (res) ErrorHandler_Fail2(res, "D3D9_CreateTexture");

		D3D9_SetTextureData(tex, bmp, 0);
		if (mipmaps) D3D9_DoMipmaps(tex, 0, 0, bmp, false);
	} else {
		IDirect3DTexture9* sys;
		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 
			0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, NULL);
		if (res) ErrorHandler_Fail2(res, "D3D9_CreateTexture - SystemMem");

		D3D9_SetTextureData(sys, bmp, 0);
		if (mipmaps) D3D9_DoMipmaps(sys, 0, 0, bmp, false);

		res = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 
			0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
		if (res) ErrorHandler_Fail2(res, "D3D9_CreateTexture - GPU");

		res = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)sys, (IDirect3DBaseTexture9*)tex);
		if (res) ErrorHandler_Fail2(res, "D3D9_CreateTexture - Update");
		D3D9_FreeResource(&sys);
	}
	return tex;
}

void Gfx_UpdateTexturePart(GfxResourceID texId, Int32 x, Int32 y, Bitmap* part, bool mipmaps) {
	IDirect3DTexture9* texture = (IDirect3DTexture9*)texId;
	D3D9_SetTexturePartData(texture, x, y, part, 0);
	if (mipmaps) D3D9_DoMipmaps(texture, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	ReturnCode res = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9*)texId);
	if (res) ErrorHandler_Fail2(res, "D3D9_BindTexture");
}

void Gfx_DeleteTexture(GfxResourceID* texId) { D3D9_FreeResource(texId); }

void Gfx_SetTexturing(bool enabled) {
	if (enabled) return;
	ReturnCode res = IDirect3DDevice9_SetTexture(device, 0, NULL);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetTexturing");
}

void Gfx_EnableMipmaps(void) {
	if (Gfx_Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		if (res) ErrorHandler_Fail2(res, "D3D9_EnableMipmaps");
	}
}

void Gfx_DisableMipmaps(void) {
	if (Gfx_Mipmaps) {
		ReturnCode res = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		if (res) ErrorHandler_Fail2(res, "D3D9_DisableMipmaps");
	}
}


bool d3d9_fogEnable = false;
bool Gfx_GetFog(void) { return d3d9_fogEnable; }
void Gfx_SetFog(bool enabled) {
	if (d3d9_fogEnable == enabled) return;
	d3d9_fogEnable = enabled;
	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGENABLE, (UInt32)enabled, "D3D9_SetFog");
}

UInt32 d3d9_fogCol = 0xFF000000; /* black */
void Gfx_SetFogCol(PackedCol col) {
	if (col.Packed == d3d9_fogCol) return;
	d3d9_fogCol = col.Packed;
	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGCOLOR, col.Packed, "D3D9_SetFogColour");
}

float d3d9_fogDensity = -1.0f;
void Gfx_SetFogDensity(float value) {
	if (value == d3d9_fogDensity) return;
	d3d9_fogDensity = value;
	if (Gfx_LostContext) return;
	union IntAndFloat raw; raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGDENSITY, raw.u, "D3D9_SetFogDensity");
}

float d3d9_fogEnd = -1.0f;
void Gfx_SetFogEnd(float value) {
	if (value == d3d9_fogEnd) return;
	d3d9_fogEnd = value;
	if (Gfx_LostContext) return;
	union IntAndFloat raw; raw.f = value;
	D3D9_SetRenderState(D3DRS_FOGEND, raw.u, "D3D9_SetFogEnd");
}

D3DFOGMODE d3d9_fogTableMode = D3DFOG_NONE;
void Gfx_SetFogMode(Int32 fogMode) {
	D3DFOGMODE mode = d3d9_modes[fogMode];
	if (mode == d3d9_fogTableMode) return;
	d3d9_fogTableMode = mode;
	if (Gfx_LostContext) return;
	D3D9_SetRenderState(D3DRS_FOGTABLEMODE, mode, "D3D9_SetFogMode");
}


void Gfx_SetFaceCulling(bool enabled) {
	D3DCULL mode = enabled ? D3DCULL_CW : D3DCULL_NONE;
	D3D9_SetRenderState(D3DRS_CULLMODE, mode, "D3D9_SetFaceCulling");
}

bool d3d9_alphaTest = false;
void Gfx_SetAlphaTest(bool enabled) {
	if (d3d9_alphaTest == enabled) return;

	d3d9_alphaTest = enabled;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE, (UInt32)enabled, "D3D9_SetAlphaTest");
}

D3DCMPFUNC d3d9_alphaTestFunc = D3DCMP_ALWAYS;
Int32 d3d9_alphaTestRef = 0;
void Gfx_SetAlphaTestFunc(Int32 compareFunc, float refValue) {
	d3d9_alphaTestFunc = d3d9_compareFuncs[compareFunc];
	D3D9_SetRenderState(D3DRS_ALPHAFUNC, d3d9_alphaTestFunc, "D3D9_SetAlphaTest_Func");
	d3d9_alphaTestRef = (Int32)(refValue * 255);
	D3D9_SetRenderState2(D3DRS_ALPHAREF, d3d9_alphaTestRef, "D3D9_SetAlphaTest_Ref");
}

bool d3d9_alphaBlend = false;
void Gfx_SetAlphaBlending(bool enabled) {
	if (d3d9_alphaBlend == enabled) return;

	d3d9_alphaBlend = enabled;
	D3D9_SetRenderState(D3DRS_ALPHABLENDENABLE, (UInt32)enabled, "D3D9_SetAlphaBlending");
}

D3DBLEND d3d9_srcBlendFunc = D3DBLEND_ONE;
D3DBLEND d3d9_dstBlendFunc = D3DBLEND_ZERO;
void Gfx_SetAlphaBlendFunc(Int32 srcBlendFunc, Int32 dstBlendFunc) {
	d3d9_srcBlendFunc = d3d9_blendFuncs[srcBlendFunc];
	D3D9_SetRenderState(D3DRS_SRCBLEND, d3d9_srcBlendFunc, "D3D9_SetAlphaBlendFunc_Src");
	d3d9_dstBlendFunc = d3d9_blendFuncs[dstBlendFunc];
	D3D9_SetRenderState2(D3DRS_DESTBLEND, d3d9_dstBlendFunc, "D3D9_SetAlphaBlendFunc_Dst");
}

void Gfx_SetAlphaArgBlend(bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	ReturnCode res = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetAlphaArgBlend");
}


UInt32 d3d9_clearCol = 0xFF000000;
void Gfx_Clear(void) {
	DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	ReturnCode res = IDirect3DDevice9_Clear(device, 0, NULL, flags, d3d9_clearCol, 1.0f, 0);
	if (res) ErrorHandler_Fail2(res, "D3D9_Clear");
}

void Gfx_ClearCol(PackedCol col) {
	d3d9_clearCol = col.Packed;
}

bool d3d9_depthTest = false;
void Gfx_SetDepthTest(bool enabled) {
	d3d9_depthTest = enabled;
	D3D9_SetRenderState(D3DRS_ZENABLE, (UInt32)enabled, "D3D9_SetDepthTest");
}

D3DCMPFUNC d3d9_depthTestFunc = D3DCMP_LESSEQUAL;
void Gfx_SetDepthTestFunc(Int32 compareFunc) {
	d3d9_depthTestFunc = d3d9_compareFuncs[compareFunc];
	D3D9_SetRenderState(D3DRS_ZFUNC, d3d9_alphaTestFunc, "D3D9_SetDepthTestFunc");
}

void Gfx_SetColourWriteMask(bool r, bool g, bool b, bool a) {
	UInt32 channels = (r ? 1u : 0u) | (g ? 2u : 0u) | (b ? 4u : 0u) | (a ? 8u : 0u);
	D3D9_SetRenderState(D3DRS_COLORWRITEENABLE, channels, "D3D9_SetColourWrite");
}

bool d3d9_depthWrite = false;
void Gfx_SetDepthWrite(bool enabled) {
	d3d9_depthWrite = enabled;
	D3D9_SetRenderState(D3DRS_ZWRITEENABLE, (UInt32)enabled, "D3D9_SetDepthWrite");
}


GfxResourceID Gfx_CreateDynamicVb(Int32 vertexFormat, Int32 maxVertices) {
	Int32 size = maxVertices * Gfx_strideSizes[vertexFormat];
	IDirect3DVertexBuffer9* vbuffer;
	ReturnCode res = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		d3d9_formatMappings[vertexFormat], D3DPOOL_DEFAULT, &vbuffer, NULL);
	if (res) ErrorHandler_Fail2(res, "D3D9_CreateDynamicVb");

	return vbuffer;
}

static void D3D9_SetVbData(IDirect3DVertexBuffer9* buffer, void* data, Int32 size, const char* lockMsg, const char* unlockMsg, Int32 lockFlags) {
	void* dst = NULL;
	ReturnCode res = IDirect3DVertexBuffer9_Lock(buffer, 0, size, &dst, lockFlags);
	if (res) ErrorHandler_Fail2(res, lockMsg);

	Mem_Copy(dst, data, size);
	res = IDirect3DVertexBuffer9_Unlock(buffer);
	if (res) ErrorHandler_Fail2(res, unlockMsg);
}

GfxResourceID Gfx_CreateVb(void* vertices, Int32 vertexFormat, Int32 count) {
	Int32 size = count * Gfx_strideSizes[vertexFormat];
	IDirect3DVertexBuffer9* vbuffer;

	for (;;) {
		ReturnCode res = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_WRITEONLY,
			d3d9_formatMappings[vertexFormat], D3DPOOL_DEFAULT, &vbuffer, NULL);
		if (!res) break;

		if (res != D3DERR_OUTOFVIDEOMEMORY) ErrorHandler_Fail2(res, "D3D9_CreateVb");
		Event_RaiseVoid(&GfxEvents_LowVRAMDetected);
	}

	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_CreateVb - Lock", "D3D9_CreateVb - Unlock", 0);
	return vbuffer;
}

static void D3D9_SetIbData(IDirect3DIndexBuffer9* buffer, void* data, Int32 size) {
	void* dst = NULL;
	ReturnCode res = IDirect3DIndexBuffer9_Lock(buffer, 0, size, &dst, 0);
	if (res) ErrorHandler_Fail2(res, "D3D9_CreateIb - Lock");

	Mem_Copy(dst, data, size);
	res = IDirect3DIndexBuffer9_Unlock(buffer);
	if (res) ErrorHandler_Fail2(res, "D3D9_CreateIb - Unlock");
}

GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount) {
	Int32 size = indicesCount * sizeof(UInt16);
	IDirect3DIndexBuffer9* ibuffer;
	ReturnCode res = IDirect3DDevice9_CreateIndexBuffer(device, size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibuffer, NULL);
	if (res) ErrorHandler_Fail2(res, "D3D9_CreateIb");

	D3D9_SetIbData(ibuffer, indices, size);
	return ibuffer;
}

Int32 d3d9_batchStride;
void Gfx_BindVb(GfxResourceID vb) {
	IDirect3DVertexBuffer9* vbuffer = (IDirect3DVertexBuffer9*)vb;
	ReturnCode res = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, d3d9_batchStride);
	if (res) ErrorHandler_Fail2(res, "D3D9_BindVb");
}

void Gfx_BindIb(GfxResourceID ib) {
	IDirect3DIndexBuffer9* ibuffer = (IDirect3DIndexBuffer9*)ib;
	ReturnCode res = IDirect3DDevice9_SetIndices(device, ibuffer);
	if (res) ErrorHandler_Fail2(res, "D3D9_BindIb");
}

void Gfx_DeleteVb(GfxResourceID* vb) { D3D9_FreeResource(vb); }
void Gfx_DeleteIb(GfxResourceID* ib) { D3D9_FreeResource(ib); }

Int32 d3d9_batchFormat = -1;
void Gfx_SetBatchFormat(Int32 format) {
	if (format == d3d9_batchFormat) return;
	d3d9_batchFormat = format;

	ReturnCode res = IDirect3DDevice9_SetFVF(device, d3d9_formatMappings[format]);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetBatchFormat");
	d3d9_batchStride = Gfx_strideSizes[format];
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount) {
	Int32 size = vCount * d3d9_batchStride;
	IDirect3DVertexBuffer9* vbuffer = (IDirect3DVertexBuffer9*)vb;
	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_SetDynamicVbData - Lock", "D3D9_SetDynamicVbData - Unlock", D3DLOCK_DISCARD);

	ReturnCode res = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, d3d9_batchStride);
	if (res) ErrorHandler_Fail2(res, "D3D9_SetDynamicVbData - Bind");
}

void Gfx_DrawVb_Lines(Int32 verticesCount) {
	ReturnCode res = IDirect3DDevice9_DrawPrimitive(device, D3DPT_LINELIST, 0, verticesCount >> 1);
	if (res) ErrorHandler_Fail2(res, "D3D9_DrawVb_Lines");
}

void Gfx_DrawVb_IndexedTris(Int32 verticesCount) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		0, 0, verticesCount, 0, verticesCount >> 1);
	if (res) ErrorHandler_Fail2(res, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawVb_IndexedTris_Range(Int32 verticesCount, Int32 startVertex) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	if (res) ErrorHandler_Fail2(res, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 verticesCount, Int32 startVertex) {
	ReturnCode res = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	if (res) ErrorHandler_Fail2(res, "D3D9_DrawIndexedVb_TrisT2fC4b");
}


void Gfx_SetMatrixMode(Int32 matrixType) {
	if (matrixType == MATRIX_TYPE_PROJECTION) {
		curMatrix = D3DTS_PROJECTION;
	} else if (matrixType == MATRIX_TYPE_VIEW) {
		curMatrix = D3DTS_VIEW;
	} else if (matrixType == MATRIX_TYPE_TEXTURE) {
		curMatrix = D3DTS_TEXTURE0;
	}
}

void Gfx_LoadMatrix(struct Matrix* matrix) {
	if (curMatrix == D3DTS_TEXTURE0) {
		matrix->Row2.X = matrix->Row3.X; /* NOTE: this hack fixes the texture movements. */
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	}

	if (Gfx_LostContext) return;
	ReturnCode res = IDirect3DDevice9_SetTransform(device, curMatrix, matrix);
	if (res) ErrorHandler_Fail2(res, "D3D9_LoadMatrix");
}

void Gfx_LoadIdentityMatrix(void) {
	if (curMatrix == D3DTS_TEXTURE0) {
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	}

	if (Gfx_LostContext) return;
	ReturnCode res = IDirect3DDevice9_SetTransform(device, curMatrix, &Matrix_Identity);
	if (res) ErrorHandler_Fail2(res, "D3D9_LoadIdentityMatrix");
}

#define d3d9_zN -10000.0f
#define d3d9_zF 10000.0f
void Gfx_CalcOrthoMatrix(float width, float height, struct Matrix* matrix) {
	Matrix_OrthographicOffCenter(matrix, 0.0f, width, height, 0.0f, d3d9_zN, d3d9_zF);
	matrix->Row2.Z = 1.0f    / (d3d9_zN - d3d9_zF);
	matrix->Row3.Z = d3d9_zN / (d3d9_zN - d3d9_zF);
}
void Gfx_CalcPerspectiveMatrix(float fov, float aspect, float zNear, float zFar, struct Matrix* matrix) {
	Matrix_PerspectiveFieldOfView(matrix, fov, aspect, zNear, zFar);
}


ReturnCode Gfx_TakeScreenshot(struct Stream* output, Int32 width, Int32 height) {
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
		res = Bitmap_EncodePng(&bmp, output);
		if (res) { IDirect3DSurface9_UnlockRect(temp); goto finished; }
	}
	res = IDirect3DSurface9_UnlockRect(temp);
	if (res) goto finished;

finished:
	D3D9_FreeResource(&backbuffer);
	D3D9_FreeResource(&temp);
	return res;
}

bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_SetVSync(bool value) {
	if (d3d9_vsync == value) return;
	d3d9_vsync = value;

	GfxCommon_LoseContext(" (toggling VSync)");
	D3D9_RecreateDevice();
}

void Gfx_BeginFrame(void) {
	IDirect3DDevice9_BeginScene(device);
}

void Gfx_EndFrame(void) {
	IDirect3DDevice9_EndScene(device);
	ReturnCode res = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
	if (!res) return;
	if (res != D3DERR_DEVICELOST) ErrorHandler_Fail2(res, "D3D9_EndFrame");

	/* TODO: Make sure this actually works on all graphics cards.*/
	GfxCommon_LoseContext(" (Direct3D9 device lost)");
	D3D9_LoopUntilRetrieved();
	D3D9_RecreateDevice();
}

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

float d3d9_totalMem;
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
	GfxCommon_LoseContext(" (resizing window)");
	D3D9_RecreateDevice();
}


static void D3D9_SetDefaultRenderStates(void) {
	Gfx_SetFaceCulling(false);
	d3d9_batchFormat = -1;
	D3D9_SetRenderState(D3DRS_COLORVERTEX,        false, "D3D9_ColorVertex");
	D3D9_SetRenderState2(D3DRS_LIGHTING,          false, "D3D9_Lighting");
	D3D9_SetRenderState2(D3DRS_SPECULARENABLE,    false, "D3D9_SpecularEnable");
	D3D9_SetRenderState2(D3DRS_LOCALVIEWER,       false, "D3D9_LocalViewer");
	D3D9_SetRenderState2(D3DRS_DEBUGMONITORTOKEN, false, "D3D9_DebugMonitor");
}

static void D3D9_RestoreRenderStates(void) {
	union IntAndFloat raw;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE, d3d9_alphaTest, "D3D9_AlphaTest");
	D3D9_SetRenderState2(D3DRS_ALPHABLENDENABLE, d3d9_alphaBlend, "D3D9_AlphaBlend");
	D3D9_SetRenderState2(D3DRS_ALPHAFUNC, d3d9_alphaTestFunc, "D3D9_AlphaTestFunc");
	D3D9_SetRenderState2(D3DRS_ALPHAREF, d3d9_alphaTestRef, "D3D9_AlphaRefFunc");
	D3D9_SetRenderState2(D3DRS_SRCBLEND, d3d9_srcBlendFunc, "D3D9_AlphaSrcBlend");
	D3D9_SetRenderState2(D3DRS_DESTBLEND, d3d9_dstBlendFunc, "D3D9_AlphaDstBlend");
	D3D9_SetRenderState2(D3DRS_FOGENABLE, d3d9_fogEnable, "D3D9_Fog");
	D3D9_SetRenderState2(D3DRS_FOGCOLOR, d3d9_fogCol, "D3D9_FogColor");
	raw.f = d3d9_fogDensity;
	D3D9_SetRenderState2(D3DRS_FOGDENSITY, raw.u, "D3D9_FogDensity");
	raw.f = d3d9_fogEnd;
	D3D9_SetRenderState2(D3DRS_FOGEND, raw.u, "D3D9_FogEnd");
	D3D9_SetRenderState2(D3DRS_FOGTABLEMODE, d3d9_fogTableMode, "D3D9_FogMode");
	D3D9_SetRenderState2(D3DRS_ZFUNC, d3d9_depthTestFunc, "D3D9_DepthTestFunc");
	D3D9_SetRenderState2(D3DRS_ZENABLE, d3d9_depthTest, "D3D9_DepthTest");
	D3D9_SetRenderState2(D3DRS_ZWRITEENABLE, d3d9_depthWrite, "D3D9_DepthWrite");
}
#endif
