// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
#include "GraphicsAPI.h"
#include "D3D9Api.h"
#include "ErrorHandler.h"

#define USE_DX true
#ifdef USE_DX

IDirect3D9* d3d;
IDirect3DDevice9* device;

#define D3D9_SetRenderState(raw, state, name) \
ReturnCode hresult = IDirect3DDevice9_SetRenderState(device, state, raw); \
ErrorHandler_CheckOrFail(hresult, name)

#define D3D9_SetRenderState2(raw, state, name) \
hresult = IDirect3DDevice9_SetRenderState(device, state, raw); \
ErrorHandler_CheckOrFail(hresult, name)


void Gfx_Init(Game* game) {
}


bool Gfx_SetTexturing(bool enabled) {
	if (enabled) return;
	ReturnCode hresult = IDirect3DDevice9_SetTexture(device, 0, NULL);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetTexturing");
}


bool d3d9_fogEnable = false;
void Gfx_SetFog(bool enabled) {
	if (d3d9_fogEnable == enabled) return;

	d3d9_fogEnable = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_FOGENABLE, "D3D9_SetFog");
}

UInt32 d3d9_fogCol = 0xFF000000; // black
void Gfx_SetFogColour(FastColour col) {
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

void Gfx_SetFogStart(Real32 value) {
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

D3DFOGMODE fogTableMode = D3DFOG_NONE;
void Gfx_SetFogMode(Int32 fogMode) {
	D3DFOGMODE mode = d3d9_modes[fogMode];
	if (mode == fogTableMode) return;

	fogTableMode = mode;
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
void Gfx_SetAlphaTestFunc(Int32 compareFunc, Real32 refValue) {
	d3d9_alphaTestFunc = d3d9_compareFuncs[compareFunc];
	D3D9_SetRenderState(d3d9_alphaTestFunc, D3DRS_ALPHAFUNC, "D3D9_SetAlphaTestFunc");
	d3d9_alphaTestRef = (Int32)(refValue * 255);
	D3D9_SetRenderState2(d3d9_alphaTestRef, D3DRS_ALPHAREF, "D3D9_SetAlphaTestFunc2");
}

bool d3d9_alphaBlend = false;
void Gfx_SetAlphaBlending(bool enabled) {
	if (d3d9_alphaBlend == enabled) return;

	d3d9_alphaBlend = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ALPHABLENDENABLE, "D3D9_SetAlphaBlend");
}

D3DBLEND d3d9_srcBlendFunc = 0;
D3DBLEND d3d9_dstBlendFunc = 0;
void Gfx_SetAlphaBlendFunc(Int32 srcBlendFunc, Int32 dstBlendFunc) {
	d3d9_srcBlendFunc = d3d9_blendFuncs[srcBlendFunc];
	D3D9_SetRenderState(d3d9_srcBlendFunc, D3DRS_SRCBLEND, "D3D9_SetAlphaBlendFunc");
	d3d9_dstBlendFunc = d3d9_blendFuncs[dstBlendFunc];
	D3D9_SetRenderState2(d3d9_dstBlendFunc, D3DRS_DESTBLEND, "D3D9_SetAlphaBlendFunc2");
}

void Gfx_SetAlphaArgBlend(bool enabled) {
	D3DTEXTUREOP op = enabled ? D3DTOP_MODULATE : D3DTOP_SELECTARG1;
	ReturnCode hresult = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, op);
	ErrorHandler_CheckOrFail(hresult, "D3D9_SetAlphaArgBlend");
}

UInt32 d3d9_clearCol = 0xFF000000;
void Gfx_Clear() {
	DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	ReturnCode hresult = IDirect3DDevice9_Clear(device, 0, NULL, flags, d3d9_clearCol, 1.0f, 0);
	ErrorHandler_CheckOrFail(hresult, "D3D9_Clear");
}

void Gfx_ClearColour(FastColour col) {
	d3d9_clearCol = col.Packed;
}


bool d3d9_depthTest = false;
void Gfx_SetDepthTest(bool enabled) {
	d3d9_depthTest = enabled;
	D3D9_SetRenderState((UInt32)enabled, D3DRS_ZENABLE, "D3D9_SetDepthTest");
}

D3DCMPFUNC d3d9_depthTestFunc = 0;
void Gfx_SetDepthTestFunc(Int32 compareFunc) {
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
#endif