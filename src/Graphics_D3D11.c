#include "Core.h"
#ifdef CC_BUILD_D3D11
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include "_D3D11Shaders.h"

/* Avoid pointless includes */
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define COBJMACROS
#include <d3d11.h>
static const GUID guid_ID3D11Texture2D = { 0x6f15aaf2, 0xd208, 0x4e89, { 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c } };
static const GUID guid_IXDGIDevice     = { 0x54ec77fa, 0x1377, 0x44e6, { 0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c } };

// some generally useful debugging links
//   https://docs.microsoft.com/en-us/visualstudio/debugger/graphics/visual-studio-graphics-diagnostics
//   https://stackoverflow.com/questions/50591937/how-do-i-launch-the-hlsl-debugger-in-visual-studio-2017
//   https://docs.microsoft.com/en-us/visualstudio/debugger/graphics/graphics-object-table?view=vs-2019
//   https://github.com/gfx-rs/wgpu/issues/1171
// Some generally useful background links
//   https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052

static int depthBits; // TODO implement depthBits?? for ZNear calc
static GfxResourceID white_square;

#ifdef _MSC_VER
#define CC_ALIGNED(x) __declspec(align(x))
#else
#define CC_ALIGNED(x) __attribute__((aligned(x)))
#endif

static ID3D11Device* device;
static ID3D11DeviceContext* context;
static IDXGIDevice1* dxgi_device;
static IDXGIAdapter* dxgi_adapter;
static IDXGIFactory1* dxgi_factory;
static IDXGISwapChain* swapchain;
struct ShaderDesc { const void* data; int len; };

static void IA_UpdateLayout(void);
static void VS_UpdateShader(void);
static void PS_UpdateShader(void);
static void InitPipeline(void);
static void FreePipeline(void);

static PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN _D3D11CreateDeviceAndSwapChain;

static void LoadD3D11Library(void) {
	static const struct DynamicLibSym funcs[] = {
		DynamicLib_Sym(D3D11CreateDeviceAndSwapChain)
	};
	static const cc_string path = String_FromConst("d3d11.dll");
	void* lib;
	DynamicLib_LoadAll(&path, funcs, Array_Elems(funcs), &lib);

	if (lib) return;
	Logger_FailToStart("Failed to load d3d11.dll. You may need to install Direct3D11.\n\nNOTE: Direct3D11 requires Windows 7 or later\nYou may need to use the Direct3D9 version instead.\n");
}

static void CreateDeviceAndSwapChain(void) {
	// https://docs.microsoft.com/en-us/windows/uwp/gaming/simple-port-from-direct3d-9-to-11-1-part-1--initializing-direct3d
	DWORD createFlags = 0;
	D3D_FEATURE_LEVEL fl;
	HRESULT hr;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC desc = { 0 };
	desc.BufferCount = 1;
	desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	// RefreshRate intentionally left at 0 so display's refresh rate is used
	desc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow = WindowInfo.Handle;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;
	desc.Windowed           = TRUE;

	hr = _D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
			createFlags, NULL, 0, D3D11_SDK_VERSION,
			&desc, &swapchain, &device, &fl, &context);
	if (hr) Logger_Abort2(hr, "Failed to create D3D11 device");

	// The fog calculation requires reading Z/W of fragment position (SV_POSITION) in pixel shader,
	//  unfortunately this is unsupported in Direct3d9 (https://docs.microsoft.com/en-us/windows/uwp/gaming/glsl-to-hlsl-reference)
	// So for the sake of simplicity and since only a few old GPUs don't support feature level 10 anyways
	//   https://walbourn.github.io/direct3d-feature-levels/
	//   https://github.com/MonoGame/MonoGame/issues/5789
	//  I decided to just not support GPUs that do not support at least feature level 10
	if (fl < D3D_FEATURE_LEVEL_10_0)
		Logger_FailToStart("Your GPU is too old to support the Direct3D11 version.\nTry using the Direct3D9 version instead.\n");

	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_texture2d_desc
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-limits
	//  see "Max Texture Dimension" row in Feature Support table
	Gfx.MaxTexWidth  = fl < D3D_FEATURE_LEVEL_11_0 ? 8192 : 16384;
	Gfx.MaxTexHeight = fl < D3D_FEATURE_LEVEL_11_0 ? 8192 : 16384;
}

void Gfx_Create(void) {
	LoadD3D11Library();
	CreateDeviceAndSwapChain();
	Gfx.Created         = true;
	customMipmapsLevels = true;
	Gfx_RestoreState();
}

void Gfx_Free(void) {
	Gfx_FreeState();
	IDXGISwapChain_Release(swapchain);
	ID3D11DeviceContext_Release(context);

#ifndef _DEBUG
	ID3D11Device_Release(device);
#else
	ULONG refCount = ID3D11Device_Release(device);
	if (refCount == 0) return; // device destroyed with no issues

	ID3D11Debug *d3dDebug;
	static const GUID guid_d3dDebug = { 0x79cf2233, 0x7536, 0x4948,{ 0x9d, 0x36, 0x1e, 0x46, 0x92, 0xdc, 0x57, 0x60 } };
	HRESULT hr = ID3D11Device_QueryInterface(device, &guid_d3dDebug, &d3dDebug);
	if (SUCCEEDED(hr))
	{
		hr = ID3D11Debug_ReportLiveDeviceObjects(d3dDebug, D3D11_RLDO_DETAIL);
	}
#endif
}

static cc_bool inited;
cc_bool Gfx_TryRestoreContext(void) {
	// https://docs.microsoft.com/en-us/windows/uwp/gaming/handling-device-lost-scenarios
	Gfx_Free();
	Gfx_Create();
	return true;
}

static void Gfx_FreeState(void) {
	if (!inited) return;
	inited = false;

	FreeDefaultResources();
	FreePipeline();
	Gfx_DeleteTexture(&white_square);
}

static void Gfx_RestoreState(void) {
	if (inited) return;
	inited = true;

	InitDefaultResources();
	gfx_format = -1;
	InitPipeline();

	/* 1x1 dummy white texture */
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	Gfx_RecreateTexture(&white_square, &bmp, 0, false);
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void D3D11_DoMipmaps(ID3D11Resource* texture, int x, int y, struct Bitmap* bmp, int rowWidth) {
	BitmapCol* prev = bmp->scan0;
	BitmapCol* cur;

	int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
	int lvl, width = bmp->width, height = bmp->height;

	for (lvl = 1; lvl <= lvls; lvl++) 
	{
		x /= 2; y /= 2;
		if (width > 1)  width  /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, 4, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);

		D3D11_BOX box;
		box.front  = 0;
		box.back   = 1;
		box.left   = x;
		box.right  = x + width;
		box.top    = y;
		box.bottom = y + height;

		// https://eatplayhate.me/2013/09/29/d3d11-texture-update-costs/
		// Might not be ideal, but seems to work well enough
		int stride = width * 4;
		ID3D11DeviceContext_UpdateSubresource(context, texture, lvl, &box, cur, stride, stride * height);

		if (prev != bmp->scan0) Mem_Free(prev);
		prev     = cur;
		rowWidth = width;
	}
	if (prev != bmp->scan0) Mem_Free(prev);
}

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	ID3D11Texture2D* tex = NULL;
	ID3D11ShaderResourceView* view = NULL;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width     = bmp->width;
	desc.Height    = bmp->height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format    = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Usage     = D3D11_USAGE_DEFAULT; // TODO D3D11_USAGE_IMMUTABLE when not dynamic
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = bmp->scan0;
	data.SysMemPitch      = bmp->width * 4;
	data.SysMemSlicePitch = 0;
	D3D11_SUBRESOURCE_DATA* src = &data;

	// Direct3D11 specifies pInitialData as an array of D3D11_SUBRESOURCE_DATA of length desc->MipsLevels
	// Rather than writing such code just to support the mipmaps case, I went with the simpler approach of
	//  leaving pInitialData as NULL and specfiying the texture data later using Gfx_UpdateTexturePart
	if (mipmaps) {
		desc.MipLevels += CalcMipmapsLevels(bmp->width, bmp->height);
		src = NULL;
	}

	while ((hr = ID3D11Device_CreateTexture2D(device, &desc, src, &tex)))
	{
		if (hr == E_OUTOFMEMORY) {
			// insufficient VRAM or RAM left to allocate texture, try to reduce the memory in use first
			//  if can't reduce, return 'empty' texture so that at least the game will continue running
			if (!Game_ReduceVRAM()) return 0;
		} else {
			// unknown issue, so don't even try to handle the error
			Logger_Abort2(hr, "Failed to create texture");
		}
	}

	hr = ID3D11Device_CreateShaderResourceView(device, tex, NULL, &view);
	if (hr) Logger_Abort2(hr, "Failed to create view");

	if (mipmaps) Gfx_UpdateTexturePart(view, 0, 0, bmp, mipmaps);
	return view;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	ID3D11ShaderResourceView* view = (ID3D11ShaderResourceView*)texId;
	ID3D11Resource* res = NULL;
	D3D11_BOX box;
	box.front  = 0;
	box.back   = 1;
	box.left   = x;
	box.right  = x + part->width;
	box.top    = y;
	box.bottom = y + part->height;

	// https://eatplayhate.me/2013/09/29/d3d11-texture-update-costs/
	// Might not be ideal, but seems to work well enough
	int stride = rowWidth * 4;
	ID3D11ShaderResourceView_GetResource(view, &res);
	ID3D11DeviceContext_UpdateSubresource(context, res, 0, &box, part->scan0, stride, stride * part->height);

	if (mipmaps) D3D11_DoMipmaps(res, x, y, part, rowWidth);
	ID3D11Resource_Release(res);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	ID3D11ShaderResourceView* view = (ID3D11ShaderResourceView*)(*texId);
	ID3D11Resource* res = NULL;

	if (view) {
		ID3D11ShaderResourceView_GetResource(view, &res);
		ID3D11Resource_Release(res);
		ID3D11ShaderResourceView_Release(view);

		// note that ID3D11ShaderResourceView_GetResource increments refcount, so need to Release twice
		//  https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11view-getresource
		ID3D11Resource_Release(res);
	}
	*texId = NULL;
}

void Gfx_SetTexturing(cc_bool enabled) { }


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	ID3D11Buffer* buffer = NULL;
	cc_uint16 indices[GFX_MAX_INDICES];
	fillFunc(indices, count, obj);
	
	D3D11_BUFFER_DESC desc = { 0 };
	desc.Usage     = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = count * sizeof(cc_uint16);
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = indices;
	data.SysMemPitch      = 0;
	data.SysMemSlicePitch = 0;

	HRESULT hr = ID3D11Device_CreateBuffer(device, &desc, &data, &buffer);
	if (hr) Logger_Abort2(hr, "Failed to create index buffer");
	return buffer;
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	ID3D11Buffer* buffer = (ID3D11Buffer*)(*ib);
	if (buffer) ID3D11Buffer_Release(buffer);
	*ib = NULL;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static ID3D11Buffer* CreateVertexBuffer(VertexFormat fmt, int count, cc_bool dynamic) {
	ID3D11Buffer* buffer = NULL;

	D3D11_BUFFER_DESC desc = { 0 };
	desc.Usage          = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	desc.ByteWidth = count * strideSizes[fmt];
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	/* TODO set data initially */

	HRESULT hr = ID3D11Device_CreateBuffer(device, &desc, NULL, &buffer);
	if (hr) Logger_Abort2(hr, "Failed to create vertex buffer");
	return buffer;
}

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	/* TODO immutable? */
	return CreateVertexBuffer(fmt, count, false);
}

void Gfx_DeleteVb(GfxResourceID* vb) { 
	ID3D11Buffer* buffer = (ID3D11Buffer*)(*vb);
	if (buffer) ID3D11Buffer_Release(buffer);
	*vb = NULL;
}

static void* tmp;
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	tmp = Mem_TryAlloc(count, strideSizes[fmt]);
	return tmp;
}

void Gfx_UnlockVb(GfxResourceID vb) {
	ID3D11Buffer* buffer = (ID3D11Buffer*)vb;
	ID3D11DeviceContext_UpdateSubresource(context, buffer, 0, NULL, tmp, 0, 0);
	Mem_Free(tmp);
	tmp = NULL;
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return CreateVertexBuffer(fmt, maxVertices, true);
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { 
	ID3D11Buffer* buffer = (ID3D11Buffer*)(*vb);
	if (buffer) ID3D11Buffer_Release(buffer);
	*vb = NULL;
}

static D3D11_MAPPED_SUBRESOURCE mapDesc;
void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	ID3D11Buffer* buffer = (ID3D11Buffer*)vb;
	mapDesc.pData = NULL;

	HRESULT hr = ID3D11DeviceContext_Map(context, buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapDesc);
	if (hr) Logger_Abort2(hr, "Failed to lock dynamic VB");
	return mapDesc.pData;
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	ID3D11Buffer* buffer = (ID3D11Buffer*)vb;
	ID3D11DeviceContext_Unmap(context, buffer, 0);
	Gfx_BindDynamicVb(vb);
}


/*########################################################################################################################*
*-----------------------------------------------------Vertex rendering----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	IA_UpdateLayout();
	VS_UpdateShader();
	PS_UpdateShader();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	ID3D11DeviceContext_Draw(context, verticesCount, 0);
	ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	ID3D11DeviceContext_DrawIndexed(context, ICOUNT(verticesCount), 0, 0);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	ID3D11DeviceContext_DrawIndexed(context, ICOUNT(verticesCount), 0, startVertex);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	ID3D11DeviceContext_DrawIndexed(context, ICOUNT(verticesCount), 0, startVertex);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	// Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh
	//   The simplified calculation below uses: L = 0, R = width, T = 0, B = height
	// NOTE: This calculation is shared with Direct3D 9 backend
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z =  1.0f / (zNear - zFar);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = zNear / (zNear - zFar);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	// Deliberately swap zNear/zFar in projection matrix calculation to produce
	//  a projection matrix that results in a reversed depth buffer
	// https://developer.nvidia.com/content/depth-precision-visualized
	float zNear_ = zFar;
	float zFar_  = Reversed_CalcZNear(fov, 24); // TODO don't always hardcode to 24 bits

	// Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh
	// NOTE: This calculation is shared with Direct3D 9 backend
	float c = (float)Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = zFar_ / (zNear_ - zFar_);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear_ * zFar_) / (zNear_ - zFar_);
	matrix->row4.w =  0.0f;
}

//#####################z###################################################################################################
//-------------------------------------------------------Input Assembler--------------------------------------------------
//########################################################################################################################
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage
static ID3D11InputLayout* input_textured;

static void IA_CreateLayouts(void) {
	ID3D11InputLayout* input = NULL;
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage-getting-started
	// https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-legacy-formats
	// https://stackoverflow.com/questions/23398711/d3d11-input-element-desc-element-types-ordering-packing
	// D3D11_APPEND_ALIGNED_ELEMENT
	static D3D11_INPUT_ELEMENT_DESC T_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR"   , 0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HRESULT hr = ID3D11Device_CreateInputLayout(device, T_layout, Array_Elems(T_layout), 
												vs_textured, sizeof(vs_textured), &input);
	input_textured = input;
}

static void IA_UpdateLayout(void) {
	ID3D11DeviceContext_IASetInputLayout(context, input_textured);
}

static void IA_Init(void) {
	IA_CreateLayouts();
	ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

static void IA_Free(void) {
	ID3D11InputLayout_Release(input_textured);
}

void Gfx_BindIb(GfxResourceID ib) {
	ID3D11Buffer* buffer = (ID3D11Buffer*)ib;
	ID3D11DeviceContext_IASetIndexBuffer(context, buffer, DXGI_FORMAT_R16_UINT, 0);
}

void Gfx_BindVb(GfxResourceID vb) {
	ID3D11Buffer* buffer   = (ID3D11Buffer*)vb;
	static UINT32 offset[] = { 0 };
	ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &buffer, &gfx_stride, offset);
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	ID3D11Buffer* buffer   = (ID3D11Buffer*)vb;
	static UINT32 offset[] = { 0 };
	ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &buffer, &gfx_stride, offset);
}


//########################################################################################################################
//--------------------------------------------------------Vertex shader---------------------------------------------------
//########################################################################################################################
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/vertex-shader-stage
static ID3D11VertexShader* vs_shaders[3];
static ID3D11Buffer* vs_cBuffer;

static struct CC_ALIGNED(64) VSConstants {
	struct Matrix mvp;
	float texX, texY;
} vs_constants;
static const struct ShaderDesc vs_descs[] = {
	{ vs_colored,         sizeof(vs_colored) },
	{ vs_textured,        sizeof(vs_textured) },
	{ vs_textured_offset, sizeof(vs_textured_offset) },
};

static void VS_CreateShaders(void) {
	for (int i = 0; i < Array_Elems(vs_shaders); i++)
	{
		HRESULT hr = ID3D11Device_CreateVertexShader(device, vs_descs[i].data, vs_descs[i].len, NULL, &vs_shaders[i]);
		if (hr) Logger_Abort2(hr, "Failed to compile vertex shader");
	}
}

static void VS_CreateConstants(void) {
	// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-constant-how-to
	// https://gamedev.stackexchange.com/questions/18026/directx11-how-do-i-manage-and-update-multiple-shader-constant-buffers
	D3D11_BUFFER_DESC desc = { 0 };
	desc.ByteWidth      = sizeof(vs_constants);
	//desc.Usage          = D3D11_USAGE_DYNAMIC;
	//desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	//desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage     = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = &vs_constants;
	data.SysMemPitch      = 0;
	data.SysMemSlicePitch = 0;

	HRESULT hr = ID3D11Device_CreateBuffer(device, &desc, &data, &vs_cBuffer);
	ID3D11DeviceContext_VSSetConstantBuffers(context, 0, 1, &vs_cBuffer);
}

static int VS_CalcShaderIndex(void) {
	if (gfx_format == VERTEX_FORMAT_COLOURED) return 0;

	cc_bool has_offset = vs_constants.texX != 0 || vs_constants.texY != 0;
	return has_offset ? 2 : 1;
}

static void VS_UpdateShader(void) {
	int idx = VS_CalcShaderIndex();
	ID3D11DeviceContext_VSSetShader(context, vs_shaders[idx], NULL, 0);
}

static void VS_FreeShaders(void) {
	for (int i = 0; i < Array_Elems(vs_shaders); i++) 
	{
		ID3D11VertexShader_Release(vs_shaders[i]);
	}
}

static void VS_UpdateConstants(void) {
	ID3D11DeviceContext_UpdateSubresource(context, vs_cBuffer, 0, NULL, &vs_constants, 0, 0);
}

static void VS_FreeConstants(void) {
	ID3D11Buffer_Release(vs_cBuffer);
}

static void VS_Init(void) {
	VS_CreateShaders();
	VS_CreateConstants();
	VS_UpdateShader();
}

static void VS_Free(void) {
	VS_FreeShaders();
	VS_FreeConstants();
}

static struct Matrix _view, _proj;
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&vs_constants.mvp, &_view, &_proj);
	VS_UpdateConstants();
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
}

void Gfx_EnableTextureOffset(float x, float y) {
	vs_constants.texX = x;
	vs_constants.texY = y;
	VS_UpdateShader();
	VS_UpdateConstants();
}

void Gfx_DisableTextureOffset(void) {
	vs_constants.texX = 0;
	vs_constants.texY = 0;
	VS_UpdateShader();
}


//########################################################################################################################
//---------------------------------------------------------Rasteriser-----------------------------------------------------
//########################################################################################################################
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage
static ID3D11RasterizerState* rs_states[2];
static cc_bool rs_culling;

static void RS_CreateRasterState(void) {
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc
	D3D11_RASTERIZER_DESC desc = { 0 };
	desc.CullMode              = D3D11_CULL_NONE;
	desc.FillMode              = D3D11_FILL_SOLID;
	desc.FrontCounterClockwise = true;
	desc.DepthClipEnable       = true; // otherwise vertices/pixels beyond far plane are still wrongly rendered
	ID3D11Device_CreateRasterizerState(device, &desc, &rs_states[0]);

	desc.CullMode = D3D11_CULL_BACK;
	ID3D11Device_CreateRasterizerState(device, &desc, &rs_states[1]);
}

static void RS_UpdateViewport(void) {
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width    = WindowInfo.Width;
	viewport.Height   = WindowInfo.Height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	ID3D11DeviceContext_RSSetViewports(context, 1, &viewport);
}

static void RS_UpdateRasterState(void) {
	ID3D11DeviceContext_RSSetState(context, rs_states[rs_culling]);
}

static void RS_FreeRasterStates(void) {
	for (int i = 0; i < Array_Elems(rs_states); i++) 
	{
		ID3D11RasterizerState_Release(rs_states[i]);
	}
}

static void RS_Init(void) {
	RS_CreateRasterState();
	RS_UpdateViewport();
	RS_UpdateRasterState();
}

static void RS_Free(void) {
	RS_FreeRasterStates();
}

void Gfx_SetFaceCulling(cc_bool enabled) {
	rs_culling = enabled;
	RS_UpdateRasterState();
}


//########################################################################################################################
//--------------------------------------------------------Pixel shader----------------------------------------------------
//########################################################################################################################
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/pixel-shader-stage
static ID3D11SamplerState* ps_samplers[2];
static ID3D11PixelShader* ps_shaders[12];
static ID3D11Buffer* ps_cBuffer;
static cc_bool ps_alphaTesting, ps_mipmaps;
static float ps_fogEnd, ps_fogDensity;
static PackedCol ps_fogColor;
static int ps_fogMode;

static struct CC_ALIGNED(64) PSConstants {
	float fogValue;
	float fogR, fogG, fogB;
} ps_constants;
static const struct ShaderDesc ps_descs[] = {
	{ ps_colored,       sizeof(ps_colored) },
	{ ps_textured,      sizeof(ps_textured) },
	{ ps_colored_test,  sizeof(ps_colored_test) },
	{ ps_textured_test, sizeof(ps_textured_test) },
	{ ps_colored_linear,       sizeof(ps_colored_linear) },
	{ ps_textured_linear,      sizeof(ps_textured_linear) },
	{ ps_colored_test_linear,  sizeof(ps_colored_test_linear) },
	{ ps_textured_test_linear, sizeof(ps_textured_test_linear) },
	{ ps_colored_density,       sizeof(ps_colored_density) },
	{ ps_textured_density,      sizeof(ps_textured_density) },
	{ ps_colored_test_density,  sizeof(ps_colored_test_density) },
	{ ps_textured_test_density, sizeof(ps_textured_test_density) },
};

static void PS_CreateShaders(void) {
	for (int i = 0; i < Array_Elems(ps_shaders); i++) 
	{
		HRESULT hr = ID3D11Device_CreatePixelShader(device, ps_descs[i].data, ps_descs[i].len, NULL, &ps_shaders[i]);
		if (hr) Logger_Abort2(hr, "Failed to compile pixel shader");
	}
}

static int PS_CalcShaderIndex(void) {
	int idx = gfx_format == VERTEX_FORMAT_COLOURED ? 0 : 1;
	if (ps_alphaTesting) idx += 2;

	if (gfx_fogEnabled) {
		// uncomment when it works
		if (ps_fogMode == FOG_LINEAR) idx += 4;
		if (ps_fogMode == FOG_EXP)    idx += 8;
	}
	return idx;
}

static void PS_UpdateShader(void) {
	int idx = PS_CalcShaderIndex();
	ID3D11DeviceContext_PSSetShader(context, ps_shaders[idx], NULL, 0);
}

static void PS_FreeShaders(void) {
	for (int i = 0; i < Array_Elems(ps_shaders); i++) 
	{
		ID3D11PixelShader_Release(ps_shaders[i]);
	}
}

static void PS_CreateSamplers(void) {
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createsamplerstate
	// https://gamedev.stackexchange.com/questions/18026/directx11-how-do-i-manage-and-update-multiple-shader-constant-buffers
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-buffers-constant-how-to
	D3D11_SAMPLER_DESC desc = { 0 };

	desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MaxAnisotropy  = 1;
	desc.MaxLOD         = D3D11_FLOAT32_MAX;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	HRESULT hr1 = ID3D11Device_CreateSamplerState(device, &desc, &ps_samplers[0]);

	desc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	HRESULT hr2 = ID3D11Device_CreateSamplerState(device, &desc, &ps_samplers[1]);
}

static void PS_UpdateSampler(void) {
	ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &ps_samplers[ps_mipmaps]);
}

static void PS_FreeSamplers(void) {
	for (int i = 0; i < Array_Elems(ps_samplers); i++) 
	{
		ID3D11SamplerState_Release(ps_samplers[i]);
	}
}

static void PS_CreateConstants(void) {
	D3D11_BUFFER_DESC desc = { 0 }; // TODO see notes in VS_CreateConstants
	desc.ByteWidth      = sizeof(ps_constants);
	desc.Usage     = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem          = &ps_constants;
	data.SysMemPitch      = 0;
	data.SysMemSlicePitch = 0;

	HRESULT hr = ID3D11Device_CreateBuffer(device, &desc, &data, &ps_cBuffer);
	ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &ps_cBuffer);
}

static void PS_UpdateConstants(void) {
	ps_constants.fogR = PackedCol_R(ps_fogColor) / 255.0f;
	ps_constants.fogG = PackedCol_G(ps_fogColor) / 255.0f;
	ps_constants.fogB = PackedCol_B(ps_fogColor) / 255.0f;

	// avoid doing - in pixel shader for density fog
	ps_constants.fogValue = ps_fogMode == FOG_LINEAR ? ps_fogEnd : -ps_fogDensity;
	ID3D11DeviceContext_UpdateSubresource(context, ps_cBuffer, 0, NULL, &ps_constants, 0, 0);
}

static void PS_FreeConstants(void) {
	ID3D11Buffer_Release(ps_cBuffer);
}

static void PS_Init(void) {
	PS_CreateShaders();
	PS_CreateSamplers();
	PS_CreateConstants();
	PS_UpdateSampler();
	PS_UpdateShader();
}

static void PS_Free(void) {
	PS_FreeShaders();
	PS_FreeSamplers();
	PS_FreeConstants();
}

void Gfx_SetAlphaTest(cc_bool enabled) {
	ps_alphaTesting = enabled;
	PS_UpdateShader();
}
// unnecessary? check if any performance is gained, probably irrelevant
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_BindTexture(GfxResourceID texId) {
	/* defasult texture is otherwise transparent black */
	if (!texId) texId = white_square;

	ID3D11ShaderResourceView* view = (ID3D11ShaderResourceView*)texId;
	ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &view);
}

void Gfx_SetFog(cc_bool enabled) {
	if (gfx_fogEnabled == enabled) return;
	gfx_fogEnabled = enabled;
	PS_UpdateShader();
}

void Gfx_SetFogCol(PackedCol color) {
	if (color == ps_fogColor) return;
	ps_fogColor = color;
	PS_UpdateConstants();
}

void Gfx_SetFogDensity(float value) {
	if (value == ps_fogDensity) return;
	ps_fogDensity = value;
	PS_UpdateConstants();
}

void Gfx_SetFogEnd(float value) {
	if (value == ps_fogEnd) return;
	ps_fogEnd = value;
	PS_UpdateConstants();
}

void Gfx_SetFogMode(FogFunc func) {
	if (ps_fogMode == func) return;
	ps_fogMode = func;
	PS_UpdateShader();
}

void Gfx_EnableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	ps_mipmaps = true;
	PS_UpdateSampler();
}

void Gfx_DisableMipmaps(void) {
	if (!Gfx.Mipmaps) return;
	ps_mipmaps = false;
	PS_UpdateSampler();
}


//########################################################################################################################
//-------------------------------------------------------Output merger----------------------------------------------------
//########################################################################################################################
// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage
static ID3D11RenderTargetView* backbuffer;
static ID3D11Texture2D* depthbuffer;
static ID3D11DepthStencilView* depthbufferView;
static ID3D11BlendState* om_blendStates[4];
static ID3D11DepthStencilState* om_depthStates[4];
static float gfx_clearColor[4];
static cc_bool gfx_alphaBlending, gfx_colorEnabled = true;
static cc_bool gfx_depthTest, gfx_depthWrite;

static void OM_Clear(void) {
	ID3D11DeviceContext_ClearRenderTargetView(context, backbuffer, gfx_clearColor);
	ID3D11DeviceContext_ClearDepthStencilView(context, depthbufferView, D3D11_CLEAR_DEPTH, 0.0f, 0);
}

static void OM_UpdateTarget(void) {
	ID3D11DeviceContext_OMSetRenderTargets(context, 1, &backbuffer, depthbufferView);
}

static void OM_InitTargets(void) {
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
	D3D11_TEXTURE2D_DESC desc;
	ID3D11Texture2D* pBackBuffer;
	HRESULT hr;

	hr = IDXGISwapChain_GetBuffer(swapchain, 0, &guid_ID3D11Texture2D, (void**)&pBackBuffer);
	if (hr) Logger_Abort2(hr, "Failed to get swapchain backbuffer");

	hr = ID3D11Device_CreateRenderTargetView(device, pBackBuffer, NULL, &backbuffer);
	if (hr) Logger_Abort2(hr, "Failed to create render target");

	ID3D11Texture2D_GetDesc(pBackBuffer, &desc);
    desc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &depthbuffer);
	if (hr) Logger_Abort2(hr, "Failed to create depthbuffer texture");

	hr = ID3D11Device_CreateDepthStencilView(device, depthbuffer, NULL, &depthbufferView);
	if (hr) Logger_Abort2(hr, "Failed to create depthbuffer view");

	ID3D11Texture2D_Release(pBackBuffer);
	OM_UpdateTarget();
}

static void OM_CreateDepthStates(void) {
	D3D11_DEPTH_STENCIL_DESC desc = { 0 };
	HRESULT hr;
	desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

	for (int i = 0; i < Array_Elems(om_depthStates); i++) 
	{
		desc.DepthEnable    = (i & 1) != 0;
		desc.DepthWriteMask = (i & 2) != 0;

		hr = ID3D11Device_CreateDepthStencilState(device, &desc, &om_depthStates[i]);
		if (hr) Logger_Abort2(hr, "Failed to create depth state");
	}
}

static void OM_UpdateDepthState(void) {
	ID3D11DepthStencilState* depthState = om_depthStates[gfx_depthTest | (gfx_depthWrite << 1)];
	ID3D11DeviceContext_OMSetDepthStencilState(context, depthState, 0);
}

static void OM_FreeDepthStates(void) {
	for (int i = 0; i < Array_Elems(om_depthStates); i++) 
	{
		ID3D11DepthStencilState_Release(om_depthStates[i]);
	}
}

static void OM_CreateBlendStates(void) {
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-blend-state
	D3D11_BLEND_DESC desc = { 0 };
	HRESULT hr;
	desc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;

	for (int i = 0; i < Array_Elems(om_blendStates); i++) 
	{
		desc.RenderTarget[0].RenderTargetWriteMask = (i & 1) ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
		desc.RenderTarget[0].BlendEnable           = (i & 2) != 0;

		hr = ID3D11Device_CreateBlendState(device, &desc, &om_blendStates[i]);
		if (hr) Logger_Abort2(hr, "Failed to create blend state");
	}
}

static void OM_UpdateBlendState(void) {
	ID3D11BlendState* blendState = om_blendStates[gfx_colorEnabled | (gfx_alphaBlending << 1)];
	ID3D11DeviceContext_OMSetBlendState(context, blendState, NULL, 0xffffffff);
}

static void OM_FreeBlendStates(void) {
	for (int i = 0; i < Array_Elems(om_blendStates); i++) 
	{
		ID3D11BlendState_Release(om_blendStates[i]);
	}
}

static void OM_Init(void) {
	OM_InitTargets();
	OM_CreateDepthStates();
	OM_UpdateDepthState();
	OM_CreateBlendStates();
	OM_UpdateBlendState();
}

static void OM_FreeTargets(void) {
	ID3D11DeviceContext_OMSetRenderTargets(context, 0, NULL, NULL);
	ID3D11RenderTargetView_Release(backbuffer);
	ID3D11DepthStencilView_Release(depthbufferView);
	ID3D11Texture2D_Release(depthbuffer);
}

static void OM_Free(void) {
	OM_FreeTargets();
	OM_FreeDepthStates();
	OM_FreeBlendStates();
}

void Gfx_ClearCol(PackedCol color) {
	gfx_clearColor[0] = PackedCol_R(color) / 255.0f;
	gfx_clearColor[1] = PackedCol_G(color) / 255.0f;
	gfx_clearColor[2] = PackedCol_B(color) / 255.0f;
	gfx_clearColor[3] = PackedCol_A(color) / 255.0f;
}

void Gfx_SetDepthTest(cc_bool enabled) {
	gfx_depthTest = enabled;
	OM_UpdateDepthState();
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	gfx_depthWrite = enabled;
	OM_UpdateDepthState();
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
	gfx_alphaBlending = enabled;
	OM_UpdateBlendState();
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	gfx_colorEnabled = r;
	OM_UpdateBlendState();
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* D3D11_GetRow(struct Bitmap* bmp, int y) {
	// You were expecting a BitmapCol*, but it was me, D3D11_MAPPED_SUBRESOURCE*!
	//  This is necessary because the stride of the mapped backbuffer often doesn't equal width of the bitmap
	//    e.g. with backbuffer width of 854, stride is 3456 bytes instead of expected 3416 (854*4)
	//  Therefore have to calculate row address manually instead of using Bitmap_GetRow
	D3D11_MAPPED_SUBRESOURCE* buffer = (D3D11_MAPPED_SUBRESOURCE*)bmp->scan0;

	char* row = (char*)buffer->pData + y * buffer->RowPitch;
	return (BitmapCol*)row;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	ID3D11Texture2D* tmp = NULL;
	struct Bitmap bmp;
	HRESULT hr;

	ID3D11Resource* backbuffer_res;
	D3D11_RENDER_TARGET_VIEW_DESC backbuffer_desc;
	D3D11_MAPPED_SUBRESOURCE buffer;
	ID3D11RenderTargetView_GetResource(backbuffer, &backbuffer_res);
	ID3D11RenderTargetView_GetDesc(backbuffer,     &backbuffer_desc);

	D3D11_TEXTURE2D_DESC desc = { 0 };
	desc.Width     = WindowInfo.Width;
	desc.Height    = WindowInfo.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format    = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage     = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &tmp);
	if (hr) goto finished;
	ID3D11DeviceContext_CopyResource(context, tmp, backbuffer_res);

	hr = ID3D11DeviceContext_Map(context, tmp, 0, D3D11_MAP_READ, 0, &buffer);
	if (hr) goto finished;
	{
		Bitmap_Init(bmp, desc.Width, desc.Height, (BitmapCol*)&buffer);
		hr = Png_Encode(&bmp, output, D3D11_GetRow, false);
	}
	ID3D11DeviceContext_Unmap(context, tmp, 0);

finished:
	if (tmp) { ID3D11Texture2D_Release(tmp); }
	ID3D11Resource_Release(backbuffer_res);
	return hr;
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}
void Gfx_BeginFrame(void) { OM_UpdateTarget(); }
void Gfx_Clear(void)      { OM_Clear(); }

void Gfx_EndFrame(void) {
	// https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present
	// gfx_vsync happens to match SyncInterval parameter
	HRESULT hr = IDXGISwapChain_Present(swapchain, gfx_vsync, 0);

	// run at reduced FPS when minimised
	if (hr == DXGI_STATUS_OCCLUDED) {
		TickReducedPerformance(); return;
	}
	EndReducedPerformance();

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
		Gfx_LoseContext(" (Direct3D11 device lost)");
	} else if (hr) {
		Logger_Abort2(hr, "Failed to swap buffers");
	}
	if (gfx_minFrameMs) LimitFPS();
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	HRESULT hr;
	String_Format1(info, "-- Using Direct3D11 (%i bit) --\n", &pointerSize);

	// TODO this overlaps with global declarations, switch to them at some point.. ?
	// apparently using D3D11CreateDeviceAndSwapChain is bad, need to investigate
	IDXGIDevice* dxgi_device = NULL;
	hr = ID3D11Device_QueryInterface(device, &guid_IXDGIDevice, &dxgi_device);
	if (hr || !dxgi_device) return;

	IDXGIAdapter* dxgi_adapter;
	hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
	if (hr || !dxgi_adapter) goto release_device;

	DXGI_ADAPTER_DESC desc = { 0 };
	hr = IDXGIAdapter_GetDesc(dxgi_adapter, &desc);
	if (hr) goto release_adapter;

	// desc.Description is a WCHAR, convert to char
	char adapter[128] = { 0 };
	for (int i = 0; i < 128; i++) { adapter[i] = desc.Description[i]; }

	SIZE_T vram = desc.DedicatedVideoMemory;
	SIZE_T dram = desc.DedicatedSystemMemory;
	SIZE_T sram = desc.SharedSystemMemory;
	float tram_ = (vram + dram + sram)  / (1024.0 * 1024.0);
	float vram_ = vram                  / (1024.0 * 1024.0);

	String_Format1(info, "Adapter: %c\n", adapter);
	String_Format2(info, "Graphics memory: %f2 MB total (%f2 MB VRAM)\n", &tram_, &vram_);
	PrintMaxTextureInfo(info);

release_adapter:
	IDXGIAdapter_Release(dxgi_adapter);
release_device:
	IDXGIDevice_Release(dxgi_device);
}

void Gfx_OnWindowResize(void) {
	// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#handling-window-resizing
	OM_FreeTargets();
	HRESULT hr = IDXGISwapChain_ResizeBuffers(swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	if (hr) Logger_Abort2(hr, "Failed to resize swapchain");

	OM_InitTargets();
	RS_UpdateViewport();
}

static void InitPipeline(void) {
	// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-graphics-pipeline
	IA_Init();
	VS_Init();
	RS_Init();
	PS_Init();
	OM_Init();
}

static void FreePipeline(void) {
	IA_Free();
	VS_Free();
	RS_Free();
	PS_Free();
	OM_Free();

	ID3D11DeviceContext_ClearState(context);
	// Direct3D11 uses deferred resource destruction, so Flush to force destruction
	//  https://stackoverflow.com/questions/44155133/directx11-com-object-with-0-references-not-released
	//  https://stackoverflow.com/questions/20032816/can-someone-explain-why-i-still-have-live-objects-after-releasing-the-pointers-t
	//  https://www.gamedev.net/forums/topic/659651-dxgi-leak-warnings/5172345/
	ID3D11DeviceContext_Flush(context);
}
#endif
