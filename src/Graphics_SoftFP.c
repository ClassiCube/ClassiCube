#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_SOFTFP
#define CC_DYNAMIC_VBS_ARE_STATIC
#define OVERRIDE_BEGEND2D_FUNCTIONS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <limits.h>
#include <stdint.h>


// 16.16 fixed point
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)
#define FP_HALF (FP_ONE >> 1)
#define FP_MASK ((1 << FP_SHIFT) - 1)

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define FloatToFixed(x) ((int)((x) * FP_ONE))
#define FixedToFloat(x) ((float)(x) / FP_ONE)
#define FixedToInt(x) ((x) >> FP_SHIFT)
#define IntToFixed(x) ((x) << FP_SHIFT)

#define FixedMul(a, b) (((int64_t)(a) * (b)) >> FP_SHIFT)

#define FixedDiv(a, b) (((int64_t)(a) << FP_SHIFT) / (b))

static int FixedReciprocal(int x) {
    if (x == 0) return 0;
    int64_t res = ((int64_t)FP_ONE << FP_SHIFT) / (int64_t)x; // (1<<32)/x -> fixed reciprocal
    if (res > INT_MAX) return INT_MAX;
    if (res < INT_MIN) return INT_MIN;
    return (int)res;
}

static cc_bool faceCulling;
static int fb_width, fb_height; 
static struct Bitmap fb_bmp;
static int vp_hwidth_fp, vp_hheight_fp;
static int fb_maxX, fb_maxY;

static BitmapCol* colorBuffer;
static BitmapCol clearColor;
static cc_bool colWrite = true;
static int cb_stride;

static int* depthBuffer;
static cc_bool depthTest  = true;
static cc_bool depthWrite = true;
static int db_stride;

static void* gfx_vertices;
static GfxResourceID white_square;

typedef struct FixedMatrix {
    int m[4][4];
} FixedMatrix;

typedef struct VertexFixed {
    int x, y, z, w;
    int u, v;
    PackedCol c;
} VertexFixed;

static int tex_offseting;
static int texOffsetX_fp, texOffsetY_fp; // Fixed point texture offsets
static FixedMatrix _view_fp, _proj_fp, _mvp_fp;

static void Gfx_RestoreState(void) {
    InitDefaultResources();

    struct Bitmap bmp;
    BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
    Bitmap_Init(bmp, 1, 1, pixels);
    white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
    FreeDefaultResources();
    Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
    Gfx.MaxTexWidth  = 4096;
    Gfx.MaxTexHeight = 4096;
    Gfx.Created      = true;
    Gfx.BackendType  = CC_GFX_BACKEND_SOFTGPU;
    Gfx.Limitations  = GFX_LIMIT_MINIMAL;
    
    Gfx_RestoreState();
}

static void DestroyBuffers(void) {
    Window_FreeFramebuffer(&fb_bmp);
    Mem_Free(depthBuffer);
    depthBuffer = NULL;
}

void Gfx_Free(void) { 
    Gfx_FreeState();
    DestroyBuffers();
}

typedef struct CCTexture {
    int width, height;
    BitmapCol pixels[];
} CCTexture;

static CCTexture* curTexture;
static BitmapCol* curTexPixels;
static int curTexWidth, curTexHeight;
static int texWidthMask, texHeightMask;
static int texSinglePixel;
        
void Gfx_BindTexture(GfxResourceID texId) {
    if (!texId) texId = white_square;
    CCTexture* tex = texId;

    curTexture   = tex;
    curTexPixels = tex->pixels;
    curTexWidth  = tex->width;
    curTexHeight = tex->height;

    texWidthMask   = (1 << Math_ilog2(tex->width))  - 1;
    texHeightMask  = (1 << Math_ilog2(tex->height)) - 1;

    texSinglePixel = curTexWidth == 1;
}
        
void Gfx_DeleteTexture(GfxResourceID* texId) {
    GfxResourceID data = *texId;
    if (data) Mem_Free(data);
    *texId = NULL;
}
        
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
    CCTexture* tex = (CCTexture*)Mem_Alloc(2 + bmp->width * bmp->height, 4, "Texture");

    tex->width  = bmp->width;
    tex->height = bmp->height;

    CopyPixels(tex->pixels, bmp->width * BITMAPCOLOR_SIZE,
               bmp->scan0,  rowWidth * BITMAPCOLOR_SIZE,
               bmp->width,  bmp->height);
    return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
    CCTexture* tex = (CCTexture*)texId;
    BitmapCol* dst = (tex->pixels + x) + y * tex->width;

    CopyPixels(dst,         tex->width * BITMAPCOLOR_SIZE,
               part->scan0, rowWidth   * BITMAPCOLOR_SIZE,
               part->width, part->height);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*--------------------------------------------------------2D drawing-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Begin2D(int width, int height) {
	gfx_rendering2D = true;

	Gfx_SetDepthTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
}

void Gfx_End2D(void) {
	gfx_rendering2D = false;

	Gfx_SetDepthTest(true);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) {
    faceCulling = enabled;
}

static void SetAlphaTest(cc_bool enabled) {
}

static void SetAlphaBlend(cc_bool enabled) {
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void ClearColorBuffer(void) {
    int i, x, y, size = fb_width * fb_height;

    if (cb_stride == fb_width) {
        Mem_Set(colorBuffer, clearColor, sizeof(BitmapCol) * size);
    } else {
        for (y = 0; y < fb_height; y++) {
            i = y * cb_stride;
            for (x = 0; x < fb_width; x++) {
                colorBuffer[i + x] = clearColor;
            }
        }
    }
}

static void ClearDepthBuffer(void) {
    int i, size = fb_width * fb_height;
    int maxDepth = 0x7FFFFFFF; // max depth
    for (i = 0; i < size; i++) depthBuffer[i] = maxDepth;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
    if (buffers & GFX_BUFFER_COLOR) ClearColorBuffer();
    if (buffers & GFX_BUFFER_DEPTH) ClearDepthBuffer();
}

void Gfx_ClearColor(PackedCol color) {
    int R = PackedCol_R(color);
    int G = PackedCol_G(color);
    int B = PackedCol_B(color);
    int A = PackedCol_A(color);

    clearColor = BitmapCol_Make(R, G, B, A);
}

void Gfx_SetDepthTest(cc_bool enabled) {
    depthTest = enabled;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
    depthWrite = enabled;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
    // TODO
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
    colWrite = !depthOnly;
}

/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
    return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }

/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
// Preprocess vertex buffers into fixed point values
struct FPVertexColoured { 
	int x, y, z;
	BitmapCol c;
};

struct FPVertexTextured { 
	int x, y, z;
	BitmapCol c;
	int u, v;
};

static VertexFormat buf_fmt;
static int buf_count;

static void PreprocessTexturedVertices(void* vertices) {
	struct FPVertexTextured* dst = vertices;
	struct VertexTextured* src   = vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = FloatToFixed(src->x);
		dst->y = FloatToFixed(src->y);
		dst->z = FloatToFixed(src->z);
		dst->u = FloatToFixed(src->U);
		dst->v = FloatToFixed(src->V);
		dst->c = src->Col;
	}
}

static void PreprocessColouredVertices(void* vertices) {
	struct FPVertexColoured* dst = vertices;
	struct VertexColoured* src   = vertices;

	for (int i = 0; i < buf_count; i++, src++, dst++)
	{
		dst->x = FloatToFixed(src->x);
		dst->y = FloatToFixed(src->y);
		dst->z = FloatToFixed(src->z);
		dst->c = src->Col;
	}
}

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
    if (buf_fmt == VERTEX_FORMAT_TEXTURED) {
        PreprocessTexturedVertices(vb);
    } else {
        PreprocessColouredVertices(vb);
    }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/

static void MatrixToFixed(FixedMatrix* dst, const struct Matrix* src) {
    dst->m[0][0] = FloatToFixed(src->row1.x); dst->m[0][1] = FloatToFixed(src->row1.y);
    dst->m[0][2] = FloatToFixed(src->row1.z); dst->m[0][3] = FloatToFixed(src->row1.w);
    
    dst->m[1][0] = FloatToFixed(src->row2.x); dst->m[1][1] = FloatToFixed(src->row2.y);
    dst->m[1][2] = FloatToFixed(src->row2.z); dst->m[1][3] = FloatToFixed(src->row2.w);
    
    dst->m[2][0] = FloatToFixed(src->row3.x); dst->m[2][1] = FloatToFixed(src->row3.y);
    dst->m[2][2] = FloatToFixed(src->row3.z); dst->m[2][3] = FloatToFixed(src->row3.w);
    
    dst->m[3][0] = FloatToFixed(src->row4.x); dst->m[3][1] = FloatToFixed(src->row4.y);
    dst->m[3][2] = FloatToFixed(src->row4.z); dst->m[3][3] = FloatToFixed(src->row4.w);
}

static void MatrixMulFixed(FixedMatrix* dst, const FixedMatrix* a, const FixedMatrix* b) {
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            int64_t sum = 0;
            for (k = 0; k < 4; k++) {
                sum += (int64_t)a->m[i][k] * b->m[k][j];
            }
            dst->m[i][j] = (int)(sum >> FP_SHIFT);
        }
    }
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
    if (type == MATRIX_VIEW) {
        MatrixToFixed(&_view_fp, matrix);
    }
    if (type == MATRIX_PROJ) {
        MatrixToFixed(&_proj_fp, matrix);
    }

    MatrixMulFixed(&_mvp_fp, &_view_fp, &_proj_fp);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
    MatrixToFixed(&_view_fp, view);
    MatrixToFixed(&_proj_fp, proj);

    MatrixMulFixed(&_mvp_fp, &_view_fp, &_proj_fp);
    
    Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	tex_offseting = true;
    texOffsetX_fp = FloatToFixed(x);
    texOffsetY_fp = FloatToFixed(y);
}

void Gfx_DisableTextureOffset(void) {
	tex_offseting = false;
}

static CC_NOINLINE void ShiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) 
	{
		struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + i;
		v->u += texOffsetX_fp; // TODO avoid overflow
		v->v += texOffsetY_fp;
	}
}

static CC_NOINLINE void UnshiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) 
	{
		struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + i;
		v->u -= texOffsetX_fp; // TODO avoid overflow
		v->v -= texOffsetY_fp;
	}
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
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
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
static void TransformVertex2D(int index, VertexFixed* vertex) {
    struct FPVertexColoured* v_col;
    struct FPVertexTextured* v_tex;
    
    if (gfx_format != VERTEX_FORMAT_TEXTURED) {
        v_col = (struct FPVertexColoured*)gfx_vertices + index;
        vertex->x = v_col->x;
        vertex->y = v_col->y;
        vertex->u = 0;
        vertex->v = 0;
        vertex->c = v_col->c;
    } else {
        v_tex = (struct FPVertexTextured*)gfx_vertices + index;
        vertex->x = v_tex->x;
        vertex->y = v_tex->y;
        vertex->u = v_tex->u;
        vertex->v = v_tex->v;
        vertex->c = v_tex->c;
    }
}

static int TransformVertex3D(int index, VertexFixed* vertex) {
    int pos_x, pos_y, pos_z;
    struct FPVertexColoured* v_col;
    struct FPVertexTextured* v_tex;
    
    if (gfx_format != VERTEX_FORMAT_TEXTURED) {
        v_col = (struct FPVertexColoured*)gfx_vertices + index;
        vertex->x = v_col->x;
        vertex->y = v_col->y;
		vertex->z = v_col->z;
        vertex->u = 0;
        vertex->v = 0;
        vertex->c = v_col->c;
    } else {
        v_tex = (struct FPVertexTextured*)gfx_vertices + index;
        vertex->x = v_tex->x;
        vertex->y = v_tex->y;
        vertex->z = v_tex->z;
        vertex->u = v_tex->u;
        vertex->v = v_tex->v;
        vertex->c = v_tex->c;
    }

	pos_x = vertex->x;
	pos_y = vertex->y;
	pos_z = vertex->z;

    if (ABS(pos_x) > (1 << 28) || ABS(pos_y) > (1 << 28) || ABS(pos_z) > (1 << 28)) {
        return 0;
    }

    int64_t x_temp = (int64_t)pos_x * _mvp_fp.m[0][0] + (int64_t)pos_y * _mvp_fp.m[1][0] + 
                     (int64_t)pos_z * _mvp_fp.m[2][0] + ((int64_t)_mvp_fp.m[3][0] << FP_SHIFT);
    int64_t y_temp = (int64_t)pos_x * _mvp_fp.m[0][1] + (int64_t)pos_y * _mvp_fp.m[1][1] + 
                     (int64_t)pos_z * _mvp_fp.m[2][1] + ((int64_t)_mvp_fp.m[3][1] << FP_SHIFT);
    int64_t z_temp = (int64_t)pos_x * _mvp_fp.m[0][2] + (int64_t)pos_y * _mvp_fp.m[1][2] + 
                     (int64_t)pos_z * _mvp_fp.m[2][2] + ((int64_t)_mvp_fp.m[3][2] << FP_SHIFT);
    int64_t w_temp = (int64_t)pos_x * _mvp_fp.m[0][3] + (int64_t)pos_y * _mvp_fp.m[1][3] + 
                     (int64_t)pos_z * _mvp_fp.m[2][3] + ((int64_t)_mvp_fp.m[3][3] << FP_SHIFT);

    x_temp >>= FP_SHIFT;
    y_temp >>= FP_SHIFT;
    z_temp >>= FP_SHIFT;
    w_temp >>= FP_SHIFT;

    vertex->x = (x_temp > INT_MAX) ? INT_MAX : (x_temp < INT_MIN) ? INT_MIN : (int)x_temp;
    vertex->y = (y_temp > INT_MAX) ? INT_MAX : (y_temp < INT_MIN) ? INT_MIN : (int)y_temp;
    vertex->z = (z_temp > INT_MAX) ? INT_MAX : (z_temp < INT_MIN) ? INT_MIN : (int)z_temp;
    vertex->w = (w_temp > INT_MAX) ? INT_MAX : (w_temp < INT_MIN) ? INT_MIN : (int)w_temp;

    return 1;
}

static cc_bool ViewportVertex3D(VertexFixed* vertex) {
    if (vertex->w == 0) return false;
    if (ABS(vertex->w) < 64) return false; // Too small w
    
    int invW = FixedReciprocal(vertex->w);
    
    if (ABS(invW) > (FP_ONE << 10)) return false; // 1024x magnification limit

    int64_t x_ndc = ((int64_t)vertex->x * invW) >> FP_SHIFT;
    int64_t y_ndc = ((int64_t)vertex->y * invW) >> FP_SHIFT;
    int64_t z_ndc = ((int64_t)vertex->z * invW) >> FP_SHIFT;
    
    int64_t screen_x = vp_hwidth_fp  + ((x_ndc * vp_hwidth_fp)  >> FP_SHIFT);
    int64_t screen_y = vp_hheight_fp - ((y_ndc * vp_hheight_fp) >> FP_SHIFT);
    
    // Clamp
    if (screen_x < -(fb_width  << FP_SHIFT) || screen_x > (fb_width  << (FP_SHIFT + 1))) return false;
    if (screen_y < -(fb_height << FP_SHIFT) || screen_y > (fb_height << (FP_SHIFT + 1))) return false;
    
    vertex->x = (int)screen_x;
    vertex->y = (int)screen_y;
    vertex->z = (z_ndc > INT_MAX) ? INT_MAX : (z_ndc < INT_MIN) ? INT_MIN : (int)z_ndc;
    vertex->w = invW;
    
    vertex->u = FixedMul(vertex->u, invW);
    vertex->v = FixedMul(vertex->v, invW);
    
    return true;
}

static void DrawSprite2D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    PackedCol vColor = V0->c;
    int minX = FixedToInt(V0->x);
    int minY = FixedToInt(V0->y);
    int maxX = FixedToInt(V1->x);
    int maxY = FixedToInt(V2->y);

    if (maxX < 0 || minX > fb_maxX) return;
    if (maxY < 0 || minY > fb_maxY) return;

    int begTX = FixedMul(V0->u, IntToFixed(curTexWidth)) >> FP_SHIFT;
    int begTY = FixedMul(V0->v, IntToFixed(curTexHeight)) >> FP_SHIFT;
    int delTX = (FixedMul(V1->u, IntToFixed(curTexWidth)) >> FP_SHIFT) - begTX;
    int delTY = (FixedMul(V2->v, IntToFixed(curTexHeight)) >> FP_SHIFT) - begTY;

    int width = maxX - minX, height = maxY - minY;
    if (width == 0) width = 1;
    if (height == 0) height = 1;

    int fast = delTX == width && delTY == height && 
               (begTX + delTX < curTexWidth) && 
               (begTY + delTY < curTexHeight);

    minX = max(minX, 0); maxX = min(maxX, fb_maxX);
    minY = max(minY, 0); maxY = min(maxY, fb_maxY);

    int x, y;
    for (y = minY; y <= maxY; y++) 
    {
        int texY = fast ? (begTY + (y - minY)) : (((begTY + delTY * (y - minY) / height)) & texHeightMask);
        for (x = minX; x <= maxX; x++) 
        {
            int texX = fast ? (begTX + (x - minX)) : (((begTX + delTX * (x - minX) / width)) & texWidthMask);
            int texIndex = texY * curTexWidth + texX;

            BitmapCol color = curTexPixels[texIndex];
            int R, G, B, A;

            A = BitmapCol_A(color);
            if (gfx_alphaBlend && A == 0) continue;
            int cb_index = y * cb_stride + x;

            if (gfx_alphaBlend && A != 255) {
                BitmapCol dst = colorBuffer[cb_index];
                int dstR = BitmapCol_R(dst);
                int dstG = BitmapCol_G(dst);
                int dstB = BitmapCol_B(dst);

                R = BitmapCol_R(color);
                G = BitmapCol_G(color);
                B = BitmapCol_B(color);

                R = (R * A + dstR * (255 - A)) >> 8;
                G = (G * A + dstG * (255 - A)) >> 8;
                B = (B * A + dstB * (255 - A)) >> 8;
                color = BitmapCol_Make(R, G, B, 0xFF);
            }

            if (vColor != PACKEDCOL_WHITE) {
                int r1 = PackedCol_R(vColor), r2 = BitmapCol_R(color);
                R = ( r1 * r2 ) >> 8;
                int g1 = PackedCol_G(vColor), g2 = BitmapCol_G(color);
                G = ( g1 * g2 ) >> 8;
                int b1 = PackedCol_B(vColor), b2 = BitmapCol_B(color);
                B = ( b1 * b2 ) >> 8;

                color = BitmapCol_Make(R, G, B, 0xFF);
            }

            colorBuffer[cb_index] = color;
        }
    }
}

#define edgeFunctionFixed(ax,ay, bx,by, cx,cy) \
    (FixedMul((bx) - (ax), (cy) - (ay)) - FixedMul((by) - (ay), (cx) - (ax)))


#define MultiplyColors(vColor, tColor) \
    a1 = PackedCol_A(vColor); \
    a2 = BitmapCol_A(tColor); \
    A  = ( a1 * a2 ) >> 8;    \
\
    r1 = PackedCol_R(vColor); \
    r2 = BitmapCol_R(tColor); \
    R  = ( r1 * r2 ) >> 8;    \
\
    g1 = PackedCol_G(vColor); \
    g2 = BitmapCol_G(tColor); \
    G  = ( g1 * g2 ) >> 8;    \
\
    b1 = PackedCol_B(vColor); \
    b2 = BitmapCol_B(tColor); \
    B  = ( b1 * b2 ) >> 8;    \

/*
static void DrawTriangle3D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    int x0_fp = V0->x, y0_fp = V0->y;
    int x1_fp = V1->x, y1_fp = V1->y;
    int x2_fp = V2->x, y2_fp = V2->y;

    int minX = FixedToInt(min(x0_fp, min(x1_fp, x2_fp)));
    int minY = FixedToInt(min(y0_fp, min(y1_fp, y2_fp)));
    int maxX = FixedToInt(max(x0_fp, max(x1_fp, x2_fp)));
    int maxY = FixedToInt(max(y0_fp, max(y1_fp, y2_fp)));

    if (maxX < 0 || minX > fb_maxX) return;
    if (maxY < 0 || minY > fb_maxY) return;

    minX = max(minX, 0); maxX = min(maxX, fb_maxX);
    minY = max(minY, 0); maxY = min(maxY, fb_maxY);

    int area = edgeFunctionFixed(x0_fp, y0_fp, x1_fp, y1_fp, x2_fp, y2_fp);
    if (area == 0) return;

    int factor = FixedReciprocal(area);

    int w0 = V0->w, w1 = V1->w, w2 = V2->w;
    if (w0 <= 0 && w1 <= 0 && w2 <= 0) return;

    int z0 = V0->z, z1 = V1->z, z2 = V2->z;
    PackedCol color = V0->c;

    int u0 = FixedMul(V0->u, IntToFixed(curTexWidth));
    int v0 = FixedMul(V0->v, IntToFixed(curTexHeight));
    int u1 = FixedMul(V1->u, IntToFixed(curTexWidth));
    int v1 = FixedMul(V1->v, IntToFixed(curTexHeight));
    int u2 = FixedMul(V2->u, IntToFixed(curTexWidth));
    int v2 = FixedMul(V2->v, IntToFixed(curTexHeight));

    int dx01  = y0_fp - y1_fp, dy01 = x1_fp - x0_fp;
    int dx12  = y1_fp - y2_fp, dy12 = x2_fp - x1_fp;
    int dx20  = y2_fp - y0_fp, dy20 = x0_fp - x2_fp;

    int minX_fp = IntToFixed(minX) + FP_HALF;
    int minY_fp = IntToFixed(minY) + FP_HALF;

    int bc0_start = edgeFunctionFixed(x1_fp, y1_fp, x2_fp, y2_fp, minX_fp, minY_fp);
    int bc1_start = edgeFunctionFixed(x2_fp, y2_fp, x0_fp, y0_fp, minX_fp, minY_fp);
    int bc2_start = edgeFunctionFixed(x0_fp, y0_fp, x1_fp, y1_fp, minX_fp, minY_fp);


    int R, G, B, A, x, y;
    int a1, r1, g1, b1;
    int a2, r2, g2, b2;
    cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;

    if (!texturing) {
        R = PackedCol_R(color);
        G = PackedCol_G(color);
        B = PackedCol_B(color);
        A = PackedCol_A(color);
    } else if (texSinglePixel) {
        int rawY0 = FixedDiv(v0, w0);
        int rawY1 = FixedDiv(v1, w1);
        int rawY = min(rawY0, rawY1);
        int texY = (FixedToInt(rawY) + 1) & texHeightMask;
        MultiplyColors(color, curTexPixels[texY * curTexWidth]);
        texturing = false;
    }

    for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
    {
        int bc0 = bc0_start;
        int bc1 = bc1_start;
        int bc2 = bc2_start;

        for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
        {
            int ic0 = FixedMul(bc0, factor);
            int ic1 = FixedMul(bc1, factor);
            int ic2 = FixedMul(bc2, factor);
            if (ic0 < 0 || ic1 < 0 || ic2 < 0) continue;
            int db_index = y * db_stride + x;

            int w_interp = FixedMul(ic0, w0) + FixedMul(ic1, w1) + FixedMul(ic2, w2);
            if (w_interp == 0) continue;
            int w = FixedReciprocal(w_interp);
            int z_interp = FixedMul(ic0, z0) + FixedMul(ic1, z1) + FixedMul(ic2, z2);
            int z = FixedMul(z_interp, w);

            if (depthTest && (z < 0 || z > depthBuffer[db_index])) continue;
            if (!colWrite) {
                if (depthWrite) depthBuffer[db_index] = z;
                continue;
            }

            if (texturing) {
                int u_interp = FixedMul(ic0, u0) + FixedMul(ic1, u1) + FixedMul(ic2, u2);
                int v_interp = FixedMul(ic0, v0) + FixedMul(ic1, v1) + FixedMul(ic2, v2);
                int u = FixedMul(u_interp, w);
                int v = FixedMul(v_interp, w);
                
                int texX = FixedToInt(u) & texWidthMask;
                int texY = FixedToInt(v) & texHeightMask;

                int texIndex = texY * curTexWidth + texX;
                BitmapCol tColor = curTexPixels[texIndex];

                MultiplyColors(color, tColor);
            }

            if (gfx_alphaTest && A < 0x80) continue;
            if (depthWrite) depthBuffer[db_index] = z;
            int cb_index = y * cb_stride + x;
            
            if (!gfx_alphaBlend) {
                colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
                continue;
            }

            BitmapCol dst = colorBuffer[cb_index];
            int dstR = BitmapCol_R(dst);
            int dstG = BitmapCol_G(dst);
            int dstB = BitmapCol_B(dst);

            int finR = (R * A + dstR * (255 - A)) >> 8;
            int finG = (G * A + dstG * (255 - A)) >> 8;
            int finB = (B * A + dstB * (255 - A)) >> 8;
            colorBuffer[cb_index] = BitmapCol_Make(finR, finG, finB, 0xFF);
        }
    }
}
*/

static inline void MultiplyColorsInline(PackedCol vColor, BitmapCol tColor,
                                        int *outR, int *outG, int *outB, int *outA) {
    int a1 = PackedCol_A(vColor);
    int r1 = PackedCol_R(vColor);
    int g1 = PackedCol_G(vColor);
    int b1 = PackedCol_B(vColor);

    int a2 = BitmapCol_A(tColor);
    int r2 = BitmapCol_R(tColor);
    int g2 = BitmapCol_G(tColor);
    int b2 = BitmapCol_B(tColor);

    *outA = (a1 * a2) >> 8;
    *outR = (r1 * r2) >> 8;
    *outG = (g1 * g2) >> 8;
    *outB = (b1 * b2) >> 8;
}

static void DrawTriangle3D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    int x0_fp = V0->x, y0_fp = V0->y;
    int x1_fp = V1->x, y1_fp = V1->y;
    int x2_fp = V2->x, y2_fp = V2->y;

    int minX = FixedToInt(min(x0_fp, min(x1_fp, x2_fp)));
    int minY = FixedToInt(min(y0_fp, min(y1_fp, y2_fp)));
    int maxX = FixedToInt(max(x0_fp, max(x1_fp, x2_fp)));
    int maxY = FixedToInt(max(y0_fp, max(y1_fp, y2_fp)));

    if (maxX < 0 || minX > fb_maxX) return;
    if (maxY < 0 || minY > fb_maxY) return;

    minX = max(minX, 0); maxX = min(maxX, fb_maxX);
    minY = max(minY, 0); maxY = min(maxY, fb_maxY);

    int area = edgeFunctionFixed(x0_fp, y0_fp, x1_fp, y1_fp, x2_fp, y2_fp);
    if (area == 0) return;

    int factor = FixedReciprocal(area);

    int w0 = V0->w, w1 = V1->w, w2 = V2->w;
    if (w0 <= 0 && w1 <= 0 && w2 <= 0) return;

    int z0 = V0->z, z1 = V1->z, z2 = V2->z;
    PackedCol color = V0->c;

    int u0 = FixedMul(V0->u, IntToFixed(curTexWidth));
    int v0 = FixedMul(V0->v, IntToFixed(curTexHeight));
    int u1 = FixedMul(V1->u, IntToFixed(curTexWidth));
    int v1 = FixedMul(V1->v, IntToFixed(curTexHeight));
    int u2 = FixedMul(V2->u, IntToFixed(curTexWidth));
    int v2 = FixedMul(V2->v, IntToFixed(curTexHeight));

    int dx01  = y0_fp - y1_fp, dy01 = x1_fp - x0_fp;
    int dx12  = y1_fp - y2_fp, dy12 = x2_fp - x1_fp;
    int dx20  = y2_fp - y0_fp, dy20 = x0_fp - x2_fp;

    int minX_fp = IntToFixed(minX) + FP_HALF;
    int minY_fp = IntToFixed(minY) + FP_HALF;

    int bc0_start = edgeFunctionFixed(x1_fp, y1_fp, x2_fp, y2_fp, minX_fp, minY_fp);
    int bc1_start = edgeFunctionFixed(x2_fp, y2_fp, x0_fp, y0_fp, minX_fp, minY_fp);
    int bc2_start = edgeFunctionFixed(x0_fp, y0_fp, x1_fp, y1_fp, minX_fp, minY_fp);

    int R, G, B, A, x, y;
    int a1, r1, g1, b1;
    int a2, r2, g2, b2;
    cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;

    if (!texturing) {
        R = PackedCol_R(color);
        G = PackedCol_G(color);
        B = PackedCol_B(color);
        A = PackedCol_A(color);
    } else if (texSinglePixel) {
        int rawY0 = FixedDiv(v0, w0);
        int rawY1 = FixedDiv(v1, w1);
        int rawY = min(rawY0, rawY1);
        int texY = (FixedToInt(rawY) + 1) & texHeightMask;
        MultiplyColorsInline(color, curTexPixels[texY * curTexWidth], &R, &G, &B, &A);
        texturing = false;
    } else {
        a1 = PackedCol_A(color);
        r1 = PackedCol_R(color);
        g1 = PackedCol_G(color);
        b1 = PackedCol_B(color);
    }

    int step_ic0_per_x = FixedMul(dx12, factor);  // bc0 += dx12 per x -> ic0 += step_ic0_per_x
    int step_ic1_per_x = FixedMul(dx20, factor);  // bc1 += dx20
    int step_ic2_per_x = FixedMul(dx01, factor);  // bc2 += dx01

    for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
    {
        int ic0 = FixedMul(bc0_start, factor);
        int ic1 = FixedMul(bc1_start, factor);
        int ic2 = FixedMul(bc2_start, factor);

        int w_interp = FixedMul(ic0, w0) + FixedMul(ic1, w1) + FixedMul(ic2, w2);
        int step_w = FixedMul(step_ic0_per_x, w0) + FixedMul(step_ic1_per_x, w1) + FixedMul(step_ic2_per_x, w2);

        int z_interp = FixedMul(ic0, z0) + FixedMul(ic1, z1) + FixedMul(ic2, z2);
        int step_z = FixedMul(step_ic0_per_x, z0) + FixedMul(step_ic1_per_x, z1) + FixedMul(step_ic2_per_x, z2);

        int u_interp = FixedMul(ic0, u0) + FixedMul(ic1, u1) + FixedMul(ic2, u2);
        int step_u = FixedMul(step_ic0_per_x, u0) + FixedMul(step_ic1_per_x, u1) + FixedMul(step_ic2_per_x, u2);

        int v_interp = FixedMul(ic0, v0) + FixedMul(ic1, v1) + FixedMul(ic2, v2);
        int step_v = FixedMul(step_ic0_per_x, v0) + FixedMul(step_ic1_per_x, v1) + FixedMul(step_ic2_per_x, v2);

        int bc0 = bc0_start;
        int bc1 = bc1_start;
        int bc2 = bc2_start;

        for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
        {
            if (ic0 < 0 || ic1 < 0 || ic2 < 0) {
                ic0 += step_ic0_per_x; ic1 += step_ic1_per_x; ic2 += step_ic2_per_x;
                w_interp += step_w; z_interp += step_z; u_interp += step_u; v_interp += step_v;
                continue;
            }

            int db_index = y * db_stride + x;

            if (w_interp == 0) {
                // update and skip
                ic0 += step_ic0_per_x; ic1 += step_ic1_per_x; ic2 += step_ic2_per_x;
                w_interp += step_w; z_interp += step_z; u_interp += step_u; v_interp += step_v;
                continue;
            }

            int w = FixedReciprocal(w_interp);
            int z = FixedMul(z_interp, w);

            if (depthTest && (z < 0 || z > depthBuffer[db_index])) {
                // update and continue
                ic0 += step_ic0_per_x; ic1 += step_ic1_per_x; ic2 += step_ic2_per_x;
                w_interp += step_w; z_interp += step_z; u_interp += step_u; v_interp += step_v;
                continue;
            }
            if (!colWrite) {
                if (depthWrite) depthBuffer[db_index] = z;
                // update and continue
                ic0 += step_ic0_per_x; ic1 += step_ic1_per_x; ic2 += step_ic2_per_x;
                w_interp += step_w; z_interp += step_z; u_interp += step_u; v_interp += step_v;
                continue;
            }

            int Rloc = R, Gloc = G, Bloc = B, Aloc = A; // local copy (non-texturing path keeps them)

            if (texturing) {
                int u = FixedMul(u_interp, w);
                int v = FixedMul(v_interp, w);
                
                int texX = FixedToInt(u) & texWidthMask;
                int texY = FixedToInt(v) & texHeightMask;

                int texIndex = texY * curTexWidth + texX;
                BitmapCol tColor = curTexPixels[texIndex];

                int ta = BitmapCol_A(tColor);
                int tr = BitmapCol_R(tColor);
                int tg = BitmapCol_G(tColor);
                int tb = BitmapCol_B(tColor);

                Aloc = (a1 * ta) >> 8;
                Rloc = (r1 * tr) >> 8;
                Gloc = (g1 * tg) >> 8;
                Bloc = (b1 * tb) >> 8;
            }

            if (gfx_alphaTest && Aloc < 0x80) {
                // update and continue
                if (depthWrite) ; // nothing
            } else {
                if (depthWrite) depthBuffer[db_index] = z;
                int cb_index = y * cb_stride + x;
                
                if (!gfx_alphaBlend) {
                    colorBuffer[cb_index] = BitmapCol_Make(Rloc, Gloc, Bloc, 0xFF);
                } else {
                    BitmapCol dst = colorBuffer[cb_index];
                    int dstR = BitmapCol_R(dst);
                    int dstG = BitmapCol_G(dst);
                    int dstB = BitmapCol_B(dst);

                    int finR = (Rloc * Aloc + dstR * (255 - Aloc)) >> 8;
                    int finG = (Gloc * Aloc + dstG * (255 - Aloc)) >> 8;
                    int finB = (Bloc * Aloc + dstB * (255 - Aloc)) >> 8;
                    colorBuffer[cb_index] = BitmapCol_Make(finR, finG, finB, 0xFF);
                }
            }

            // update ic and interpolants
            ic0 += step_ic0_per_x; ic1 += step_ic1_per_x; ic2 += step_ic2_per_x;
            w_interp += step_w; z_interp += step_z; u_interp += step_u; v_interp += step_v;
        } // x
    } // y
}


static void DrawTriangle2D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    int x0_fp = V0->x, y0_fp = V0->y;
    int x1_fp = V1->x, y1_fp = V1->y;
    int x2_fp = V2->x, y2_fp = V2->y;

    int minX = FixedToInt(min(x0_fp, min(x1_fp, x2_fp)));
    int minY = FixedToInt(min(y0_fp, min(y1_fp, y2_fp)));
    int maxX = FixedToInt(max(x0_fp, max(x1_fp, x2_fp)));
    int maxY = FixedToInt(max(y0_fp, max(y1_fp, y2_fp)));

    if (maxX < 0 || minX > fb_maxX) return;
    if (maxY < 0 || minY > fb_maxY) return;

    minX = max(minX, 0); maxX = min(maxX, fb_maxX);
    minY = max(minY, 0); maxY = min(maxY, fb_maxY);

    int u0 = FixedMul(V0->u, IntToFixed(curTexWidth));
    int v0 = FixedMul(V0->v, IntToFixed(curTexHeight));
    int u1 = FixedMul(V1->u, IntToFixed(curTexWidth));
    int v1 = FixedMul(V1->v, IntToFixed(curTexHeight));
    int u2 = FixedMul(V2->u, IntToFixed(curTexWidth));
    int v2 = FixedMul(V2->v, IntToFixed(curTexHeight));
    PackedCol color = V0->c;

    int area = edgeFunctionFixed(x0_fp,y0_fp, x1_fp,y1_fp, x2_fp,y2_fp);
    if (area == 0) return;
    int factor = FixedReciprocal(area);

    int dx01  = y0_fp - y1_fp, dy01 = x1_fp - x0_fp;
    int dx12  = y1_fp - y2_fp, dy12 = x2_fp - x1_fp;
    int dx20  = y2_fp - y0_fp, dy20 = x0_fp - x2_fp;

    int minX_fp = IntToFixed(minX) + FP_HALF;
    int minY_fp = IntToFixed(minY) + FP_HALF;

    int bc0_start = edgeFunctionFixed(x1_fp,y1_fp, x2_fp,y2_fp, minX_fp,minY_fp);
    int bc1_start = edgeFunctionFixed(x2_fp,y2_fp, x0_fp,y0_fp, minX_fp,minY_fp);
    int bc2_start = edgeFunctionFixed(x0_fp,y0_fp, x1_fp,y1_fp, minX_fp,minY_fp);

    int x, y;
    for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
    {
        int bc0 = bc0_start;
        int bc1 = bc1_start;
        int bc2 = bc2_start;

        for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
        {
            int ic0 = FixedMul(bc0, factor);
            int ic1 = FixedMul(bc1, factor);
            int ic2 = FixedMul(bc2, factor);

            if (ic0 < 0 || ic1 < 0 || ic2 < 0) continue;
            int cb_index = y * cb_stride + x;

            int R, G, B, A;
            if (gfx_format == VERTEX_FORMAT_TEXTURED) {
                int u = FixedMul(ic0, u0) + FixedMul(ic1, u1) + FixedMul(ic2, u2);
                int v = FixedMul(ic0, v0) + FixedMul(ic1, v1) + FixedMul(ic2, v2);
                int texX = FixedToInt(u) & texWidthMask;
                int texY = FixedToInt(v) & texHeightMask;
                int texIndex = texY * curTexWidth + texX;

                BitmapCol tColor = curTexPixels[texIndex];
                int a1 = PackedCol_A(color), a2 = BitmapCol_A(tColor);
                A = ( a1 * a2 ) >> 8;
                int r1 = PackedCol_R(color), r2 = BitmapCol_R(tColor);
                R = ( r1 * r2 ) >> 8;
                int g1 = PackedCol_G(color), g2 = BitmapCol_G(tColor);
                G = ( g1 * g2 ) >> 8;
                int b1 = PackedCol_B(color), b2 = BitmapCol_B(tColor);
                B = ( b1 * b2 ) >> 8;
            } else {
                R = PackedCol_R(color);
                G = PackedCol_G(color);
                B = PackedCol_B(color);
                A = PackedCol_A(color);
            }

            if (gfx_alphaTest && A < 0x80) continue;
            if (gfx_alphaBlend && A == 0)  continue;

            if (gfx_alphaBlend && A != 255) {
                BitmapCol dst = colorBuffer[cb_index];
                int dstR = BitmapCol_R(dst);
                int dstG = BitmapCol_G(dst);
                int dstB = BitmapCol_B(dst);

                R = (R * A + dstR * (255 - A)) >> 8;
                G = (G * A + dstG * (255 - A)) >> 8;
                B = (B * A + dstB * (255 - A)) >> 8;
            }

            colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
        }
    }
}


static void ProcessClippedTriangleAndDraw(const VertexFixed* inVerts, int polyCount) {
    if (polyCount < 3) return;
    
    for (int i = 1; i + 1 < polyCount; i++) {
        VertexFixed A = inVerts[0];
        VertexFixed B = inVerts[i];
        VertexFixed C = inVerts[i + 1];

        if (faceCulling) {
            if (A.w > 0 && B.w > 0 && C.w > 0) {
                int invA = FixedReciprocal(A.w);
                int invB = FixedReciprocal(B.w);
                int invC = FixedReciprocal(C.w);

                int ax = FixedMul(A.x, invA);
                int ay = FixedMul(A.y, invA);
                int bx = FixedMul(B.x, invB);
                int by = FixedMul(B.y, invB);
                int cx = FixedMul(C.x, invC);
                int cy = FixedMul(C.y, invC);

                int area_ndc = edgeFunctionFixed(ax, ay, bx, by, cx, cy);
                if (area_ndc < 0) continue; /* 裏向き → スキップ */
            }
        }

        if (!ViewportVertex3D(&A) || !ViewportVertex3D(&B) || !ViewportVertex3D(&C)) {
            continue;
        }
        
        int minX = FixedToInt(min(A.x, min(B.x, C.x)));
        int minY = FixedToInt(min(A.y, min(B.y, C.y)));
        int maxX = FixedToInt(max(A.x, max(B.x, C.x)));
        int maxY = FixedToInt(max(A.y, max(B.y, C.y)));
        
        if (maxX < 0 || minX > fb_maxX || maxY < 0 || minY > fb_maxY) {
            continue;
        }
        
        // too small
        int area = edgeFunctionFixed(A.x, A.y, B.x, B.y, C.x, C.y);
        if (ABS(area) <= FP_ONE) continue;
        
        DrawTriangle3D(&A, &B, &C);
    }
}


enum { PLANE_LEFT=0, PLANE_RIGHT, PLANE_BOTTOM, PLANE_TOP, PLANE_NEAR, PLANE_FAR };
#define NUM_PLANES 6

static int PlaneDistFixed(const VertexFixed* v, int plane) {
    int64_t result;
    switch (plane) {
    case PLANE_LEFT:
        result = (int64_t)v->x + v->w;
        break;
    case PLANE_RIGHT:
        result = (int64_t)v->w - v->x;
        break;
    case PLANE_BOTTOM:
        result = (int64_t)v->y + v->w;
        break;
    case PLANE_TOP:
        result = (int64_t)v->w - v->y;
        break;
    case PLANE_NEAR:
        result = v->z;
        break;
    case PLANE_FAR:
        result = (int64_t)v->w - (v->z >> 2); //hacked
        break;
    default:
        return 0;
    }
    
    // Clamp to valid range
    if (result > INT_MAX) return INT_MAX;
    if (result < INT_MIN) return INT_MIN;
    return (int)result;
}

static void LerpClipFixed(VertexFixed* out, const VertexFixed* a, const VertexFixed* b, int t) {
    if (t < 0) t = 0;
    if (t > FP_ONE) t = FP_ONE;
    
    int invt = FP_ONE - t;
    
    int64_t x_interp = ((int64_t)invt * a->x + (int64_t)t * b->x) >> FP_SHIFT;
    int64_t y_interp = ((int64_t)invt * a->y + (int64_t)t * b->y) >> FP_SHIFT;
    int64_t z_interp = ((int64_t)invt * a->z + (int64_t)t * b->z) >> FP_SHIFT;
    int64_t w_interp = ((int64_t)invt * a->w + (int64_t)t * b->w) >> FP_SHIFT;
    int64_t u_interp = ((int64_t)invt * a->u + (int64_t)t * b->u) >> FP_SHIFT;
    int64_t v_interp = ((int64_t)invt * a->v + (int64_t)t * b->v) >> FP_SHIFT;
    
    // Clamp results
    out->x = (x_interp > INT_MAX) ? INT_MAX : (x_interp < INT_MIN) ? INT_MIN : (int)x_interp;
    out->y = (y_interp > INT_MAX) ? INT_MAX : (y_interp < INT_MIN) ? INT_MIN : (int)y_interp;
    out->z = (z_interp > INT_MAX) ? INT_MAX : (z_interp < INT_MIN) ? INT_MIN : (int)z_interp;
    out->w = (w_interp > INT_MAX) ? INT_MAX : (w_interp < INT_MIN) ? INT_MIN : (int)w_interp;
    out->u = (u_interp > INT_MAX) ? INT_MAX : (u_interp < INT_MIN) ? INT_MIN : (int)u_interp;
    out->v = (v_interp > INT_MAX) ? INT_MAX : (v_interp < INT_MIN) ? INT_MIN : (int)v_interp;
    
    out->c = (t < FP_HALF) ? a->c : b->c;
}

static int SafeFixedDiv(int numerator, int denominator) {
    if (denominator == 0) return FP_HALF;
    if (ABS(denominator) < 16) return FP_HALF; // Avoid extreme divisions
    
    int64_t result = ((int64_t)numerator << FP_SHIFT) / denominator;
    
    if (result > FP_ONE) return FP_ONE;
    if (result < 0) return 0;
    return (int)result;
}

static int ClipPolygonPlaneFixed(const VertexFixed* in, int inCount, VertexFixed* out, int plane) {
    int outCount = 0;
    
	// https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
    for (int i = 0; i < inCount; i++) 
	{
        const VertexFixed* cur  = &in[i];
        const VertexFixed* next = &in[(i + 1) % inCount];
        
        int dCur  = PlaneDistFixed(cur,  plane);
        int dNext = PlaneDistFixed(next, plane);
        cc_bool curIn  = dCur >= 0;
        cc_bool nextIn = dNext >= 0;
        
        if (curIn && nextIn) {
            // Both inside
            out[outCount++] = *next;
        } else if (curIn && !nextIn) {
            // Exiting
            int denom = dCur - dNext;
            int t = SafeFixedDiv(dCur, denom);
            
            VertexFixed intersection;
            LerpClipFixed(&intersection, cur, next, t);
            out[outCount++] = intersection;
        } else if (!curIn && nextIn) {
            // Entering
            int denom = dCur - dNext;
            int t = SafeFixedDiv(dCur, denom);
            
            VertexFixed intersection;
            LerpClipFixed(&intersection, cur, next, t);
            out[outCount++] = intersection;
            out[outCount++] = *next;
        }
        // Both outside
        
        if (outCount >= 15) break; // Safety limit
    }
    
    return outCount;
}

static int ClipQuadToFrustumFixed(const VertexFixed quad[4], VertexFixed buf1[16], VertexFixed buf2[16], VertexFixed** outPoly) {
    VertexFixed* src = buf1;
    VertexFixed* dst = buf2;

    for (int i = 0; i < 4; i++) src[i] = quad[i];
    int count = 4;

    // check w zero
    for (int i = 0; i < 4; i++) if (src[i].w == 0) return 0;

    for (int plane = 0; plane < NUM_PLANES; plane++) 
	{
        int newCount = ClipPolygonPlaneFixed(src, count, dst, plane);
        if (newCount == 0) return 0;

        // swap lists
        VertexFixed* tmp = src; src = dst; dst = tmp;
        count = newCount;
        if (count > 15) count = 15; // TODO prob not needed?
    }

    *outPoly = src;
    return count;
}

static cc_bool QuadFullyInsideFrustum(const VertexFixed quad[4]) {
    for (int plane = 0; plane < NUM_PLANES; plane++) 
	{
        for (int i = 0; i < 4; i++) 
		{
            if (PlaneDistFixed(&quad[i], plane) < 0) return false;
        }
    }
    return true;
}

static void DrawClippedFixed(VertexFixed quad[4]) {
	VertexFixed buf1[16], buf2[16];
    VertexFixed* outPoly;

    if (QuadFullyInsideFrustum(quad)) {
        ProcessClippedTriangleAndDraw(quad, 4);
        return;
    }

    int polyCount = ClipQuadToFrustumFixed(quad, buf1, buf2, &outPoly);
    if (polyCount > 0) {
        ProcessClippedTriangleAndDraw(outPoly, polyCount);
    }
}

void DrawQuadsFixed(int startVertex, int verticesCount, DrawHints hints) {
    VertexFixed vertices[4];
    int i, j = startVertex;

    if (gfx_rendering2D && (hints & (DRAW_HINT_SPRITE|DRAW_HINT_RECT))) {
        for (i = 0; i < verticesCount / 4; i++, j += 4) {
            TransformVertex2D(j + 0, &vertices[0]);
            TransformVertex2D(j + 1, &vertices[1]);
            TransformVertex2D(j + 2, &vertices[2]);
            DrawSprite2D(&vertices[0], &vertices[1], &vertices[2]);
        }
    } else if (gfx_rendering2D) {
        for (i = 0; i < verticesCount / 4; i++, j += 4) {
            TransformVertex2D(j + 0, &vertices[0]);
            TransformVertex2D(j + 1, &vertices[1]);
            TransformVertex2D(j + 2, &vertices[2]);
            TransformVertex2D(j + 3, &vertices[3]);
            DrawTriangle2D(&vertices[0], &vertices[2], &vertices[1]);
            DrawTriangle2D(&vertices[2], &vertices[0], &vertices[3]);
        }
    } else {
		if (tex_offseting) ShiftTextureCoords(verticesCount);

        for (i = 0; i < verticesCount / 4; i++, j += 4) {
            TransformVertex3D(j + 0, &vertices[0]);
            TransformVertex3D(j + 1, &vertices[1]);
            TransformVertex3D(j + 2, &vertices[2]);
            TransformVertex3D(j + 3, &vertices[3]);
            
            DrawClippedFixed(vertices);
        }

		if (tex_offseting) UnshiftTextureCoords(verticesCount);
    }
}
void Gfx_SetVertexFormat(VertexFormat fmt) {
    gfx_format = fmt;
    gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) { } /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
    DrawQuadsFixed(startVertex, verticesCount, hints);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
    DrawQuadsFixed(0, verticesCount, DRAW_HINT_NONE);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
    DrawQuadsFixed(startVertex, verticesCount, DRAW_HINT_NONE);
}

/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* CB_GetRow(struct Bitmap* bmp, int y, void* ctx) {
    return colorBuffer + cb_stride * y;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
    struct Bitmap bmp;
    Bitmap_Init(bmp, fb_width, fb_height, NULL);
    return Png_Encode(&bmp, output, CB_GetRow, false, NULL);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_BeginFrame(void) { }

void Gfx_EndFrame(void) {
    Rect2D r = { 0, 0, fb_width, fb_height };
    Window_DrawFramebuffer(r, &fb_bmp);
}

void Gfx_SetVSync(cc_bool vsync) {
    gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
    if (depthBuffer) DestroyBuffers();

    fb_width   = Game.Width;
    fb_height  = Game.Height;

    Window_AllocFramebuffer(&fb_bmp, Game.Width, Game.Height);
    colorBuffer = fb_bmp.scan0;
    cb_stride   = fb_bmp.width;

    depthBuffer = Mem_Alloc(fb_width * fb_height, 4, "depth buffer");
    db_stride   = fb_width;

    Gfx_SetViewport(0, 0, Game.Width, Game.Height);
    Gfx_SetScissor (0, 0, Game.Width, Game.Height);
}

void Gfx_SetViewport(int x, int y, int w, int h) {
    vp_hwidth_fp  = FloatToFixed(w / 2.0f);
    vp_hheight_fp = FloatToFixed(h / 2.0f);
}

void Gfx_SetScissor (int x, int y, int w, int h) {
    fb_maxX = x + w - 1;
    fb_maxY = y + h - 1;
}

void Gfx_GetApiInfo(cc_string* info) {
    int pointerSize = sizeof(void*) * 8;
    String_Format1(info, "-- Using software fixed-point (%i bit) --\n", &pointerSize);
    PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
