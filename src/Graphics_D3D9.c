#include "Core.h"
#ifdef CC_BUILD_D3D9
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Logger.h"
#include "Window.h"

/* Avoid pointless includes */
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
/* NOTE: Direct3D9Ex backend was dropped on 2022-02-25 in favour of Direct3D11 */
#include <d3d9.h>
#include <d3d9caps.h>
#include <d3d9types.h>

/* https://docs.microsoft.com/en-us/windows/win32/dxtecharts/resource-management-best-practices */
/* https://docs.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline */

/* https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dfvf-texcoordsizen */
static DWORD d3d9_formatMappings[2] = { D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 };
/* Current format and size of vertices */
static int gfx_stride, gfx_format = -1;

static IDirect3D9* d3d;
static IDirect3DDevice9* device;
static DWORD createFlags;
static D3DFORMAT viewFormat, depthFormat;
static int cachedWidth, cachedHeight;
static int depthBits;
static float totalMem;
static cc_bool fallbackRendering;

static void D3D9_RestoreRenderStates(void);
static void D3D9_FreeResource(GfxResourceID* resource) {
	cc_uintptr addr;
	ULONG refCount;
	IUnknown* unk;
	
	unk = (IUnknown*)(*resource);
	if (!unk) return;
	*resource = 0;

#ifdef __cplusplus
	refCount = unk->Release();
#else
	refCount = unk->lpVtbl->Release(unk);
#endif

	if (refCount <= 0) return;
	addr = (cc_uintptr)unk;
	Platform_Log2("D3D9 resource has %i outstanding references! ID 0x%x", &refCount, &addr);
}

static IDirect3D9* (WINAPI *_Direct3DCreate9)(UINT SDKVersion);

static void LoadD3D9Library(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(Direct3DCreate9)
	};
	static const cc_string path = String_FromConst("d3d9.dll");
	void* lib;
	DynamicLib_LoadAll(&path, funcs, Array_Elems(funcs), &lib);

	if (lib) return;
	Logger_FailToStart("Failed to load d3d9.dll. You may need to install Direct3D9.");
}

static void CreateD3D9Instance(void) {
	d3d = _Direct3DCreate9(D3D_SDK_VERSION);

	/* Normal Direct3D9 supports POOL_MANAGED textures */
	/*  (Direct3D9Ex does not support them however) */
	Gfx.ManagedTextures = true;
	if (!d3d) Logger_Abort("Direct3DCreate9 returned NULL");

	fallbackRendering = Options_GetBool("fallback-rendering", false);
	if (!fallbackRendering) return;
	Platform_LogConst("WARNING: Using fallback rendering mode, which will reduce performance");
}

static void FindCompatibleViewFormat(void) {
	static const D3DFORMAT formats[4] = { D3DFMT_X8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 };
	cc_result res;
	int i;

	for (i = 0; i < Array_Elems(formats); i++) {
		viewFormat = formats[i];
		res = IDirect3D9_CheckDeviceType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, viewFormat, viewFormat, true);
		if (!res) return;
	}
	Logger_FailToStart("Failed to create back buffer. Graphics drivers may not be installed.\n\nIf that still does not work, try the OpenGL build instead");
}

static void FindCompatibleDepthFormat(void) {
	static const D3DFORMAT formats[6] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1 };
	cc_result res;
	int i;

	for (i = 0; i < Array_Elems(formats); i++) {
		depthFormat = formats[i];
		res = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, viewFormat, viewFormat, depthFormat);
		if (!res) return;
	}
	Logger_FailToStart("Failed to create depth buffer. Graphics drivers may not be installed.\n\nIf that still does not work, try the OpenGL build instead");
}

static void D3D9_FillPresentArgs(D3DPRESENT_PARAMETERS* args) {
	args->AutoDepthStencilFormat = depthFormat;
	args->BackBufferWidth  = Game.Width;
	args->BackBufferHeight = Game.Height;
	args->BackBufferFormat = viewFormat;
	args->BackBufferCount  = 1;
	args->EnableAutoDepthStencil = true;
	args->PresentationInterval   = gfx_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	args->SwapEffect = D3DSWAPEFFECT_DISCARD;
	args->Windowed   = true;
	//args->MultiSampleType = D3DMULTISAMPLE_8_SAMPLES;
}

static const int D3D9_DepthBufferBits(void) {
	switch (depthFormat) {
	case D3DFMT_D32:     return 32;
	case D3DFMT_D24X8:   return 24;
	case D3DFMT_D24S8:   return 24;
	case D3DFMT_D24X4S4: return 24;
	case D3DFMT_D16:     return 16;
	case D3DFMT_D15S1:   return 15;
	}
	return 0;
}

static void D3D9_UpdateCachedDimensions(void) {
	cachedWidth  = Game.Width;
	cachedHeight = Game.Height;
}

static cc_bool deviceCreated;
static void TryCreateDevice(void) {
	cc_result res;
	D3DCAPS9 caps;
	HWND winHandle = (HWND)WindowInfo.Handle;
	D3DPRESENT_PARAMETERS args = { 0 };
	D3D9_FillPresentArgs(&args);

	/* Try to create a device with as much hardware usage as possible. */
	createFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	/* Another running fullscreen application might prevent creating device */
	if (res == D3DERR_DEVICELOST) { Gfx.LostContext = true; return; }

	/* Fallback with using CPU for some parts of rendering */
	if (res) {
		createFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}
	if (res) {
		createFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}

	/* Not enough memory? Try again later in a bit */
	if (res == D3DERR_OUTOFVIDEOMEMORY) { Gfx.LostContext = true; return; }

	if (res) Logger_Abort2(res, "Creating Direct3D9 device");
	res = IDirect3DDevice9_GetDeviceCaps(device, &caps);
	if (res) Logger_Abort2(res, "Getting Direct3D9 capabilities");

	D3D9_UpdateCachedDimensions();
	deviceCreated    = true;
	Gfx.MaxTexWidth  = caps.MaxTextureWidth;
	Gfx.MaxTexHeight = caps.MaxTextureHeight;
	totalMem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);
}

void Gfx_Create(void) {
	LoadD3D9Library();
	CreateD3D9Instance();
	FindCompatibleViewFormat();
	FindCompatibleDepthFormat();
	depthBits = D3D9_DepthBufferBits();

	customMipmapsLevels = true;
	Gfx.Created         = true;
	TryCreateDevice();
}

cc_bool Gfx_TryRestoreContext(void) {
	static int availFails;
	D3DPRESENT_PARAMETERS args = { 0 };
	cc_result res;

	/* Rarely can't even create device to begin with */
	if (!deviceCreated) {
		TryCreateDevice();
		return deviceCreated;
	}

	res = IDirect3DDevice9_TestCooperativeLevel(device);
	if (res && res != D3DERR_DEVICENOTRESET) return false;

	D3D9_FillPresentArgs(&args);
	res = IDirect3DDevice9_Reset(device, &args);
	if (res == D3DERR_DEVICELOST) return false;

	/* A user reported an issue where after changing some settings in */
	/*  nvidia control panel, IDirect3DDevice9_Reset would return */
	/*  D3DERR_NOTAVAILABLE and hence crash the game */
	/* So try to workaround this by only crashing after 50 failures */
	if (res == D3DERR_NOTAVAILABLE && availFails++ < 50) return false;

	if (res) Logger_Abort2(res, "Error recreating D3D9 context");
	D3D9_UpdateCachedDimensions();
	return true;
}

void Gfx_Free(void) {
	Gfx_FreeState();
	D3D9_FreeResource(&device);
	D3D9_FreeResource(&d3d);
}

static void Gfx_FreeState(void) { 
	FreeDefaultResources();
	cachedWidth  = 0;
	cachedHeight = 0;
}

static void Gfx_RestoreState(void) {
	Gfx_SetFaceCulling(false);
	InitDefaultResources();
	gfx_format = -1;

	IDirect3DDevice9_SetRenderState(device, D3DRS_COLORVERTEX,       false);
	IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING,          false);
	IDirect3DDevice9_SetRenderState(device, D3DRS_SPECULARENABLE,    false);
	IDirect3DDevice9_SetRenderState(device, D3DRS_LOCALVIEWER,       false);
	IDirect3DDevice9_SetRenderState(device, D3DRS_DEBUGMONITORTOKEN, false);

	/* States relevant to the game */
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAREF,  127);
	IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
	IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC,     D3DCMP_GREATEREQUAL);
	D3D9_RestoreRenderStates();
}

static cc_bool D3D9_CheckResult(cc_result res, const char* func) {
	if (!res) return true;

	if (res == D3DERR_OUTOFVIDEOMEMORY || res == E_OUTOFMEMORY) {
		if (!Game_ReduceVRAM()) Logger_Abort("Out of video memory!");
	} else {
		Logger_Abort2(res, func);
	}
	return false;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void D3D9_SetTextureData(IDirect3DTexture9* texture, struct Bitmap* bmp, int lvl) {
	D3DLOCKED_RECT rect;
	cc_result res = IDirect3DTexture9_LockRect(texture, lvl, &rect, NULL, 0);
	if (res) Logger_Abort2(res, "D3D9_LockTextureData");

	cc_uint32 size = Bitmap_DataSize(bmp->width, bmp->height);
	Mem_Copy(rect.pBits, bmp->scan0, size);

	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) Logger_Abort2(res, "D3D9_UnlockTextureData");
}

static void D3D9_SetTexturePartData(IDirect3DTexture9* texture, int x, int y, const struct Bitmap* bmp, int rowWidth, int lvl) {
	D3DLOCKED_RECT rect;
	cc_result res;
	RECT part;

	part.left = x; part.right  = x + bmp->width;
	part.top  = y; part.bottom = y + bmp->height;

	res = IDirect3DTexture9_LockRect(texture, lvl, &rect, &part, 0);
	if (res) Logger_Abort2(res, "D3D9_LockTexturePartData");

	CopyTextureData(rect.pBits, rect.Pitch, bmp, rowWidth << 2);
	res = IDirect3DTexture9_UnlockRect(texture, lvl);
	if (res) Logger_Abort2(res, "D3D9_UnlockTexturePartData");
}

static void D3D9_DoMipmaps(IDirect3DTexture9* texture, int x, int y, struct Bitmap* bmp, int rowWidth, cc_bool partial) {
	BitmapCol* prev = bmp->scan0;
	BitmapCol* cur;
	struct Bitmap mipmap;

	int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
	int lvl, width = bmp->width, height = bmp->height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)  width /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, 4, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);

		Bitmap_Init(mipmap, width, height, cur);
		if (partial) {
			D3D9_SetTexturePartData(texture, x, y, &mipmap, width, lvl);
		} else {
			D3D9_SetTextureData(texture, &mipmap, lvl);
		}

		if (prev != bmp->scan0) Mem_Free(prev);
		prev     = cur;
		rowWidth = width;
	}
	if (prev != bmp->scan0) Mem_Free(prev);
}

static IDirect3DTexture9* DoCreateTexture(struct Bitmap* bmp, int levels, int pool) {
	IDirect3DTexture9* tex;
	cc_result res;
	
	for (;;) {
		res = IDirect3DDevice9_CreateTexture(device, bmp->width, bmp->height, levels,
			0, D3DFMT_A8R8G8B8, pool, &tex, NULL);
		if (D3D9_CheckResult(res, "D3D9_CreateTexture failed")) break;
	}
	return tex;
}

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	IDirect3DTexture9* tex;
	IDirect3DTexture9* sys;
	cc_result res;

	int mipmapsLevels = CalcMipmapsLevels(bmp->width, bmp->height);
	int levels = 1 + (mipmaps ? mipmapsLevels : 0);

	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}
	if (Gfx.LostContext) return 0;

	if (flags & TEXTURE_FLAG_MANAGED) {
		while ((res = IDirect3DDevice9_CreateTexture(device, bmp->width, bmp->height, levels,
				0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL))) 
		{
			if (res == D3DERR_OUTOFVIDEOMEMORY || res == E_OUTOFMEMORY) {
				/* insufficient VRAM or RAM left to allocate texture, try to reduce the memory in use first */
				/*  if can't reduce, return 'empty' texture so that at least the game will continue running */
				if (!Game_ReduceVRAM()) return 0;
			} else {
				/* unknown issue, so don't even try to handle the error */
				Logger_Abort2(res, "D3D9_CreateManagedTexture failed");
			}
		}

		D3D9_SetTextureData(tex, bmp, 0);
		if (mipmaps) D3D9_DoMipmaps(tex, 0, 0, bmp, bmp->width, false);
		return tex;
	}

	sys = DoCreateTexture(bmp, levels, D3DPOOL_SYSTEMMEM);
	D3D9_SetTextureData(sys, bmp, 0);
	if (mipmaps) D3D9_DoMipmaps(sys, 0, 0, bmp, bmp->width, false);
		
	tex = DoCreateTexture(bmp, levels, D3DPOOL_DEFAULT);
	res = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)sys, (IDirect3DBaseTexture9*)tex);
	if (res) Logger_Abort2(res, "D3D9_CreateTexture - Update");

	D3D9_FreeResource(&sys);
	return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	IDirect3DTexture9* texture = (IDirect3DTexture9*)texId;
	D3D9_SetTexturePartData(texture, x, y, part, rowWidth, 0);
	if (mipmaps) D3D9_DoMipmaps(texture, x, y, part, rowWidth, true);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_BindTexture(GfxResourceID texId) {
	cc_result res = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9*)texId);
	if (res) Logger_Abort2(res, "D3D9_BindTexture");
}

void Gfx_DeleteTexture(GfxResourceID* texId) { D3D9_FreeResource(texId); }

void Gfx_SetTexturing(cc_bool enabled) { }

void Gfx_EnableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
}

void Gfx_DisableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static D3DFOGMODE gfx_fogMode = D3DFOG_NONE;
static cc_bool gfx_alphaTesting, gfx_alphaBlending;
static cc_bool gfx_depthTesting, gfx_depthWriting;
static PackedCol gfx_clearColor, gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;

/* NOTE: Although SetRenderState is okay to call on a lost device, it's also possible */
/*   the context is lost because the device was never created to begin with!          */
/* In that case, device will be NULL, so calling SetRenderState will crash the game.  */
/*  (see Gfx_Create, TryCreateDevice, Gfx_TryRestoreContext)                          */

void Gfx_SetFaceCulling(cc_bool enabled) {
	D3DCULL mode = enabled ? D3DCULL_CW : D3DCULL_NONE;
	IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, mode);
}

void Gfx_SetFog(cc_bool enabled) {
	if (gfx_fogEnabled == enabled) return;
	gfx_fogEnabled = enabled;

	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, enabled);
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;

	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, gfx_fogColor);
}

void Gfx_SetFogDensity(float value) {
	union IntAndFloat raw;
	if (value == gfx_fogDensity) return;
	gfx_fogDensity = value;

	raw.f = value;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGDENSITY, raw.u);
}

void Gfx_SetFogEnd(float value) {
	union IntAndFloat raw;
	if (value == gfx_fogEnd) return;
	gfx_fogEnd = value;

	raw.f = value;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, raw.u);
}

void Gfx_SetFogMode(FogFunc func) {
	static D3DFOGMODE modes[3] = { D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2 };
	D3DFOGMODE mode = modes[func];
	if (mode == gfx_fogMode) return;

	gfx_fogMode = mode;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, mode);
}

void Gfx_SetAlphaTest(cc_bool enabled) {
	if (gfx_alphaTesting == enabled) return;
	gfx_alphaTesting = enabled;

	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, enabled);
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	if (gfx_alphaBlending == enabled) return;
	gfx_alphaBlending = enabled;

	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, enabled);
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
}

void Gfx_ClearCol(PackedCol color) { gfx_clearColor = color; }
void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	DWORD channels = (r ? 1u : 0u) | (g ? 2u : 0u) | (b ? 4u : 0u) | (a ? 8u : 0u);
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, channels);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	gfx_depthTesting = enabled;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, enabled);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	gfx_depthWriting = enabled;
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, enabled);
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
	if (depthOnly) IDirect3DDevice9_SetTexture(device, 0, NULL);

	/* For when Direct3D9 device doesn't support D3DRS_COLORWRITEENABLE */
	/*  Technically, the correct way to check for whether it works or not */
	/*  is by checking whether the PrimitiveMiscCaps field in D3DCAPS9 */
	/*  has the D3DPMISCCAPS_COLORWRITEENABLE flag set */
	/* But since I'm unsure if there might be some GPU drivers out there */
	/*  that do support D3DRS_COLORWRITEENABLE but forget to set the flag, */
	/*  I've decided to require the user to manually enable this fallback */
	if (!fallbackRendering) return;

	/* https://gamedev.net/forums/topic/375017-c-enabling-disabling-writing-to-buffers/3473819/ */
	if (depthOnly) {
		/* finalX = srcX*0 + dstX*1 */
		/*  So in other words, final pixel = existing pixel */
		/*  Pretty costly performance wise though */
		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND,  D3DBLEND_ZERO);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ONE);
		Gfx_SetAlphaBlending(true);
	} else {
		/* finalX = srcX*srcA + dstX*(1-srcA) */
		IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
}

static void D3D9_RestoreRenderStates(void) {
	union IntAndFloat raw;
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE,   gfx_alphaTesting);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, gfx_alphaBlending);

	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, gfx_fogEnabled);
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR,  gfx_fogColor);
	raw.f = gfx_fogDensity;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGDENSITY, raw.u);
	raw.f = gfx_fogEnd;
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, raw.u);
	IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, gfx_fogMode);

	IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE,      gfx_depthTesting);
	IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, gfx_depthWriting);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static void D3D9_SetIbData(IDirect3DIndexBuffer9* buffer, int count, Gfx_FillIBFunc fillFunc, void* obj) {
	void* dst = NULL;
	cc_result res = IDirect3DIndexBuffer9_Lock(buffer, 0, count * 2, &dst, 0);
	if (res) Logger_Abort2(res, "D3D9_LockIb");

	fillFunc((cc_uint16*)dst, count, obj);
	res = IDirect3DIndexBuffer9_Unlock(buffer);
	if (res) Logger_Abort2(res, "D3D9_UnlockIb");
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	int size = count * 2;
	IDirect3DIndexBuffer9* ibuffer;
	cc_result res = IDirect3DDevice9_CreateIndexBuffer(device, size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ibuffer, NULL);
	if (res) Logger_Abort2(res, "D3D9_CreateIb");

	D3D9_SetIbData(ibuffer, count, fillFunc, obj);
	return ibuffer;
}

void Gfx_BindIb(GfxResourceID ib) {
	IDirect3DIndexBuffer9* ibuffer = (IDirect3DIndexBuffer9*)ib;
	cc_result res = IDirect3DDevice9_SetIndices(device, ibuffer);
	if (res) Logger_Abort2(res, "D3D9_BindIb");
}

void Gfx_DeleteIb(GfxResourceID* ib) { D3D9_FreeResource(ib); }


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static IDirect3DVertexBuffer9* D3D9_AllocVertexBuffer(VertexFormat fmt, int count, DWORD usage) {
	IDirect3DVertexBuffer9* vbuffer;
	cc_result res;
	int size = count * strideSizes[fmt];

	for (;;) {
		res = IDirect3DDevice9_CreateVertexBuffer(device, size, usage,
					d3d9_formatMappings[fmt], D3DPOOL_DEFAULT, &vbuffer, NULL);
		if (D3D9_CheckResult(res, "D3D9_CreateVb failed")) break;
	}
	return vbuffer;
}

static void D3D9_SetVbData(IDirect3DVertexBuffer9* buffer, void* data, int size, int lockFlags) {
	void* dst = NULL;
	cc_result res = IDirect3DVertexBuffer9_Lock(buffer, 0, size, &dst, lockFlags);
	if (res) Logger_Abort2(res, "D3D9_LockVb");

	Mem_Copy(dst, data, size);
	res = IDirect3DVertexBuffer9_Unlock(buffer);
	if (res) Logger_Abort2(res, "D3D9_UnlockVb");
}

static void* D3D9_LockVb(GfxResourceID vb, VertexFormat fmt, int count, int lockFlags) {
	IDirect3DVertexBuffer9* buffer = (IDirect3DVertexBuffer9*)vb;
	void* dst = NULL;
	int size  = count * strideSizes[fmt];

	cc_result res = IDirect3DVertexBuffer9_Lock(buffer, 0, size, &dst, lockFlags);
	if (res) Logger_Abort2(res, "D3D9_LockVb");
	return dst;
}

GfxResourceID Gfx_CreateVb(VertexFormat fmt, int count) {
	return D3D9_AllocVertexBuffer(fmt, count, D3DUSAGE_WRITEONLY);
}

void Gfx_BindVb(GfxResourceID vb) {
	IDirect3DVertexBuffer9* vbuffer = (IDirect3DVertexBuffer9*)vb;
	cc_result res = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, gfx_stride);
	if (res) Logger_Abort2(res, "D3D9_BindVb");
}

void Gfx_DeleteVb(GfxResourceID* vb) { D3D9_FreeResource(vb); }
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return D3D9_LockVb(vb, fmt, count, 0);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	IDirect3DVertexBuffer9* buffer = (IDirect3DVertexBuffer9*)vb;
	cc_result res = IDirect3DVertexBuffer9_Unlock(buffer);
	if (res) Logger_Abort2(res, "Gfx_UnlockVb");
}


void Gfx_SetVertexFormat(VertexFormat fmt) {
	cc_result res;
	if (fmt == gfx_format) return;
	gfx_format = fmt;

	if (fmt == VERTEX_FORMAT_COLOURED) {
		/* it's necessary to unbind the texture, otherwise the alpha from the last bound texture */
		/*  gets used - because D3DTSS_ALPHAOP texture stage state is still set to D3DTOP_SELECTARG1 */
		IDirect3DDevice9_SetTexture(device, 0, NULL);
		/*  IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, fmt == VERTEX_FORMAT_COLOURED ? D3DTOP_DISABLE : D3DTOP_MODULATE); */
		/*  IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, fmt == VERTEX_FORMAT_COLOURED ? D3DTOP_DISABLE : D3DTOP_SELECTARG1); */
		/* SetTexture(NULL) seems to be enough, not really required to call SetTextureStageState */
	}

	res = IDirect3DDevice9_SetFVF(device, d3d9_formatMappings[fmt]);
	if (res) Logger_Abort2(res, "D3D9_SetVertexFormat");
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {
	/* NOTE: Skip checking return result for Gfx_DrawXYZ for performance */
	IDirect3DDevice9_DrawPrimitive(device, D3DPT_LINELIST, 0, verticesCount >> 1);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		0, 0, verticesCount, 0, verticesCount >> 1);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateDynamicVb(VertexFormat fmt, int maxVertices) {
	if (Gfx.LostContext) return 0;
	return D3D9_AllocVertexBuffer(fmt, maxVertices, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY);
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return D3D9_LockVb(vb, fmt, count, D3DLOCK_DISCARD);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	Gfx_UnlockVb(vb);
	Gfx_BindVb(vb); /* TODO: Inline this? */
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	int size = vCount * gfx_stride;
	IDirect3DVertexBuffer9* buffer = (IDirect3DVertexBuffer9*)vb;
	D3D9_SetVbData(buffer, vertices, size, D3DLOCK_DISCARD);

	cc_result res = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, gfx_stride);
	if (res) Logger_Abort2(res, "D3D9_SetDynamicVbData - Bind");
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static D3DTRANSFORMSTATETYPE matrix_modes[2] = { D3DTS_PROJECTION, D3DTS_VIEW };

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetTransform(device, matrix_modes[type], (const D3DMATRIX*)matrix);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetTransform(device, matrix_modes[type], (const D3DMATRIX*)&Matrix_Identity);
}

static struct Matrix texMatrix = Matrix_IdentityValue;
void Gfx_EnableTextureOffset(float x, float y) {
	texMatrix.row3.X = x; texMatrix.row3.Y = y;
	if (Gfx.LostContext) return;

	IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (const D3DMATRIX*)&texMatrix);
}

void Gfx_DisableTextureOffset(void) {
	if (Gfx.LostContext) return;
	IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (const D3DMATRIX*)&Matrix_Identity);
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z =  1.0f / (zNear - zFar);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = zNear / (zNear - zFar);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	/* Deliberately swap zNear/zFar in projection matrix calculation to produce */
	/*  a projection matrix that results in a reversed depth buffer */
	/* https://developer.nvidia.com/content/depth-precision-visualized */
	float zNear_ = zFar;
	float zFar_  = Reversed_CalcZNear(fov, depthBits);

	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = zFar_ / (zNear_ - zFar_);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = (zNear_ * zFar_) / (zNear_ - zFar_);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	IDirect3DSurface9* backbuffer = NULL;
	IDirect3DSurface9* temp = NULL;
	D3DSURFACE_DESC desc;
	D3DLOCKED_RECT rect;
	struct Bitmap bmp;
	cc_result res;

	res = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	if (res) goto finished;
	res = IDirect3DSurface9_GetDesc(backbuffer, &desc);
	if (res) goto finished;

	res = IDirect3DDevice9_CreateOffscreenPlainSurface(device, desc.Width, desc.Height, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &temp, NULL);
	if (res) goto finished; /* TODO: For DX 8 use IDirect3DDevice8::CreateImageSurface */
	res = IDirect3DDevice9_GetRenderTargetData(device, backbuffer, temp);
	if (res) goto finished;
	
	res = IDirect3DSurface9_LockRect(temp, &rect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
	if (res) goto finished;
	{
		Bitmap_Init(bmp, desc.Width, desc.Height, (BitmapCol*)rect.pBits);
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

static void UpdateSwapchain(const char* reason) {
	/* TODO: Can Direct3D9Ex fast path still be used here? */
	Gfx_LoseContext(reason);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	if (gfx_vsync == vsync) return;

	gfx_vsync = vsync;
	if (device) UpdateSwapchain(" (toggling VSync)");
}

void Gfx_BeginFrame(void) { 
	IDirect3DDevice9_BeginScene(device);
}

void Gfx_Clear(void) {
	DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	cc_result res = IDirect3DDevice9_Clear(device, 0, NULL, flags, gfx_clearColor, 0.0f, 0);
	if (res) Logger_Abort2(res, "D3D9_Clear");
}

void Gfx_EndFrame(void) {
	IDirect3DDevice9_EndScene(device);
	cc_result res = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

	if (res) {
		if (res != D3DERR_DEVICELOST) Logger_Abort2(res, "D3D9_EndFrame");
		/* TODO: Make sure this actually works on all graphics cards. */
		Gfx_LoseContext(" (Direct3D9 device lost)");
	}
	if (gfx_minFrameMs) LimitFPS();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
static const char* D3D9_StrFlags(void) {
	if (createFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) return "Hardware";
	if (createFlags & D3DCREATE_MIXED_VERTEXPROCESSING)    return "Mixed";
	if (createFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) return "Software";
	return "(none)";
}

void Gfx_GetApiInfo(cc_string* info) {
	D3DADAPTER_IDENTIFIER9 adapter = { 0 };
	int pointerSize = sizeof(void*) * 8;
	float curMem;

	IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &adapter);
	curMem = IDirect3DDevice9_GetAvailableTextureMem(device) / (1024.0f * 1024.0f);
	String_Format1(info, "-- Using Direct3D9 (%i bit) --\n", &pointerSize);

	String_Format1(info, "Adapter: %c\n",         adapter.Description);
	String_Format1(info, "Processing mode: %c\n", D3D9_StrFlags());
	String_Format2(info, "Video memory: %f2 MB total, %f2 free\n", &totalMem, &curMem);
	String_Format2(info, "Max texture size: (%i x %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	String_Format1(info, "Depth buffer bits: %i", &depthBits);
}

void Gfx_OnWindowResize(void) {
	if (Game.Width == cachedWidth && Game.Height == cachedHeight) return;
	/* Only resize when necessary */
	UpdateSwapchain(" (resizing window)");
}
#endif
