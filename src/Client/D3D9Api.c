#include "GraphicsAPI.h"
#include "ErrorHandler.h"
#include "GraphicsEnums.h"
#include "Platform.h"
#include "Window.h"
#include "GraphicsCommon.h"
#include "Funcs.h"

#ifdef USE_DX
//#define D3D_DISABLE_9EX causes compile errors
#include <d3d9.h>
#include <d3d9caps.h>
#include <d3d9types.h>

/* Maximum number of matrices that go on a stack. */
#define MatrixStack_Capacity 4
typedef struct MatrixStack_ {
	/* Raw array of matrices.*/
	Matrix Stack[MatrixStack_Capacity];
	/* Current active matrix. */
	Int32 Index;
	/* Type of transformation this stack is for. */
	D3DTRANSFORMSTATETYPE Type;
} MatrixStack;


D3DFORMAT d3d9_depthFormats[6] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D15S1 };
D3DFORMAT d3d9_viewFormats[4] = { D3DFMT_X8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 };
D3DBLEND d3d9_blendFuncs[6] = { D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_INVDESTALPHA };
D3DCMPFUNC d3d9_compareFuncs[8] = { D3DCMP_ALWAYS, D3DCMP_NOTEQUAL, D3DCMP_NEVER, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER };
D3DFOGMODE d3d9_modes[3] = { D3DFOG_LINEAR, D3DFOG_EXP, D3DFOG_EXP2 };
Int32 d3d9_formatMappings[2] = { D3DFVF_XYZ | D3DFVF_DIFFUSE, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 };

bool d3d9_vsync;
IDirect3D9* d3d;
IDirect3DDevice9* device;
MatrixStack* curStack;
MatrixStack viewStack, projStack, texStack;
DWORD createFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
D3DFORMAT d3d9_viewFormat, d3d9_depthFormat;

/* We only ever create a single index buffer internally. */
#define d3d9_iBuffersExpSize 2
IDirect3DIndexBuffer9* d3d9_default_iBuffers[d3d9_iBuffersExpSize];
Int32 d3d9_ibuffersCapacity = d3d9_iBuffersExpSize;
IDirect3DIndexBuffer9** d3d9_iBuffers;

/* TODO: This number's probably not big enough... */
#define d3d9_vBuffersExpSize 2048
IDirect3DVertexBuffer9* d3d9_default_vBuffers[d3d9_vBuffersExpSize];
Int32 d3d9_vbuffersCapacity = d3d9_vBuffersExpSize;
IDirect3DVertexBuffer9** d3d9_vBuffers;

/* At most we can have 256 entities with their own texture each.
Adding another 128 gives us a lot of leeway. */
#define d3d9_texturesExpSize 384
IDirect3DTexture9* d3d9_default_textures[d3d9_texturesExpSize];
Int32 d3d9_texturesCapacity = d3d9_texturesExpSize;
IDirect3DTexture9** d3d9_textures;


#define D3D9_SetRenderState(raw, state, name) \
ReturnCode hresult = IDirect3DDevice9_SetRenderState(device, state, raw); \
ErrorHandler_CheckOrFail(hresult, name)

#define D3D9_SetRenderState2(raw, state, name) \
hresult = IDirect3DDevice9_SetRenderState(device, state, raw); \
ErrorHandler_CheckOrFail(hresult, name)

#define D3D9_LogLeakedResource(msg, i) \
logMsg.length = 0;\
String_AppendConstant(&logMsg, msg);\
String_AppendInt32(&logMsg, i);\
Platform_Log(logMsg);


/* Forward declarations for these two functions. */
static void D3D9_SetDefaultRenderStates(void);
static void D3D9_RestoreRenderStates(void);

void D3D9_FreeResource(void* resource, GfxResourceID id) {
	IUnknown* unk = (IUnknown*)resource;
	UInt32 refCount = unk->lpVtbl->Release(unk);
	if (refCount <= 0) return;

	UInt8 logMsgBuffer[String_BufferSize(127)];
	String logMsg = String_FromRawBuffer(logMsgBuffer, 127);
	String_AppendConstant(&logMsg, "D3D9 Resource has outstanding references! ID: ");
	String_AppendInt32(&logMsg, id);
	Platform_Log(logMsg);
}

void D3D9_DeleteResource(void** resources, Int32 capacity, GfxResourceID* id) {
	GfxResourceID resourceID = *id;
	if (resourceID <= 0 || resourceID >= capacity) return;

	void* value = resources[resourceID];
	*id = -1;
	if (value == NULL) return;

	resources[resourceID] = NULL;
	D3D9_FreeResource(value, resourceID);
}

/* TODO: I have no clue if this even works. */
GfxResourceID D3D9_GetOrExpand(void*** resourcesPtr, Int32* capacity, void* resource, Int32 expSize) {
	GfxResourceID i;
	void** resources = *resourcesPtr;
	for (i = 1; i < *capacity; i++) {
		if (resources[i] == NULL) {
			resources[i] = resource;
			return i;
		}
	}

	/* Otherwise resize and add more elements */
	GfxResourceID oldLength = *capacity;
	(*capacity) += expSize;

	/* Allocate resized pointers table */
	void** newResources = Platform_MemAlloc(*capacity * sizeof(void*));
	if (newResources == NULL) {
		ErrorHandler_Fail("D3D9 - failed to resize pointers table");
	}
	*resourcesPtr = newResources;

	/* Update elements in new table */
	for (i = 0; i < oldLength; i++) {
		newResources[i] = resources[i];
	}
	/* Free old allocated memory if necessary */
	if (oldLength != expSize) Platform_MemFree(resources);

	newResources[oldLength] = resource;
	return oldLength;
}

void D3D9_LoopUntilRetrieved(void) {
	ScheduledTask task;
	task.Interval = 1.0f / 60.0f;
	task.Callback = LostContextFunction;

	while (true) {
		Platform_ThreadSleep(16);
		ReturnCode code = IDirect3DDevice9_TestCooperativeLevel(device);
		if (code == D3DERR_DEVICENOTRESET) return;

		task.Callback(&task);
	}
}

void D3D9_FindCompatibleFormat(void) {
	Int32 count = Array_NumElements(d3d9_viewFormats);
	Int32 i;
	ReturnCode res;

	for (i = 0; i < count; i++) {
		d3d9_viewFormat = d3d9_viewFormats[i];
		res = IDirect3D9_CheckDeviceType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, true);
		if (ErrorHandler_Check(res)) break;

		if (i == count - 1) {
			ErrorHandler_Fail("Unable to create a back buffer with sufficient precision.");
		}
	}

	count = Array_NumElements(d3d9_depthFormats);
	for (i = 0; i < count; i++) {
		d3d9_depthFormat = d3d9_depthFormats[i];
		res = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3d9_viewFormat, d3d9_viewFormat, d3d9_depthFormat);
		if (ErrorHandler_Check(res)) break;

		if (i == count - 1) {
			ErrorHandler_Fail("Unable to create a depth buffer with sufficient precision.");
		}
	}
}

void D3D9_GetPresentArgs(Int32 width, Int32 height, D3DPRESENT_PARAMETERS* args) {
	Platform_MemSet(args, 0, sizeof(D3DPRESENT_PARAMETERS));
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

void D3D9_RecreateDevice(void) {
	D3DPRESENT_PARAMETERS args;
	Size2D size = Window_GetClientSize();
	D3D9_GetPresentArgs(size.Width, size.Height, &args);

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
	D3DPRESENT_PARAMETERS args;
	D3D9_GetPresentArgs(640, 480, &args);
	ReturnCode res;

	d3d9_iBuffers = d3d9_default_iBuffers;
	d3d9_vBuffers = d3d9_default_vBuffers;
	d3d9_textures = d3d9_default_textures;

	/* Try to create a device with as much hardware usage as possible. */
	res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	if (!ErrorHandler_Check(res)) {
		createFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}
	if (!ErrorHandler_Check(res)) {
		createFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		res = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, winHandle, createFlags, &args, &device);
	}
	ErrorHandler_CheckOrFail(res, "Creating Direct3D9 device");

	D3DCAPS9 caps;
	Platform_MemSet(&caps, 0, sizeof(D3DCAPS9));
	IDirect3DDevice9_GetDeviceCaps(device, &caps);

	Gfx_CustomMipmapsLevels = true;
	viewStack.Type = D3DTS_VIEW;
	projStack.Type = D3DTS_PROJECTION;
	texStack.Type = D3DTS_TEXTURE0;
	D3D9_SetDefaultRenderStates();
	GfxCommon_Init();
}

void Gfx_Free(void) {
	UInt8 logMsgBuffer[String_BufferSize(63)];
	String logMsg = String_FromRawBuffer(logMsgBuffer, 63);
	GfxCommon_Free();

	Int32 i;
	for (i = 0; i < d3d9_texturesCapacity; i++) {
		if (d3d9_textures[i] == NULL) continue;
		GfxResourceID texId = i;
		Gfx_DeleteTexture(&texId);
		D3D9_LogLeakedResource("Texture leak! ID: ", i);
	}

	for (i = 0; i < d3d9_vbuffersCapacity; i++) {
		if (d3d9_vBuffers[i] == NULL) continue;
		GfxResourceID vb = i;
		Gfx_DeleteVb(&vb);
		D3D9_LogLeakedResource("Vertex buffer leak! ID: ", i);
	}

	for (i = 0; i < d3d9_ibuffersCapacity; i++) {
		if (d3d9_iBuffers[i] == NULL) continue;
		GfxResourceID ib = i;
		Gfx_DeleteIb(&ib);
		D3D9_LogLeakedResource("Index buffer leak! ID: ", i);
	}

	if (d3d9_ibuffersCapacity != d3d9_iBuffersExpSize) {
		Platform_MemFree(d3d9_iBuffers);
	}
	if (d3d9_vbuffersCapacity != d3d9_vBuffersExpSize) {
		Platform_MemFree(d3d9_vBuffers);
	}
	if (d3d9_texturesCapacity != d3d9_texturesExpSize) {
		Platform_MemFree(d3d9_textures);
	}
}


void D3D9_SetTextureData(IDirect3DTexture9* texture, Bitmap* bmp, Int32 lvl) {
	D3DLOCKED_RECT rect;
	ReturnCode hresult = IDirect3DTexture9_LockRect(texture, lvl, &rect, NULL, 0);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTextureData - Lock");

	UInt32 size = Bitmap_DataSize(bmp->Width, bmp->Height);
	Platform_MemCpy(rect.pBits, bmp->Scan0, size);

	hresult = IDirect3DTexture9_UnlockRect(texture, lvl);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTextureData - Unlock");
}

void D3D9_SetTexturePartData(IDirect3DTexture9* texture, Int32 x, Int32 y, Bitmap* bmp, Int32 lvl) {
	RECT part;
	part.left = x; part.right = x + bmp->Width;
	part.top = y; part.bottom = y + bmp->Height;

	D3DLOCKED_RECT rect;
	ReturnCode hresult = IDirect3DTexture9_LockRect(texture, lvl, &rect, &part, 0);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTexturePartData - Lock");

	/* We need to copy scanline by scanline, as generally rect.stride != data.stride */
	UInt8* src = (UInt8*)bmp->Scan0;
	UInt8* dst = (UInt8*)rect.pBits;
	Int32 yy;

	for (yy = 0; yy < bmp->Height; yy++) {
		Platform_MemCpy(src, dst, bmp->Width * Bitmap_PixelBytesSize);
		src += bmp->Width * Bitmap_PixelBytesSize;
		dst += rect.Pitch;
	}

	hresult = IDirect3DTexture9_UnlockRect(texture, lvl);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTexturePartData - Unlock");
}

void D3D9_DoMipmaps(IDirect3DTexture9* texture, Int32 x, Int32 y, Bitmap* bmp, bool partial) {
	UInt8* prev = bmp->Scan0;
	Int32 lvls = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	Int32 lvl, width = bmp->Width, height = bmp->Height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2; 
		if (width > 1)   width /= 2; 
		if (height > 1) height /= 2;
		UInt32 size = Bitmap_DataSize(width, height);

		UInt8* cur = Platform_MemAlloc(size);
		if (cur == NULL) {
			ErrorHandler_Fail("Allocating memory for mipmaps");
		}
		GfxCommon_GenMipmaps(width, height, cur, prev);

		Bitmap mipmap;
		Bitmap_Create(&mipmap, width, height, cur);
		if (partial) {
			D3D9_SetTexturePartData(texture, x, y, &mipmap, lvl);
		} else {
			D3D9_SetTextureData(texture, &mipmap, lvl);
		}

		if (prev != bmp->Scan0) Platform_MemFree(prev);
		prev = cur;
	}
	if (prev != bmp->Scan0) Platform_MemFree(prev);
}

GfxResourceID Gfx_CreateTexture(Bitmap* bmp, bool managedPool, bool mipmaps) {
	IDirect3DTexture9* texture;
	ReturnCode hresult;
	Int32 mipmapsLevels = GfxCommon_MipmapsLevels(bmp->Width, bmp->Height);
	Int32 levels = 1 + (mipmaps ? mipmapsLevels : 0);

	if (managedPool) {
		hresult = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 0,
			D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
		ErrorHandler_CheckOrFail(hresult, "D3D9_CreateTexture");
		D3D9_SetTextureData(texture, bmp, 0);

		if (mipmaps) D3D9_DoMipmaps(texture, 0, 0, bmp, false);
	} else {
		IDirect3DTexture9* sys;
		hresult = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 0,
			D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, NULL);
		ErrorHandler_CheckOrFail(hresult, "D3D9_CreateTexture - SystemMem");
		D3D9_SetTextureData(sys, bmp, 0);

		hresult = IDirect3DDevice9_CreateTexture(device, bmp->Width, bmp->Height, levels, 0,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
		ErrorHandler_CheckOrFail(hresult, "D3D9_CreateTexture - GPU");

		hresult = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9*)sys, (IDirect3DBaseTexture9*)texture);
		ErrorHandler_CheckOrFail(hresult, "D3D9_CreateTexture - Update");
		D3D9_FreeResource(sys, 0);
	}
	return D3D9_GetOrExpand(&d3d9_textures, &d3d9_texturesCapacity, texture, d3d9_texturesExpSize);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, Int32 x, Int32 y, Bitmap* part, bool mipmaps) {
	IDirect3DTexture9* texture = d3d9_textures[texId];
	D3D9_SetTexturePartData(texture, x, y, part, 0);
	if (mipmaps) D3D9_DoMipmaps(texture, x, y, part, true);
}

void Gfx_BindTexture(GfxResourceID texId) {
	ReturnCode hresult = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9*)d3d9_textures[texId]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_BindTexture");
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	D3D9_DeleteResource((void**)d3d9_textures, d3d9_texturesCapacity, texId);
}

void Gfx_SetTexturing(bool enabled) {
	if (enabled) return;
	ReturnCode hresult = IDirect3DDevice9_SetTexture(device, 0, NULL);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTexturing");
}

void Gfx_EnableMipmaps(void) {
	if (Gfx_Mipmaps) {
		ReturnCode hresult = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		ErrorHandler_CheckOrFail(hresult, "D3D9_EnableMipmaps");
	}
}

void Gfx_DisableMipmaps(void) {
	if (Gfx_Mipmaps) {
		ReturnCode hresult = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		ErrorHandler_CheckOrFail(hresult, "D3D9_DisableMipmaps");
	}
}


bool d3d9_fogEnable = false;
bool Gfx_GetFog(void) { return d3d9_fogEnable; }
void Gfx_SetFog(bool enabled) {
	if (d3d9_fogEnable == enabled) return;

	d3d9_fogEnable = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_FOGENABLE, "D3D9_SetFog");
}

UInt32 d3d9_fogCol = 0xFF000000; /* black */
void Gfx_SetFogColour(PackedCol col) {
	if (col.Packed == d3d9_fogCol) return;

	d3d9_fogCol = col.Packed;
	D3D9_SetRenderState(col.Packed, D3DRS_FOGCOLOR, "D3D9_SetFogColour");
}

Real32 d3d9_fogDensity = -1.0f;
void Gfx_SetFogDensity(Real32 value) {
	if (value == d3d9_fogDensity) return;

	d3d9_fogDensity = value;
	UInt32 raw = *(UInt32*)&value;
	D3D9_SetRenderState(raw, D3DRS_FOGDENSITY, "D3D9_SetFogDensity");
}

Real32 d3d9_fogStart = -1.0f;
void Gfx_SetFogStart(Real32 value) {
	d3d9_fogStart = value;
	UInt32 raw = *(UInt32*)&value;
	D3D9_SetRenderState(raw, D3DRS_FOGSTART, "D3D9_SetFogStart");
}

Real32 d3d9_fogEnd = -1.0f;
void Gfx_SetFogEnd(Real32 value) {
	if (value == d3d9_fogEnd) return;

	d3d9_fogEnd = value;
	UInt32 raw = *(UInt32*)&value;
	D3D9_SetRenderState(raw, D3DRS_FOGEND, "D3D9_SetFogEnd");
}

D3DFOGMODE d3d9_fogTableMode = D3DFOG_NONE;
void Gfx_SetFogMode(Fog fogMode) {
	D3DFOGMODE mode = d3d9_modes[fogMode];
	if (mode == d3d9_fogTableMode) return;

	d3d9_fogTableMode = mode;
	D3D9_SetRenderState(mode, D3DRS_FOGTABLEMODE, "D3D9_SetFogMode");
}


void Gfx_SetFaceCulling(bool enabled) {
	D3DCULL mode = enabled ? D3DCULL_CW : D3DCULL_NONE;
	D3D9_SetRenderState(mode, D3DRS_CULLMODE, "D3D9_SetFaceCulling");
}

bool d3d9_alphaTest = false;
void Gfx_SetAlphaTest(bool enabled) {
	if (d3d9_alphaTest == enabled) return;

	d3d9_alphaTest = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ALPHATESTENABLE, "D3D9_SetAlphaTest");
}

D3DCMPFUNC d3d9_alphaTestFunc = 0;
Int32 d3d9_alphaTestRef = 0;
void Gfx_SetAlphaTestFunc(CompareFunc compareFunc, Real32 refValue) {
	d3d9_alphaTestFunc = d3d9_compareFuncs[compareFunc];
	D3D9_SetRenderState(d3d9_alphaTestFunc, D3DRS_ALPHAFUNC, "D3D9_SetAlphaTest_Func");
	d3d9_alphaTestRef = (Int32)(refValue * 255);
	D3D9_SetRenderState2(d3d9_alphaTestRef, D3DRS_ALPHAREF, "D3D9_SetAlphaTest_Ref");
}

bool d3d9_alphaBlend = false;
void Gfx_SetAlphaBlending(bool enabled) {
	if (d3d9_alphaBlend == enabled) return;

	d3d9_alphaBlend = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ALPHABLENDENABLE, "D3D9_SetAlphaBlending");
}

D3DBLEND d3d9_srcBlendFunc = 0;
D3DBLEND d3d9_dstBlendFunc = 0;
void Gfx_SetAlphaBlendFunc(BlendFunc srcBlendFunc, BlendFunc dstBlendFunc) {
	d3d9_srcBlendFunc = d3d9_blendFuncs[srcBlendFunc];
	D3D9_SetRenderState(d3d9_srcBlendFunc, D3DRS_SRCBLEND, "D3D9_SetAlphaBlendFunc_Src");
	d3d9_dstBlendFunc = d3d9_blendFuncs[dstBlendFunc];
	D3D9_SetRenderState2(d3d9_dstBlendFunc, D3DRS_DESTBLEND, "D3D9_SetAlphaBlendFunc_Dst");
}

void Gfx_SetAlphaArgBlend(bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	ReturnCode hresult = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetAlphaArgBlend");
}

UInt32 d3d9_clearCol = 0xFF000000;
void Gfx_Clear(void) {
	DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	ReturnCode hresult = IDirect3DDevice9_Clear(device, 0, NULL, flags, d3d9_clearCol, 1.0f, 0);
	ErrorHandler_CheckOrFail(hresult, "D3D9_Clear");
}

void Gfx_ClearColour(PackedCol col) {
	d3d9_clearCol = col.Packed;
}



bool d3d9_depthTest = false;
void Gfx_SetDepthTest(bool enabled) {
	d3d9_depthTest = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ZENABLE, "D3D9_SetDepthTest");
}

D3DCMPFUNC d3d9_depthTestFunc = 0;
void Gfx_SetDepthTestFunc(CompareFunc compareFunc) {
	d3d9_depthTestFunc = d3d9_compareFuncs[compareFunc];
	D3D9_SetRenderState(d3d9_alphaTestFunc, D3DRS_ZFUNC, "D3D9_SetDepthTestFunc");
}

void Gfx_SetColourWrite(bool enabled) {
	UInt32 channels = enabled ? 0xF : 0x0;
	D3D9_SetRenderState(channels, D3DRS_COLORWRITEENABLE, "D3D9_SetColourWrite");
}

bool d3d9_depthWrite = false;
void Gfx_SetDepthWrite(bool enabled) {
	d3d9_depthWrite = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ZWRITEENABLE, "D3D9_SetDepthWrite");
}


GfxResourceID Gfx_CreateDynamicVb(VertexFormat vertexFormat, Int32 maxVertices) {
	Int32 size = maxVertices * Gfx_strideSizes[vertexFormat];
	IDirect3DVertexBuffer9* vbuffer;
	ReturnCode hresult = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 
		d3d9_formatMappings[vertexFormat], D3DPOOL_DEFAULT, &vbuffer, NULL);
	ErrorHandler_CheckOrFail(hresult, "D3D9_CreateDynamicVb");

	return D3D9_GetOrExpand(&d3d9_vBuffers, &d3d9_vbuffersCapacity, vbuffer, d3d9_vBuffersExpSize);
}

void D3D9_SetVbData(IDirect3DVertexBuffer9* buffer, void* data, Int32 size, const UInt8* lockMsg, const UInt8* unlockMsg, Int32 lockFlags) {
	void* dst = NULL;
	ReturnCode hresult = IDirect3DVertexBuffer9_Lock(buffer, 0, size, &dst, lockFlags);
	ErrorHandler_CheckOrFail(hresult, lockMsg);

	Platform_MemCpy(dst, data, size);
	hresult = IDirect3DVertexBuffer9_Unlock(buffer);
	ErrorHandler_CheckOrFail(hresult, unlockMsg);
}

GfxResourceID Gfx_CreateVb(void* vertices, VertexFormat vertexFormat, Int32 count) {
	Int32 size = count * Gfx_strideSizes[vertexFormat];
	IDirect3DVertexBuffer9* vbuffer;
	ReturnCode hresult = IDirect3DDevice9_CreateVertexBuffer(device, size, D3DUSAGE_WRITEONLY,
		d3d9_formatMappings[vertexFormat], D3DPOOL_DEFAULT, &vbuffer, NULL);
	ErrorHandler_CheckOrFail(hresult, "D3D9_CreateVb");

	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_CreateVb - Lock", "D3D9_CreateVb - Unlock", 0);
	return D3D9_GetOrExpand(&d3d9_vBuffers, &d3d9_vbuffersCapacity, vbuffer, d3d9_vBuffersExpSize);
}

void D3D9_SetIbData(IDirect3DIndexBuffer9* buffer, void* data, Int32 size, const UInt8* lockMsg, const UInt8* unlockMsg) {
	void* dst = NULL;
	ReturnCode hresult = IDirect3DIndexBuffer9_Lock(buffer, 0, size, &dst, 0);
	ErrorHandler_CheckOrFail(hresult, lockMsg);

	Platform_MemCpy(dst, data, size);
	hresult = IDirect3DIndexBuffer9_Unlock(buffer);
	ErrorHandler_CheckOrFail(hresult, unlockMsg);
}

GfxResourceID Gfx_CreateIb(void* indices, Int32 indicesCount) {
	Int32 size = indicesCount * sizeof(UInt16);
	IDirect3DIndexBuffer9* ibuffer;
	ReturnCode hresult = IDirect3DDevice9_CreateIndexBuffer(device, size, D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibuffer, NULL);
	ErrorHandler_CheckOrFail(hresult, "D3D9_CreateIb");

	D3D9_SetIbData(ibuffer, indices, size, "D3D9_CreateIb - Lock", "D3D9_CreateIb - Unlock");
	return D3D9_GetOrExpand(&d3d9_iBuffers, &d3d9_ibuffersCapacity, ibuffer, d3d9_iBuffersExpSize);
}

Int32 d3d9_batchStride;
void Gfx_BindVb(GfxResourceID vb) {
	ReturnCode hresult = IDirect3DDevice9_SetStreamSource(device, 0, d3d9_vBuffers[vb], 0, d3d9_batchStride);
	ErrorHandler_CheckOrFail(hresult, "D3D9_BindVb");
}

void Gfx_BindIb(GfxResourceID ib) {
	ReturnCode hresult = IDirect3DDevice9_SetIndices(device, d3d9_iBuffers[ib]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_BindIb");
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	D3D9_DeleteResource((void**)d3d9_vBuffers, d3d9_vbuffersCapacity, vb);
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	D3D9_DeleteResource((void**)d3d9_iBuffers, d3d9_ibuffersCapacity, ib);
}

VertexFormat d3d9_batchFormat = -1;
void Gfx_SetBatchFormat(VertexFormat format) {
	if (format == d3d9_batchFormat) return;
	d3d9_batchFormat = format;

	ReturnCode hresult = IDirect3DDevice9_SetFVF(device, d3d9_formatMappings[format]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetBatchFormat");
	d3d9_batchStride = Gfx_strideSizes[format];
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, Int32 vCount) {
	Int32 size = vCount * d3d9_batchStride;
	IDirect3DVertexBuffer9* vbuffer = d3d9_vBuffers[vb];
	D3D9_SetVbData(vbuffer, vertices, size, "D3D9_SetDynamicVbData - Lock", "D3D9_SetDynamicVbData - Unlock", D3DLOCK_DISCARD);

	ReturnCode hresult = IDirect3DDevice9_SetStreamSource(device, 0, vbuffer, 0, d3d9_batchStride);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetDynamicVbData - Bind");
}

void Gfx_DrawVb_Lines(Int32 verticesCount) {
	ReturnCode hresult = IDirect3DDevice9_DrawPrimitive(device, D3DPT_LINELIST, 0, verticesCount >> 1);
	ErrorHandler_CheckOrFail(hresult, "D3D9_DrawVb_Lines");
}

void Gfx_DrawVb_IndexedTris(Int32 verticesCount) {
	ReturnCode hresult = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 
		0, 0, verticesCount, 0, verticesCount >> 1);
	ErrorHandler_CheckOrFail(hresult, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawVb_IndexedTris_Range(Int32 verticesCount, Int32 startVertex) {
	ReturnCode hresult = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	ErrorHandler_CheckOrFail(hresult, "D3D9_DrawVb_IndexedTris");
}

void Gfx_DrawIndexedVb_TrisT2fC4b(Int32 verticesCount, Int32 startVertex) {
	ReturnCode hresult = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 
		startVertex, 0, verticesCount, 0, verticesCount >> 1);
	ErrorHandler_CheckOrFail(hresult, "D3D9_DrawIndexedVb_TrisT2fC4b");
}


void Gfx_SetMatrixMode(MatrixType matrixType) {
	if (matrixType == MatrixType_Projection) {
		curStack = &projStack;
	} else if (matrixType == MatrixType_Modelview) {
		curStack = &viewStack;
	} else if (matrixType == MatrixType_Texture) {
		curStack = &texStack;
	}
}

void Gfx_LoadMatrix(Matrix* matrix) {
	if (curStack == &texStack) {
		matrix->Row2.X = matrix->Row3.X; /* NOTE: this hack fixes the texture movements. */
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	}

	Int32 idx = curStack->Index;
	curStack->Stack[idx] = *matrix;
	
	ReturnCode hresult = IDirect3DDevice9_SetTransform(device, curStack->Type, &curStack->Stack[idx]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_LoadMatrix");
}

void Gfx_LoadIdentityMatrix(void) {
	if (curStack == &texStack) {
		IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	}

	Int32 idx = curStack->Index;
	curStack->Stack[idx] = Matrix_Identity;

	ReturnCode hresult = IDirect3DDevice9_SetTransform(device, curStack->Type, &curStack->Stack[idx]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_LoadIdentityMatrix");
}

void Gfx_MultiplyMatrix(Matrix* matrix) {
	Int32 idx = curStack->Index;
	Matrix_Mul(&curStack->Stack[idx], matrix, &curStack->Stack[idx]);

	ReturnCode hresult = IDirect3DDevice9_SetTransform(device, curStack->Type, &curStack->Stack[idx]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_MultiplyMatrix");
}

void Gfx_PushMatrix(void) {
	Int32 idx = curStack->Index;
	if (idx == MatrixStack_Capacity) {
		ErrorHandler_Fail("Unable to push matrix, at capacity already");
	}

	curStack->Stack[idx + 1] = curStack->Stack[idx]; /* mimic GL behaviour */
	curStack->Index++; /* exact same, we don't need to update DirectX state. */
}

void Gfx_PopMatrix(void) {
	Int32 idx = curStack->Index;
	if (idx == 0) {
		ErrorHandler_Fail("Unable to pop matrix, at 0 already");
	}

	curStack->Index--; idx--;
	ReturnCode hresult = IDirect3DDevice9_SetTransform(device, curStack->Type, &curStack->Stack[idx]);
	ErrorHandler_CheckOrFail(hresult, "D3D9_PopMatrix");
}

#define d3d9_zN -10000.0f
#define d3d9_zF 10000.0f
void Gfx_LoadOrthoMatrix(Real32 width, Real32 height) {
	Matrix matrix;
	Matrix_OrthographicOffCenter(&matrix, 0.0f, width, height, 0.0f, d3d9_zN, d3d9_zF);

	matrix.Row2.Y = 1.0f / (d3d9_zN - d3d9_zF);
	matrix.Row2.Z = d3d9_zN / (d3d9_zN - d3d9_zF);
	matrix.Row3.Z = 1.0f;
	Gfx_LoadMatrix(&matrix);
}


void Gfx_BeginFrame(void) {
	IDirect3DDevice9_BeginScene(device);
}

void Gfx_EndFrame(void) {
	IDirect3DDevice9_EndScene(device);
	ReturnCode code = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
	if ((Int32)code >= 0) return;

	if (code != D3DERR_DEVICELOST) {
		ErrorHandler_FailWithCode(code, "D3D9_EndFrame");
	}

	/* TODO: Make sure this actually works on all graphics cards.*/
	String reason = String_FromConstant(" (Direct3D9 device lost)");
	GfxCommon_LoseContext(&reason);
	D3D9_LoopUntilRetrieved();
	D3D9_RecreateDevice();
}

void Gfx_OnWindowResize(void) {
	String reason = String_FromConstant(" (resizing window)");
	GfxCommon_LoseContext(&reason);
	D3D9_RecreateDevice();
}


void D3D9_SetDefaultRenderStates(void) {
	Gfx_SetFaceCulling(false);
	d3d9_batchFormat = -1;
	D3D9_SetRenderState(D3DRS_COLORVERTEX, false, "D3D9_ColorVertex");
	D3D9_SetRenderState2(D3DRS_LIGHTING, false, "D3D9_Lighting");
	D3D9_SetRenderState2(D3DRS_SPECULARENABLE, false, "D3D9_SpecularEnable");
	D3D9_SetRenderState2(D3DRS_LOCALVIEWER, false, "D3D9_LocalViewer");
	D3D9_SetRenderState2(D3DRS_DEBUGMONITORTOKEN, false, "D3D9_DebugMonitor");
}

void D3D9_RestoreRenderStates(void) {
	UInt32 raw;
	D3D9_SetRenderState(D3DRS_ALPHATESTENABLE, d3d9_alphaTest, "D3D9_AlphaTest");
	D3D9_SetRenderState2(D3DRS_ALPHABLENDENABLE, d3d9_alphaBlend, "D3D9_AlphaBlend");
	D3D9_SetRenderState2(D3DRS_ALPHAFUNC, d3d9_alphaTestFunc, "D3D9_AlphaTestFunc");
	D3D9_SetRenderState2(D3DRS_ALPHAREF, d3d9_alphaTestRef, "D3D9_AlphaRefFunc");
	D3D9_SetRenderState2(D3DRS_SRCBLEND, d3d9_srcBlendFunc, "D3D9_AlphaSrcBlend");
	D3D9_SetRenderState2(D3DRS_DESTBLEND, d3d9_dstBlendFunc, "D3D9_AlphaDstBlend");
	D3D9_SetRenderState2(D3DRS_FOGENABLE, d3d9_fogEnable, "D3D9_Fog");
	D3D9_SetRenderState2(D3DRS_FOGCOLOR, d3d9_fogCol, "D3D9_FogColor");
	D3D9_SetRenderState2(D3DRS_FOGDENSITY, d3d9_fogDensity, "D3D9_FogDensity");
	raw = *(UInt32*)&d3d9_fogStart;
	D3D9_SetRenderState2(D3DRS_FOGSTART, raw, "D3D9_FogStart");
	raw = *(UInt32*)&d3d9_fogEnd;
	D3D9_SetRenderState2(D3DRS_FOGEND, raw, "D3D9_FogEnd");
	D3D9_SetRenderState2(D3DRS_FOGTABLEMODE, d3d9_fogTableMode, "D3D9_FogMode");
	D3D9_SetRenderState2(D3DRS_ZFUNC, d3d9_depthTestFunc, "D3D9_DepthTestFunc");
	D3D9_SetRenderState2(D3DRS_ZENABLE, d3d9_depthTest, "D3D9_DepthTest");
	D3D9_SetRenderState2(D3DRS_ZWRITEENABLE, d3d9_depthWrite, "D3D9_DepthWrite");
}
#endif