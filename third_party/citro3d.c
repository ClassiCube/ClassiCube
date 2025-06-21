/*
Copyright (C) 2014-2018 fincs

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you
   must not claim that you wrote the original software. If you use
   this software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source
   distribution.
*/
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

typedef u32 C3D_IVec;

typedef union
{
	struct
	{
		float w; ///< W-component
		float z; ///< Z-component
		float y; ///< Y-component
		float x; ///< X-component
	};
	float c[4];
} C3D_FVec;

typedef union
{
	C3D_FVec r[4]; ///< Rows are vectors
	float m[4*4]; ///< Raw access
} C3D_Mtx;


typedef struct
{
	u32 flags[2];
	u64 permutation;
	int attrCount;
} C3D_AttrInfo;

static void AttrInfo_Init(C3D_AttrInfo* info);
static int  AttrInfo_AddLoader(C3D_AttrInfo* info, int regId, GPU_FORMATS format, int count);

static C3D_AttrInfo* C3D_GetAttrInfo(void);




typedef struct
{
	u32 offset;
	u32 flags[2];
} C3D_BufCfg;


typedef struct
{
	void* data;
	GPU_TEXCOLOR fmt : 4;
	size_t size : 28;

	union
	{
		u32 dim;
		struct
		{
			u16 height;
			u16 width;
		};
	};

	u32 param;
	u32 border;
	union
	{
		u32 lodParam;
		struct
		{
			u16 lodBias;
			u8 maxLevel;
			u8 minLevel;
		};
	};
} C3D_Tex;

static void C3D_TexLoadImage(C3D_Tex* tex, const void* data, GPU_TEXFACE face, int level);
static void C3D_TexGenerateMipmap(C3D_Tex* tex, GPU_TEXFACE face);
static void C3D_TexBind(int unitId, C3D_Tex* tex);
static void C3D_TexFlush(C3D_Tex* tex);
static void C3D_TexDelete(C3D_Tex* tex);

static inline int C3D_TexCalcMaxLevel(u32 width, u32 height)
{
	return (31-__builtin_clz(width < height ? width : height)) - 3; // avoid sizes smaller than 8
}

static inline u32 C3D_TexCalcLevelSize(u32 size, int level)
{
	return size >> (2*level);
}

static inline u32 C3D_TexCalcTotalSize(u32 size, int maxLevel)
{
	/*
	S  = s + sr + sr^2 + sr^3 + ... + sr^n
	Sr = sr + sr^2 + sr^3 + ... + sr^(n+1)
	S-Sr = s - sr^(n+1)
	S(1-r) = s(1 - r^(n+1))
	S = s (1 - r^(n+1)) / (1-r)

	r = 1/4
	1-r = 3/4

	S = 4s (1 - (1/4)^(n+1)) / 3
	S = 4s (1 - 1/4^(n+1)) / 3
	S = (4/3) (s - s/4^(n+1))
	S = (4/3) (s - s/(1<<(2n+2)))
	S = (4/3) (s - s>>(2n+2))
	*/
	return (size - C3D_TexCalcLevelSize(size,maxLevel+1)) * 4 / 3;
}

static inline void* C3D_TexGetImagePtr(C3D_Tex* tex, void* data, int level, u32* size)
{
	if (size) *size = level >= 0 ? C3D_TexCalcLevelSize(tex->size, level) : C3D_TexCalcTotalSize(tex->size, tex->maxLevel);
	if (!level) return data;
	return (u8*)data + (level > 0 ? C3D_TexCalcTotalSize(tex->size, level-1) : 0);
}

static inline void* C3D_Tex2DGetImagePtr(C3D_Tex* tex, int level, u32* size)
{
	return C3D_TexGetImagePtr(tex, tex->data, level, size);
}

static inline void C3D_TexUpload(C3D_Tex* tex, const void* data)
{
	C3D_TexLoadImage(tex, data, GPU_TEXFACE_2D, 0);
}









static void C3D_DepthMap(bool bIsZBuffer, float zScale, float zOffset);
static void C3D_CullFace(GPU_CULLMODE mode);
static void C3D_StencilTest(void);
static void C3D_StencilOp(void);
static void C3D_EarlyDepthTest(bool enable, GPU_EARLYDEPTHFUNC function, u32 ref);
static void C3D_DepthTest(bool enable, GPU_TESTFUNC function, GPU_WRITEMASK writemask);
static void C3D_AlphaTest(bool enable, GPU_TESTFUNC function, int ref);
static void C3D_AlphaBlend(GPU_BLENDEQUATION colorEq, GPU_BLENDEQUATION alphaEq, GPU_BLENDFACTOR srcClr, GPU_BLENDFACTOR dstClr, GPU_BLENDFACTOR srcAlpha, GPU_BLENDFACTOR dstAlpha);
static void C3D_ColorLogicOp(GPU_LOGICOP op);
static void C3D_FragOpMode(GPU_FRAGOPMODE mode);
static void C3D_FragOpShadow(float scale, float bias);






#define C3D_DEFAULT_CMDBUF_SIZE 0x40000

enum
{
	C3D_UNSIGNED_BYTE = 0,
	C3D_UNSIGNED_SHORT = 1,
};

static bool C3D_Init(size_t cmdBufSize);
static void C3D_Fini(void);

static void C3D_BindProgram(shaderProgram_s* program);

static void C3D_SetViewport(u32 x, u32 y, u32 w, u32 h);
static void C3D_SetScissor(GPU_SCISSORMODE mode, u32 left, u32 top, u32 right, u32 bottom);

static void C3D_DrawElements(GPU_Primitive_t primitive, int count);

// Immediate-mode vertex submission
static void C3D_ImmDrawBegin(GPU_Primitive_t primitive);
static void C3D_ImmSendAttrib(float x, float y, float z, float w);
static void C3D_ImmDrawEnd(void);

static inline void C3D_ImmDrawRestartPrim(void)
{
	GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
}





typedef struct
{
	u32 data[128];
} C3D_FogLut;

static inline float FogLut_CalcZ(float depth, float near, float far)
{
	return far*near/(depth*(far-near)+near);
}

static void FogLut_FromArray(C3D_FogLut* lut, const float data[256]);

static void C3D_FogGasMode(GPU_FOGMODE fogMode, GPU_GASMODE gasMode, bool zFlip);
static void C3D_FogColor(u32 color);
static void C3D_FogLutBind(C3D_FogLut* lut);











typedef struct
{
	void* colorBuf;
	void* depthBuf;
	u16 width;
	u16 height;
	GPU_COLORBUF colorFmt;
	GPU_DEPTHBUF depthFmt;
	u8 colorMask : 4;
	u8 depthMask : 4;
} C3D_FrameBuf;

// Flags for C3D_FrameBufClear
typedef enum
{
	C3D_CLEAR_COLOR = BIT(0),
	C3D_CLEAR_DEPTH = BIT(1),
	C3D_CLEAR_ALL   = C3D_CLEAR_COLOR | C3D_CLEAR_DEPTH,
} C3D_ClearBits;

static u32 C3D_CalcColorBufSize(u32 width, u32 height, GPU_COLORBUF fmt);
static u32 C3D_CalcDepthBufSize(u32 width, u32 height, GPU_DEPTHBUF fmt);

static C3D_FrameBuf* C3D_GetFrameBuf(void);
static void C3D_SetFrameBuf(C3D_FrameBuf* fb);
static void C3D_FrameBufClear(C3D_FrameBuf* fb, C3D_ClearBits clearBits, u32 clearColor, u32 clearDepth);
static void C3D_FrameBufTransfer(C3D_FrameBuf* fb, gfxScreen_t screen, gfx3dSide_t side, u32 transferFlags);

static inline void C3D_FrameBufColor(C3D_FrameBuf* fb, void* buf, GPU_COLORBUF fmt)
{
	if (buf)
	{
		fb->colorBuf  = buf;
		fb->colorFmt  = fmt;
		fb->colorMask = 0xF;
	} else
	{
		fb->colorBuf  = NULL;
		fb->colorFmt  = GPU_RB_RGBA8;
		fb->colorMask = 0;
	}
}

static inline void C3D_FrameBufDepth(C3D_FrameBuf* fb, void* buf, GPU_DEPTHBUF fmt)
{
	if (buf)
	{
		fb->depthBuf  = buf;
		fb->depthFmt  = fmt;
		fb->depthMask = fmt == GPU_RB_DEPTH24_STENCIL8 ? 0x3 : 0x2;
	} else
	{
		fb->depthBuf  = NULL;
		fb->depthFmt  = GPU_RB_DEPTH24;
		fb->depthMask = 0;
	}
}







typedef struct C3D_RenderTarget_tag C3D_RenderTarget;

struct C3D_RenderTarget_tag
{
	C3D_FrameBuf frameBuf;

	bool used, linked;
	gfxScreen_t screen;
	gfx3dSide_t side;
	u32 transferFlags;
};

// Flags for C3D_FrameBegin
enum
{
	C3D_FRAME_SYNCDRAW = BIT(0), // Perform C3D_FrameSync before checking the GPU status
	C3D_FRAME_NONBLOCK = BIT(1), // Return false instead of waiting if the GPU is busy
};

static void C3D_FrameSync(void);

static bool C3D_FrameBegin(u8 flags);
static bool C3D_FrameDrawOn(C3D_RenderTarget* target);
static void C3D_FrameSplit(u8 flags);
static void C3D_FrameEnd(u8 flags);

static void C3D_RenderTargetDelete(C3D_RenderTarget* target);
static void C3D_RenderTargetSetOutput(C3D_RenderTarget* target, gfxScreen_t screen, gfx3dSide_t side, u32 transferFlags);

static inline void C3D_RenderTargetDetachOutput(C3D_RenderTarget* target)
{
	C3D_RenderTargetSetOutput(NULL, target->screen, target->side, 0);
}

static inline void C3D_RenderTargetClear(C3D_RenderTarget* target, C3D_ClearBits clearBits, u32 clearColor, u32 clearDepth)
{
	C3D_FrameBufClear(&target->frameBuf, clearBits, clearColor, clearDepth);
}

static void C3D_SyncTextureCopy(u32* inadr, u32 indim, u32* outadr, u32 outdim, u32 size, u32 flags);




typedef struct
{
	u16 srcRgb, srcAlpha;
	union
	{
		u32 opAll;
		struct { u32 opRgb:12, opAlpha:12; };
	};
	u16 funcRgb, funcAlpha;
	u32 color;
	u16 scaleRgb, scaleAlpha;
} C3D_TexEnv;

static inline void C3D_TexEnvInit(C3D_TexEnv* env)
{
	env->srcRgb     = GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0);
	env->srcAlpha   = GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0);
	env->opAll      = 0;
	env->funcRgb    = GPU_REPLACE;
	env->funcAlpha  = GPU_REPLACE;
	env->color      = 0xFFFFFFFF;
	env->scaleRgb   = GPU_TEVSCALE_1;
	env->scaleAlpha = GPU_TEVSCALE_1;
}


void Mtx_Multiply(C3D_Mtx* out, const C3D_Mtx* a, const C3D_Mtx* b)
{
	// http://www.wolframalpha.com/input/?i={{a,b,c,d},{e,f,g,h},{i,j,k,l},{m,n,o,p}}{{α,β,γ,δ},{ε,θ,ι,κ},{λ,μ,ν,ξ},{ο,π,ρ,σ}}
	int i, j;
	for (j = 0; j < 4; ++j)
		for (i = 0; i < 4; ++i)
			out->r[j].c[i] = a->r[j].x*b->r[0].c[i] + a->r[j].y*b->r[1].c[i] + a->r[j].z*b->r[2].c[i] + a->r[j].w*b->r[3].c[i];
}




#define C3D_FVUNIF_COUNT 96
#define C3D_IVUNIF_COUNT 4

static C3D_FVec C3D_FVUnif[C3D_FVUNIF_COUNT];
static C3D_IVec C3D_IVUnif[C3D_IVUNIF_COUNT];
static u16      C3D_BoolUnifs;

static bool C3D_FVUnifDirty[C3D_FVUNIF_COUNT];
static bool C3D_IVUnifDirty[C3D_IVUNIF_COUNT];
static bool C3D_BoolUnifsDirty;

static inline C3D_FVec* C3D_FVUnifWritePtr(int id, int size)
{
	int i;
	for (i = 0; i < size; i ++)
		C3D_FVUnifDirty[id+i] = true;
	return &C3D_FVUnif[id];
}

static inline void C3D_FVUnifMtx4x4(int id, const C3D_Mtx* mtx)
{
	int i;
	C3D_FVec* ptr = C3D_FVUnifWritePtr(id, 4);
	for (i = 0; i < 4; i ++)
		ptr[i] = mtx->r[i]; // Struct copy.
}

static inline void C3D_FVUnifSet(int id, float x, float y, float z, float w)
{
	C3D_FVec* ptr = C3D_FVUnifWritePtr(id, 1);
	ptr->x = x;
	ptr->y = y;
	ptr->z = z;
	ptr->w = w;
}

static void C3D_UpdateUniforms(void);






typedef struct
{
	u32 fragOpMode;
	u32 fragOpShadow;
	u32 zScale, zOffset;
	GPU_CULLMODE cullMode;
	bool zBuffer, earlyDepth;
	GPU_EARLYDEPTHFUNC earlyDepthFunc;
	u32 earlyDepthRef;

	u32 alphaTest;
	u32 stencilMode, stencilOp;
	u32 depthTest;

	u32 alphaBlend;
	GPU_LOGICOP clrLogicOp;
} C3D_Effect;

typedef struct
{
	gxCmdQueue_s gxQueue;
	u32* cmdBuf;
	size_t cmdBufSize;

	u32 flags;
	shaderProgram_s* program;

	C3D_AttrInfo attrInfo;
	C3D_Effect effect;

	u32 texConfig;
	C3D_Tex* tex[3];

	u32 texEnvBuf, texEnvBufClr;
	u32 fogClr;
	C3D_FogLut* fogLut;

	C3D_FrameBuf fb;
	u32 viewport[5];
	u32 scissor[3];
} C3D_Context;

enum
{
	C3DiF_Active = BIT(0),
	C3DiF_DrawUsed = BIT(1),
	C3DiF_AttrInfo = BIT(2),
	C3DiF_Effect = BIT(4),
	C3DiF_FrameBuf = BIT(5),
	C3DiF_Viewport = BIT(6),
	C3DiF_Scissor = BIT(7),
	C3DiF_Program = BIT(8),
	C3DiF_TexEnvBuf = BIT(9),
	C3DiF_VshCode = BIT(11),
	C3DiF_GshCode = BIT(12),
	C3DiF_TexStatus = BIT(14),
	C3DiF_FogLut = BIT(17),
	C3DiF_Gas = BIT(18),

	C3DiF_Reset = BIT(19),

#define C3DiF_Tex(n) BIT(23+(n))
	C3DiF_TexAll = 7 << 23,
};

static C3D_Context __C3D_Context;
static inline C3D_Context* C3Di_GetContext(void)
{
	extern C3D_Context __C3D_Context;
	return &__C3D_Context;
}

static inline bool addrIsVRAM(const void* addr)
{
	u32 vaddr = (u32)addr;
	return vaddr >= OS_VRAM_VADDR && vaddr < OS_VRAM_VADDR + OS_VRAM_SIZE;
}

static inline vramAllocPos addrGetVRAMBank(const void* addr)
{
	u32 vaddr = (u32)addr;
	return vaddr < OS_VRAM_VADDR + OS_VRAM_SIZE/2 ? VRAM_ALLOC_A : VRAM_ALLOC_B;
}

static void C3Di_UpdateContext(void);
static void C3Di_AttrInfoBind(C3D_AttrInfo* info);
static void C3Di_FrameBufBind(C3D_FrameBuf* fb);
static void C3Di_TexEnvBind(int id, C3D_TexEnv* env);
static void C3Di_SetTex(int unit, C3D_Tex* tex);
static void C3Di_EffectBind(C3D_Effect* effect);

static void C3Di_DirtyUniforms(void);
static void C3Di_LoadShaderUniforms(shaderInstance_s* si);
static void C3Di_ClearShaderUniforms(GPU_SHADER_TYPE type);

static bool C3Di_SplitFrame(u32** pBuf, u32* pSize);

static void C3Di_RenderQueueInit(void);
static void C3Di_RenderQueueExit(void);
static void C3Di_RenderQueueWaitDone(void);
static void C3Di_RenderQueueEnableVBlank(void);
static void C3Di_RenderQueueDisableVBlank(void);









static void AttrInfo_Init(C3D_AttrInfo* info)
{
	memset(info, 0, sizeof(*info));
	info->flags[1] = 0xFFF << 16;
}

static int AttrInfo_AddLoader(C3D_AttrInfo* info, int regId, GPU_FORMATS format, int count)
{
	if (info->attrCount == 12) return -1;
	int id = info->attrCount++;
	if (regId < 0) regId = id;
	if (id < 8)
		info->flags[0] |= GPU_ATTRIBFMT(id, count, format);
	else
		info->flags[1] |= GPU_ATTRIBFMT(id-8, count, format);

	info->flags[1] = (info->flags[1] &~ (0xF0000000 | BIT(id+16))) | (id << 28);
	info->permutation |= regId << (id*4);
	return id;
}

static C3D_AttrInfo* C3D_GetAttrInfo(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	ctx->flags |= C3DiF_AttrInfo;
	return &ctx->attrInfo;
}

static void C3Di_AttrInfoBind(C3D_AttrInfo* info)
{
	GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFERS_FORMAT_LOW, (u32*)info->flags, sizeof(info->flags)/sizeof(u32));
	GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0xB, 0xA0000000 | (info->attrCount - 1));
	GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, info->attrCount - 1);
	GPUCMD_AddIncrementalWrites(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW, (u32*)&info->permutation, 2);
}











#define BUFFER_BASE_PADDR 0x18000000



static void C3D_DrawElements(GPU_Primitive_t primitive, int count)
{
	C3Di_UpdateContext();

	// Set primitive type
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2, primitive != GPU_TRIANGLES ? primitive : GPU_GEOMETRY_PRIM);
	// Start a new primitive (breaks off a triangle strip/fan)
	GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
	// Number of vertices
	GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);
	// First vertex
	GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0);
	// Enable triangle element drawing mode if necessary
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0x100);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0x100);
	// Enable drawing mode
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
	// Trigger element drawing
	GPUCMD_AddWrite(GPUREG_DRAWELEMENTS, 1);
	// Go back to configuration mode
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);
	// Disable triangle element drawing mode if necessary
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0);
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0);
	// Clear the post-vertex cache
	GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x8, 0);
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x8, 0);

	C3Di_GetContext()->flags |= C3DiF_DrawUsed;
}








static inline C3D_Effect* getEffect()
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Effect;
	return &ctx->effect;
}

static void C3D_DepthMap(bool bIsZBuffer, float zScale, float zOffset)
{
	C3D_Effect* e = getEffect();
	e->zBuffer = bIsZBuffer;
	e->zScale  = f32tof24(zScale);
	e->zOffset = f32tof24(zOffset);
}

static void C3D_CullFace(GPU_CULLMODE mode)
{
	C3D_Effect* e = getEffect();
	e->cullMode = mode;
}

static void C3D_StencilTest(void)
{
	C3D_Effect* e = getEffect();
	e->stencilMode = false | (GPU_ALWAYS << 4) | (0xFF << 24);
}

static void C3D_StencilOp(void)
{
	C3D_Effect* e = getEffect();
	e->stencilOp = GPU_STENCIL_KEEP | (GPU_STENCIL_KEEP << 4) | (GPU_STENCIL_KEEP << 8);
}

static void C3D_EarlyDepthTest(bool enable, GPU_EARLYDEPTHFUNC function, u32 ref)
{
	C3D_Effect* e = getEffect();
	e->earlyDepth = enable;
	e->earlyDepthFunc = function;
	e->earlyDepthRef = ref;
}

static void C3D_DepthTest(bool enable, GPU_TESTFUNC function, GPU_WRITEMASK writemask)
{
	C3D_Effect* e = getEffect();
	e->depthTest = (!!enable) | ((function & 7) << 4) | (writemask << 8);
}

static void C3D_AlphaTest(bool enable, GPU_TESTFUNC function, int ref)
{
	C3D_Effect* e = getEffect();
	e->alphaTest = (!!enable) | ((function & 7) << 4) | (ref << 8);
}

static void C3D_AlphaBlend(GPU_BLENDEQUATION colorEq, GPU_BLENDEQUATION alphaEq, GPU_BLENDFACTOR srcClr, GPU_BLENDFACTOR dstClr, GPU_BLENDFACTOR srcAlpha, GPU_BLENDFACTOR dstAlpha)
{
	C3D_Effect* e = getEffect();
	e->alphaBlend = colorEq | (alphaEq << 8) | (srcClr << 16) | (dstClr << 20) | (srcAlpha << 24) | (dstAlpha << 28);
	e->fragOpMode &= ~0xFF00;
	e->fragOpMode |= 0x0100;
}

static void C3D_ColorLogicOp(GPU_LOGICOP op)
{
	C3D_Effect* e = getEffect();
	e->fragOpMode &= ~0xFF00;
	e->clrLogicOp = op;
}

static void C3D_FragOpMode(GPU_FRAGOPMODE mode)
{
	C3D_Effect* e = getEffect();
	e->fragOpMode &= ~0xFF00FF;
	e->fragOpMode |= 0xE40000 | mode;
}

static void C3D_FragOpShadow(float scale, float bias)
{
	C3D_Effect* e = getEffect();
	e->fragOpShadow = f32tof16(scale+bias) | (f32tof16(-scale)<<16);
}

static void C3Di_EffectBind(C3D_Effect* e)
{
	GPUCMD_AddWrite(GPUREG_DEPTHMAP_ENABLE, e->zBuffer ? 1 : 0);
	GPUCMD_AddWrite(GPUREG_FACECULLING_CONFIG, e->cullMode & 0x3);
	GPUCMD_AddIncrementalWrites(GPUREG_DEPTHMAP_SCALE, (u32*)&e->zScale, 2);
	GPUCMD_AddIncrementalWrites(GPUREG_FRAGOP_ALPHA_TEST, (u32*)&e->alphaTest, 4);
	GPUCMD_AddMaskedWrite(GPUREG_GAS_DELTAZ_DEPTH, 0x8, (u32)GPU_MAKEGASDEPTHFUNC((e->depthTest>>4)&7) << 24);
	GPUCMD_AddWrite(GPUREG_BLEND_COLOR, 0);
	GPUCMD_AddWrite(GPUREG_BLEND_FUNC, e->alphaBlend);
	GPUCMD_AddWrite(GPUREG_LOGIC_OP, e->clrLogicOp);
	GPUCMD_AddMaskedWrite(GPUREG_COLOR_OPERATION, 7, e->fragOpMode);
	GPUCMD_AddWrite(GPUREG_FRAGOP_SHADOW, e->fragOpShadow);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 1, e->earlyDepth ? 1 : 0);
	GPUCMD_AddWrite(GPUREG_EARLYDEPTH_TEST2, e->earlyDepth ? 1 : 0);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_FUNC, 1, e->earlyDepthFunc);
	GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_DATA, 0x7, e->earlyDepthRef);
}







static void FogLut_FromArray(C3D_FogLut* lut, const float data[256])
{
	int i;
	for (i = 0; i < 128; i ++)
	{
		float in = data[i], diff = data[i+128];

		u32 val = 0;
		if (in > 0.0f)
		{
			in *= 0x800;
			val = (in < 0x800) ? (u32)in : 0x7FF;
		}

		u32 val2 = 0;
		if (diff != 0.0f)
		{
			diff *= 0x800;
			if (diff < -0x1000) diff = -0x1000;
			else if (diff > 0xFFF) diff = 0xFFF;
			val2 = (s32)diff & 0x1FFF;
		}

		lut->data[i] = val2 | (val << 13);
	}
}

static void C3D_FogGasMode(GPU_FOGMODE fogMode, GPU_GASMODE gasMode, bool zFlip)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	ctx->flags |= C3DiF_TexEnvBuf;
	ctx->texEnvBuf &= ~0x100FF;
	ctx->texEnvBuf |= (fogMode&7) | ((gasMode&1)<<3) | (zFlip ? BIT(16) : 0);
}

static void C3D_FogColor(u32 color)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	ctx->flags |= C3DiF_TexEnvBuf;
	ctx->fogClr = color;
}

static void C3D_FogLutBind(C3D_FogLut* lut)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	if (lut)
	{
		ctx->flags |= C3DiF_FogLut;
		ctx->fogLut = lut;
	} else
		ctx->flags &= ~C3DiF_FogLut;
}










static const u8 colorFmtSizes[] = {2,1,0,0,0};
static const u8 depthFmtSizes[] = {0,0,1,2};

static u32 C3D_CalcColorBufSize(u32 width, u32 height, GPU_COLORBUF fmt)
{
	u32 size = width*height;
	return size*(2+colorFmtSizes[fmt]);
}

static u32 C3D_CalcDepthBufSize(u32 width, u32 height, GPU_DEPTHBUF fmt)
{
	u32 size = width*height;
	return size*(2+depthFmtSizes[fmt]);
}

static C3D_FrameBuf* C3D_GetFrameBuf(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	ctx->flags |= C3DiF_FrameBuf;
	return &ctx->fb;
}

static void C3D_SetFrameBuf(C3D_FrameBuf* fb)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	if (fb != &ctx->fb)
		memcpy(&ctx->fb, fb, sizeof(*fb));
	ctx->flags |= C3DiF_FrameBuf;
}

static void C3Di_FrameBufBind(C3D_FrameBuf* fb)
{
	u32 param[4] = { 0, 0, 0, 0 };

	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);

	param[0] = osConvertVirtToPhys(fb->depthBuf) >> 3;
	param[1] = osConvertVirtToPhys(fb->colorBuf) >> 3;
	param[2] = 0x01000000 | (((u32)(fb->height-1) & 0xFFF) << 12) | (fb->width & 0xFFF);
	GPUCMD_AddIncrementalWrites(GPUREG_DEPTHBUFFER_LOC, param, 3);

	GPUCMD_AddWrite(GPUREG_RENDERBUF_DIM,       param[2]);
	GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT,  fb->depthFmt);
	GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT,  colorFmtSizes[fb->colorFmt] | ((u32)fb->colorFmt << 16));
	GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, 0);

	// Enable or disable color/depth buffers
	param[0] = param[1] = fb->colorBuf ? fb->colorMask : 0;
	param[2] = param[3] = fb->depthBuf ? fb->depthMask : 0;
	GPUCMD_AddIncrementalWrites(GPUREG_COLORBUFFER_READ, param, 4);
}

static void C3D_FrameBufClear(C3D_FrameBuf* frameBuf, C3D_ClearBits clearBits, u32 clearColor, u32 clearDepth)
{
	u32 size = (u32)frameBuf->width * frameBuf->height;
	u32 cfs = colorFmtSizes[frameBuf->colorFmt];
	u32 dfs = depthFmtSizes[frameBuf->depthFmt];
	void* colorBufEnd = (u8*)frameBuf->colorBuf + size*(2+cfs);
	void* depthBufEnd = (u8*)frameBuf->depthBuf + size*(2+dfs);

	if (clearBits & C3D_CLEAR_COLOR)
	{
		if (clearBits & C3D_CLEAR_DEPTH)
			GX_MemoryFill(
				(u32*)frameBuf->colorBuf, clearColor, (u32*)colorBufEnd, BIT(0) | (cfs << 8),
				(u32*)frameBuf->depthBuf, clearDepth, (u32*)depthBufEnd, BIT(0) | (dfs << 8));
		else
			GX_MemoryFill(
				(u32*)frameBuf->colorBuf, clearColor, (u32*)colorBufEnd, BIT(0) | (cfs << 8),
				NULL, 0, NULL, 0);
	} else
		GX_MemoryFill(
			(u32*)frameBuf->depthBuf, clearDepth, (u32*)depthBufEnd, BIT(0) | (dfs << 8),
			NULL, 0, NULL, 0);
}

static void C3D_FrameBufTransfer(C3D_FrameBuf* frameBuf, gfxScreen_t screen, gfx3dSide_t side, u32 transferFlags)
{
	u32* outputFrameBuf = (u32*)gfxGetFramebuffer(screen, side, NULL, NULL);
	u32 dim = GX_BUFFER_DIM((u32)frameBuf->width, (u32)frameBuf->height);
	GX_DisplayTransfer((u32*)frameBuf->colorBuf, dim, outputFrameBuf, dim, transferFlags);
}







static void C3D_ImmDrawBegin(GPU_Primitive_t primitive)
{
	C3Di_UpdateContext();

	// Set primitive type
	GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2, primitive);
	// Start a new primitive (breaks off a triangle strip/fan)
	GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
	// Enable vertex submission mode
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 1);
	// Enable drawing mode
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
	// Begin immediate-mode vertex submission
	GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_INDEX, 0xF);
}

static inline void write24(u8* p, u32 val)
{
	p[0] = val;
	p[1] = val>>8;
	p[2] = val>>16;
}

static void C3D_ImmSendAttrib(float x, float y, float z, float w)
{
	union
	{
		u32 packed[3];
		struct
		{
			u8 x[3];
			u8 y[3];
			u8 z[3];
			u8 w[3];
		};
	} param;

	// Convert the values to float24
	write24(param.x, f32tof24(x));
	write24(param.y, f32tof24(y));
	write24(param.z, f32tof24(z));
	write24(param.w, f32tof24(w));

	// Reverse the packed words
	u32 p = param.packed[0];
	param.packed[0] = param.packed[2];
	param.packed[2] = p;

	// Send the attribute
	GPUCMD_AddIncrementalWrites(GPUREG_FIXEDATTRIB_DATA0, param.packed, 3);
}

static void C3D_ImmDrawEnd(void)
{
	// Go back to configuration mode
	GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);
	// Disable vertex submission mode
	GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 0);
	// Clear the post-vertex cache
	GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);

	C3Di_GetContext()->flags |= C3DiF_DrawUsed;
}



static C3D_RenderTarget *linkedTarget[3];

static bool inFrame, inSafeTransfer;
static bool needSwapTop, needSwapBot, isTopStereo;
static u32 vblankCounter[2];

static void C3Di_RenderTargetDestroy(C3D_RenderTarget* target);

static void onVBlank0(void* unused)
{
	vblankCounter[0]++;
}

static void onVBlank1(void* unused)
{
	vblankCounter[1]++;
}

static void onQueueFinish(gxCmdQueue_s* queue)
{
	if (inSafeTransfer)
	{
		inSafeTransfer = false;
		if (inFrame)
		{
			gxCmdQueueStop(queue);
			gxCmdQueueClear(queue);
		}
	}
	else
	{
		if (needSwapTop)
		{
			gfxScreenSwapBuffers(GFX_TOP, isTopStereo);
			needSwapTop = false;
		}
		if (needSwapBot)
		{
			gfxScreenSwapBuffers(GFX_BOTTOM, false);
			needSwapBot = false;
		}
	}
}

static void C3D_FrameSync(void)
{
	u32 cur[2];
	u32 start[2] = { vblankCounter[0], vblankCounter[1] };
	do
	{
		gspWaitForAnyEvent();
		cur[0] = vblankCounter[0];
		cur[1] = vblankCounter[1];
	} while (cur[0]==start[0] || cur[1]==start[1]);
}

static bool C3Di_WaitAndClearQueue(s64 timeout)
{
	gxCmdQueue_s* queue = &C3Di_GetContext()->gxQueue;
	if (!gxCmdQueueWait(queue, timeout))
		return false;
	gxCmdQueueStop(queue);
	gxCmdQueueClear(queue);
	return true;
}

static void C3Di_RenderQueueEnableVBlank(void)
{
	gspSetEventCallback(GSPGPU_EVENT_VBlank0, onVBlank0, NULL, false);
	gspSetEventCallback(GSPGPU_EVENT_VBlank1, onVBlank1, NULL, false);
}

static void C3Di_RenderQueueDisableVBlank(void)
{
	gspSetEventCallback(GSPGPU_EVENT_VBlank0, NULL, NULL, false);
	gspSetEventCallback(GSPGPU_EVENT_VBlank1, NULL, NULL, false);
}

static void C3Di_RenderQueueInit(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	C3Di_RenderQueueEnableVBlank();

	GX_BindQueue(&ctx->gxQueue);
	gxCmdQueueSetCallback(&ctx->gxQueue, onQueueFinish, NULL);
	gxCmdQueueRun(&ctx->gxQueue);
}

static void C3Di_RenderQueueExit(void)
{
	C3Di_WaitAndClearQueue(-1);
	gxCmdQueueSetCallback(&C3Di_GetContext()->gxQueue, NULL, NULL);
	GX_BindQueue(NULL);

	C3Di_RenderQueueDisableVBlank();
}

static void C3Di_RenderQueueWaitDone(void)
{
	C3Di_WaitAndClearQueue(-1);
}

static bool C3D_FrameBegin(u8 flags)
{
	C3D_Context* ctx = C3Di_GetContext();
	if (inFrame) return false;

	if (!C3Di_WaitAndClearQueue((flags & C3D_FRAME_NONBLOCK) ? 0 : -1))
		return false;

	inFrame = true;
	GPUCMD_SetBuffer(ctx->cmdBuf, ctx->cmdBufSize, 0);
	return true;
}

static bool C3D_FrameDrawOn(C3D_RenderTarget* target)
{
	if (!inFrame) return false;

	target->used = true;
	C3D_SetFrameBuf(&target->frameBuf);
	C3D_SetViewport(0, 0, target->frameBuf.width, target->frameBuf.height);
	return true;
}

static void C3D_FrameSplit(u8 flags)
{
	u32 *cmdBuf, cmdBufSize;
	if (!inFrame) return;
	if (C3Di_SplitFrame(&cmdBuf, &cmdBufSize))
		GX_ProcessCommandList(cmdBuf, cmdBufSize*4, flags);
}

static void C3D_FrameEnd(u8 flags)
{
	C3D_Context* ctx = C3Di_GetContext();
	if (!inFrame) return;

	C3D_FrameSplit(flags);
	GPUCMD_SetBuffer(NULL, 0, 0);
	inFrame = false;

	// Flush the entire linear memory if the user did not explicitly mandate to flush the command list
	if (!(flags & GX_CMDLIST_FLUSH))
	{
		extern u32 __ctru_linear_heap;
		extern u32 __ctru_linear_heap_size;
		GSPGPU_FlushDataCache((void*)__ctru_linear_heap, __ctru_linear_heap_size);
	}

	C3D_RenderTarget* target;
	isTopStereo = false;
	needSwapTop = true;
	needSwapBot = true;

	for (int i = 2; i >= 0; i --)
	{
		target = linkedTarget[i];
		if (!target || !target->used)
			continue;

		target->used = false;
		C3D_FrameBufTransfer(&target->frameBuf, target->screen, target->side, target->transferFlags);

		if (target->screen == GFX_TOP && target->side == GFX_RIGHT) isTopStereo = true;
	}

	gxCmdQueueRun(&ctx->gxQueue);
}

static void C3D_RenderTargetInit(C3D_RenderTarget* target, int width, int height)
{
	memset(target, 0, sizeof(C3D_RenderTarget));

	C3D_FrameBuf* fb = &target->frameBuf;
	fb->width   = width;
	fb->height  = height;
}

static void C3D_RenderTargetColor(C3D_RenderTarget* target, GPU_COLORBUF colorFmt)
{
	C3D_FrameBuf* fb = &target->frameBuf;
	size_t colorSize = C3D_CalcColorBufSize(fb->width, fb->height, colorFmt);
	void* colorBuf   = vramAlloc(colorSize);

	if (!colorBuf) return;
	C3D_FrameBufColor(fb, colorBuf, colorFmt);
}

static void C3D_RenderTargetDepth(C3D_RenderTarget* target, GPU_DEPTHBUF depthFmt)
{
	C3D_FrameBuf* fb = &target->frameBuf;
	size_t depthSize = C3D_CalcDepthBufSize(fb->width, fb->height, depthFmt);
	void* depthBuf   = NULL;

	vramAllocPos vramBank = addrGetVRAMBank(fb->colorBuf);
	depthBuf = vramAllocAt(depthSize, vramBank ^ VRAM_ALLOC_ANY); // Attempt opposite bank first...
	if (!depthBuf) depthBuf = vramAllocAt(depthSize, vramBank); // ... if that fails, attempt same bank
	if (!depthBuf) return;

	C3D_FrameBufDepth(fb, depthBuf, depthFmt);
}

static void C3Di_RenderTargetDestroy(C3D_RenderTarget* target)
{
	vramFree(target->frameBuf.colorBuf);
	vramFree(target->frameBuf.depthBuf);
}

static void C3D_RenderTargetDelete(C3D_RenderTarget* target)
{
	if (inFrame)
		svcBreak(USERBREAK_PANIC); // Shouldn't happen.
	if (target->linked)
		C3D_RenderTargetDetachOutput(target);
	else
		C3Di_WaitAndClearQueue(-1);
	C3Di_RenderTargetDestroy(target);
}

static void C3D_RenderTargetSetOutput(C3D_RenderTarget* target, gfxScreen_t screen, gfx3dSide_t side, u32 transferFlags)
{
	int id = 0;
	if (screen==GFX_BOTTOM) id = 2;
	else if (side==GFX_RIGHT) id = 1;
	if (linkedTarget[id])
	{
		linkedTarget[id]->linked = false;
		if (!inFrame)
			C3Di_WaitAndClearQueue(-1);
	}
	linkedTarget[id] = target;
	if (target)
	{
		target->linked = true;
		target->transferFlags = transferFlags;
		target->screen = screen;
		target->side = side;
	}
}

static void C3Di_SafeTextureCopy(u32* inadr, u32 indim, u32* outadr, u32 outdim, u32 size, u32 flags)
{
	C3Di_WaitAndClearQueue(-1);
	inSafeTransfer = true;
	GX_TextureCopy(inadr, indim, outadr, outdim, size, flags);
	gxCmdQueueRun(&C3Di_GetContext()->gxQueue);
}

static void C3D_SyncTextureCopy(u32* inadr, u32 indim, u32* outadr, u32 outdim, u32 size, u32 flags)
{
	if (inFrame)
	{
		C3D_FrameSplit(0);
		GX_TextureCopy(inadr, indim, outadr, outdim, size, flags);
	} else
	{
		C3Di_SafeTextureCopy(inadr, indim, outadr, outdim, size, flags);
		gspWaitForPPF();
	}
}







static void C3Di_TexEnvBind(int id, C3D_TexEnv* env)
{
	if (id >= 4) id += 2;
	GPUCMD_AddIncrementalWrites(GPUREG_TEXENV0_SOURCE + id*8, (u32*)env, sizeof(C3D_TexEnv)/sizeof(u32));
}





static void C3D_TexLoadImage(C3D_Tex* tex, const void* data, GPU_TEXFACE face, int level)
{
	u32 size = 0;
	void* out = C3D_TexGetImagePtr(tex, tex->data, level, &size);

	if (!addrIsVRAM(out))
		memcpy(out, data, size);
	else
		C3D_SyncTextureCopy((u32*)data, 0, (u32*)out, 0, size, 8);
}

static void C3D_TexBind(int unitId, C3D_Tex* tex)
{
	C3D_Context* ctx = C3Di_GetContext();

	ctx->flags |= C3DiF_Tex(unitId);
	ctx->tex[unitId] = tex;
}

static void C3D_TexFlush(C3D_Tex* tex)
{
	if (!addrIsVRAM(tex->data))
		GSPGPU_FlushDataCache(tex->data, C3D_TexCalcTotalSize(tex->size, tex->maxLevel));
}

static void C3D_TexDelete(C3D_Tex* tex)
{
	void* addr = tex->data;
	if (addrIsVRAM(addr))
		vramFree(addr);
	else
		linearFree(addr);
}

static void C3Di_SetTex(int unit, C3D_Tex* tex)
{
	u32 reg[10];
	u32 regcount = 5;
	reg[0] = tex->border;
	reg[1] = tex->dim;
	reg[2] = tex->param;
	reg[3] = tex->lodParam;
	reg[4] = osConvertVirtToPhys(tex->data) >> 3;

	switch (unit)
	{
		case 0:
			GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT0_BORDER_COLOR, reg, regcount);
			GPUCMD_AddWrite(GPUREG_TEXUNIT0_TYPE, tex->fmt);
			break;
		case 1:
			GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT1_BORDER_COLOR, reg, 5);
			GPUCMD_AddWrite(GPUREG_TEXUNIT1_TYPE, tex->fmt);
			break;
		case 2:
			GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT2_BORDER_COLOR, reg, 5);
			GPUCMD_AddWrite(GPUREG_TEXUNIT2_TYPE, tex->fmt);
			break;
	}
}








static struct
{
	bool dirty;
	int count;
	float24Uniform_s* data;
} C3Di_ShaderFVecData;

static bool C3Di_FVUnifEverDirty[C3D_FVUNIF_COUNT];
static bool C3Di_IVUnifEverDirty[C3D_IVUNIF_COUNT];

static void C3D_UpdateUniforms(void)
{
	int i = 0;

	// Update FVec uniforms that come from shader constants
	if (C3Di_ShaderFVecData.dirty)
	{
		while (i < C3Di_ShaderFVecData.count)
		{
			float24Uniform_s* u = &C3Di_ShaderFVecData.data[i++];
			GPUCMD_AddIncrementalWrites(GPUREG_VSH_FLOATUNIFORM_CONFIG, (u32*)u, 4);
			C3D_FVUnifDirty[u->id] = false;
		}
		C3Di_ShaderFVecData.dirty = false;
		i = 0;
	}

	// Update FVec uniforms
	while (i < C3D_FVUNIF_COUNT)
	{
		if (!C3D_FVUnifDirty[i])
		{
			i ++;
			continue;
		}

		// Find the number of consecutive dirty uniforms
		int j;
		for (j = i; j < C3D_FVUNIF_COUNT && C3D_FVUnifDirty[j]; j ++);

		// Upload the uniforms
		GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x80000000|i);
		GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA, (u32*)&C3D_FVUnif[i], (j-i)*4);

		// Clear the dirty flag
		int k;
		for (k = i; k < j; k ++)
		{
			C3D_FVUnifDirty[k] = false;
			C3Di_FVUnifEverDirty[k] = true;
		}

		// Advance
		i = j;
	}

	// Update IVec uniforms
	for (i = 0; i < C3D_IVUNIF_COUNT; i ++)
	{
		if (!C3D_IVUnifDirty[i]) continue;

		GPUCMD_AddWrite(GPUREG_VSH_INTUNIFORM_I0+i, C3D_IVUnif[i]);
		C3D_IVUnifDirty[i] = false;
		C3Di_IVUnifEverDirty[i] = false;
	}

	// Update bool uniforms
	if (C3D_BoolUnifsDirty)
	{
		GPUCMD_AddWrite(GPUREG_VSH_BOOLUNIFORM, 0x7FFF0000 | C3D_BoolUnifs);
		C3D_BoolUnifsDirty = false;
	}
}

static void C3Di_DirtyUniforms(void)
{
	int i;
	C3D_BoolUnifsDirty = true;
	if (C3Di_ShaderFVecData.count)
		C3Di_ShaderFVecData.dirty = true;
	for (i = 0; i < C3D_FVUNIF_COUNT; i ++)
		C3D_FVUnifDirty[i] = C3D_FVUnifDirty[i] || C3Di_FVUnifEverDirty[i];
	for (i = 0; i < C3D_IVUNIF_COUNT; i ++)
		C3D_IVUnifDirty[i] = C3D_IVUnifDirty[i] || C3Di_IVUnifEverDirty[i];
}

static void C3Di_LoadShaderUniforms(shaderInstance_s* si)
{
	if (si->boolUniformMask)
	{
		C3D_BoolUnifs &= ~si->boolUniformMask;
		C3D_BoolUnifs |= si->boolUniforms;
	}

	C3D_BoolUnifsDirty = true;

	if (si->intUniformMask)
	{
		int i;
		for (i = 0; i < 4; i ++)
		{
			if (si->intUniformMask & BIT(i))
			{
				C3D_IVUnif[i] = si->intUniforms[i];
				C3D_IVUnifDirty[i] = true;
			}
		}
	}
	C3Di_ShaderFVecData.dirty = true;
	C3Di_ShaderFVecData.count = si->numFloat24Uniforms;
	C3Di_ShaderFVecData.data  = si->float24Uniforms;
}







static void C3Di_OnRestore(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	ctx->flags |= C3DiF_AttrInfo | C3DiF_Effect | C3DiF_FrameBuf
		| C3DiF_Viewport | C3DiF_Scissor | C3DiF_Program | C3DiF_VshCode | C3DiF_GshCode
		| C3DiF_TexAll | C3DiF_TexEnvBuf | C3DiF_Gas | C3DiF_Reset;

	C3Di_DirtyUniforms();

	if (ctx->fogLut)
		ctx->flags |= C3DiF_FogLut;
}

static bool C3D_Init(size_t cmdBufSize)
{
	int i;
	C3D_Context* ctx = C3Di_GetContext();

	if (ctx->flags & C3DiF_Active)
		return false;

	cmdBufSize = (cmdBufSize + 0xF) &~ 0xF; // 0x10-byte align
	ctx->cmdBufSize = cmdBufSize/4;
	ctx->cmdBuf = (u32*)linearAlloc(cmdBufSize);
	if (!ctx->cmdBuf)
		return false;

	ctx->gxQueue.maxEntries = 32;
	ctx->gxQueue.entries = (gxCmdEntry_s*)malloc(ctx->gxQueue.maxEntries*sizeof(gxCmdEntry_s));
	if (!ctx->gxQueue.entries)
	{
		linearFree(ctx->cmdBuf);
		return false;
	}

	ctx->flags = C3DiF_Active | C3DiF_TexEnvBuf | C3DiF_Effect | C3DiF_TexStatus | C3DiF_TexAll | C3DiF_Reset;

	// TODO: replace with direct struct access
	C3D_DepthMap(true, -1.0f, 0.0f);
	C3D_CullFace(GPU_CULL_BACK_CCW);
	C3D_StencilTest();
	C3D_StencilOp();
	C3D_EarlyDepthTest(false, GPU_EARLYDEPTH_GREATER, 0);
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	C3D_AlphaTest(false, GPU_ALWAYS, 0x00);
	C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
	C3D_FragOpMode(GPU_FRAGOPMODE_GL);
	C3D_FragOpShadow(0.0, 1.0);

	ctx->texConfig = BIT(12);
	ctx->texEnvBuf = 0;
	ctx->texEnvBufClr = 0xFFFFFFFF;
	ctx->fogClr = 0;
	ctx->fogLut = NULL;

	for (i = 0; i < 3; i ++)
		ctx->tex[i] = NULL;

	C3Di_RenderQueueInit();

	return true;
}

static void C3D_SetViewport(u32 x, u32 y, u32 w, u32 h)
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Viewport | C3DiF_Scissor;
	ctx->viewport[0] = f32tof24(w / 2.0f);
	ctx->viewport[1] = f32tof31(2.0f / w) << 1;
	ctx->viewport[2] = f32tof24(h / 2.0f);
	ctx->viewport[3] = f32tof31(2.0f / h) << 1;
	ctx->viewport[4] = (y << 16) | (x & 0xFFFF);
	ctx->scissor[0] = GPU_SCISSOR_DISABLE;
}

static void C3D_SetScissor(GPU_SCISSORMODE mode, u32 left, u32 top, u32 right, u32 bottom)
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Scissor;
	ctx->scissor[0] = mode;
	if (mode == GPU_SCISSOR_DISABLE) return;
	ctx->scissor[1] = (top << 16) | (left & 0xFFFF);
	ctx->scissor[2] = ((bottom-1) << 16) | ((right-1) & 0xFFFF);
}


static void C3Di_Reset(C3D_Context* ctx) {
	// Reset texture environment
	C3D_TexEnv texEnv;
	C3D_TexEnvInit(&texEnv);
	for (int i = 0; i < 6; i++)
	{
		C3Di_TexEnvBind(i, &texEnv);
	}

	// Reset lighting
	GPUCMD_AddWrite(GPUREG_LIGHTING_ENABLE0, false);
	GPUCMD_AddWrite(GPUREG_LIGHTING_ENABLE1,  true);

	// Reset attirubte buffer info
	C3D_BufCfg buffers[12] = { 0 };
	GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC, BUFFER_BASE_PADDR >> 3);
	GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFER0_OFFSET, (u32*)buffers, 12 * 3);
}

static void C3Di_UpdateFramebuffer(C3D_Context* ctx) {
	if (ctx->flags & C3DiF_DrawUsed)
	{
		ctx->flags &= ~C3DiF_DrawUsed;
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1);
		GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1);
	}
	C3Di_FrameBufBind(&ctx->fb);
}

static void C3Di_UpdateContext(void)
{
	int i;
	C3D_Context* ctx = C3Di_GetContext();

	if (ctx->flags & C3DiF_Reset)
	{
		ctx->flags &= ~C3DiF_Reset;
		C3Di_Reset(ctx);
	}

	if (ctx->flags & C3DiF_FrameBuf)
	{
		ctx->flags &= ~C3DiF_FrameBuf;
		C3Di_UpdateFramebuffer(ctx);
	}

	if (ctx->flags & C3DiF_Viewport)
	{
		ctx->flags &= ~C3DiF_Viewport;
		GPUCMD_AddIncrementalWrites(GPUREG_VIEWPORT_WIDTH, ctx->viewport, 4);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_XY, ctx->viewport[4]);
	}

	if (ctx->flags & C3DiF_Scissor)
	{
		ctx->flags &= ~C3DiF_Scissor;
		GPUCMD_AddIncrementalWrites(GPUREG_SCISSORTEST_MODE, ctx->scissor, 3);
	}

	if (ctx->flags & C3DiF_Program)
	{
		shaderProgramConfigure(ctx->program, (ctx->flags & C3DiF_VshCode) != 0, (ctx->flags & C3DiF_GshCode) != 0);
		ctx->flags &= ~(C3DiF_Program | C3DiF_VshCode | C3DiF_GshCode);
	}

	if (ctx->flags & C3DiF_AttrInfo)
	{
		ctx->flags &= ~C3DiF_AttrInfo;
		C3Di_AttrInfoBind(&ctx->attrInfo);
	}

	if (ctx->flags & C3DiF_Effect)
	{
		ctx->flags &= ~C3DiF_Effect;
		C3Di_EffectBind(&ctx->effect);
	}

	if (ctx->flags & C3DiF_TexAll)
	{
		u32 units = 0;
		for (i = 0; i < 3; i ++)
		{
			if (ctx->tex[i])
			{
				units |= BIT(i);
				if (ctx->flags & C3DiF_Tex(i))
					C3Di_SetTex(i, ctx->tex[i]);
			}
		}

		// Enable texture units and clear texture cache
		ctx->texConfig &= ~7;
		ctx->texConfig |= units | BIT(16);
		ctx->flags &= ~C3DiF_TexAll;
		ctx->flags |= C3DiF_TexStatus;
	}

	if (ctx->flags & C3DiF_TexStatus)
	{
		ctx->flags &= ~C3DiF_TexStatus;
		GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0xB, ctx->texConfig);
		// Clear texture cache if requested *after* configuring texture units
		if (ctx->texConfig & BIT(16))
		{
			ctx->texConfig &= ~BIT(16);
			GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0x4, BIT(16));
		}
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_SHADOW, BIT(0));
	}

	if (ctx->flags & C3DiF_TexEnvBuf)
	{
		ctx->flags &= ~C3DiF_TexEnvBuf;
		GPUCMD_AddMaskedWrite(GPUREG_TEXENV_UPDATE_BUFFER, 0x7, ctx->texEnvBuf);
		GPUCMD_AddWrite(GPUREG_TEXENV_BUFFER_COLOR, ctx->texEnvBufClr);
		GPUCMD_AddWrite(GPUREG_FOG_COLOR, ctx->fogClr);
	}

	if ((ctx->flags & C3DiF_FogLut) && (ctx->texEnvBuf&7) != GPU_NO_FOG)
	{
		ctx->flags &= ~C3DiF_FogLut;
		if (ctx->fogLut)
		{
			GPUCMD_AddWrite(GPUREG_FOG_LUT_INDEX, 0);
			GPUCMD_AddWrites(GPUREG_FOG_LUT_DATA0, ctx->fogLut->data, 128);
		}
	}

	C3D_UpdateUniforms();
}

static bool C3Di_SplitFrame(u32** pBuf, u32* pSize)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!gpuCmdBufOffset)
		return false; // Nothing was drawn

	if (ctx->flags & C3DiF_DrawUsed)
	{
		ctx->flags &= ~C3DiF_DrawUsed;
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1);
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);
		GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1);
	}

	GPUCMD_Split(pBuf, pSize);
	return true;
}

static void C3D_Fini(void)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	C3Di_RenderQueueExit();
	free(ctx->gxQueue.entries);
	linearFree(ctx->cmdBuf);
	ctx->flags = 0;
}

static void C3D_BindProgram(shaderProgram_s* program)
{
	C3D_Context* ctx = C3Di_GetContext();

	shaderProgram_s* oldProg = ctx->program;
	if (oldProg != program)
	{
		ctx->program = program;
		ctx->flags |= C3DiF_Program | C3DiF_AttrInfo;

		if (!oldProg)
			ctx->flags |= C3DiF_VshCode | C3DiF_GshCode;
		else
		{
			DVLP_s* oldProgV = oldProg->vertexShader->dvle->dvlp;
			DVLP_s* newProgV = program->vertexShader->dvle->dvlp;

			if (oldProgV != newProgV)
				ctx->flags |= C3DiF_VshCode | C3DiF_GshCode;
		}
	}

	C3Di_LoadShaderUniforms(program->vertexShader);
}
