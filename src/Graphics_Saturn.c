#include "Core.h"
#if defined CC_BUILD_SATURN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <yaul.h>


#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224

#define PRIMITIVE_DRAW_MODE_NORMAL             (0)
#define PRIMITIVE_DRAW_MODE_MESH               (1)
#define PRIMITIVE_DRAW_MODE_SHADOW             (2)
#define PRIMITIVE_DRAW_MODE_HALF_LUMINANCE     (3)
#define PRIMITIVE_DRAW_MODE_HALF_TRANSPARENT   (4)
#define PRIMITIVE_DRAW_MODE_GOURAUD_SHADING    (5)
#define PRIMITIVE_DRAW_MODE_GOURAUD_HALF_LUM   (6)
#define PRIMITIVE_DRAW_MODE_GOURAUD_HALF_TRANS (7)
#define PRIMITIVE_DRAW_MODE_COUNT              (8)

#define PRIMITIVE_WIDTH       32
#define PRIMITIVE_HEIGHT      32
#define PRIMITIVE_HALF_WIDTH  (PRIMITIVE_WIDTH / 2)
#define PRIMITIVE_HALF_HEIGHT (PRIMITIVE_HEIGHT / 2)

#define PRIMITIVE_COLOR       RGB1555(1, 31, 0, 31)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX 0
#define ORDER_LOCAL_COORDS_INDEX       1
#define ORDER_POLYGON_INDEX            2
#define ORDER_DRAW_END_INDEX           3
#define ORDER_COUNT                    4

static PackedCol clear_color;
static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static vdp1_cmdt_draw_mode_t _primitive_draw_mode = {
	.raw = 0x0000
};

static struct {
        int8_t type;
        rgb1555_t color;
        int16_vec2_t points[4];
} _primitive;


static void UpdateVDP1Env(void) {
	vdp1_env_t env;
	vdp1_env_default_init(&env);

	int R = PackedCol_R(clear_color);
	int G = PackedCol_G(clear_color);
	int B = PackedCol_B(clear_color);
	env.erase_color = RGB1555(1, R >> 3, G >> 3, B >> 3);

	vdp1_env_set(&env);
}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        _cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00,
            sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        _cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_t *cmdts;
        cmdts = &_cmdt_list->cmdts[0];

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coord);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_primitive_init(void)
{
        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER((SCREEN_WIDTH / 2)  - PRIMITIVE_HALF_WIDTH - 1,
                                   (SCREEN_HEIGHT / 2) - PRIMITIVE_HALF_HEIGHT - 1);

        _primitive.points[0].x = 0;
        _primitive.points[0].y = PRIMITIVE_HEIGHT - 1;

        _primitive.points[1].x = PRIMITIVE_WIDTH - 1;
        _primitive.points[1].y = PRIMITIVE_HEIGHT - 1;

        _primitive.points[2].x = PRIMITIVE_WIDTH - 1;
        _primitive.points[2].y = 0;

        _primitive.points[3].x = 0;
        _primitive.points[3].y = 0;

        vdp1_cmdt_t *cmdt_local_coords;
        cmdt_local_coords = &_cmdt_list->cmdts[ORDER_LOCAL_COORDS_INDEX];

        vdp1_cmdt_local_coord_set(cmdt_local_coords);
        vdp1_cmdt_vtx_local_coord_set(cmdt_local_coords, local_coord_center);

        vdp1_cmdt_t *cmdt_polygon;
        cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX];

        vdp1_gouraud_table_t *gouraud_base;
        gouraud_base = _vdp1_vram_partitions.gouraud_base;

        gouraud_base->colors[0] = RGB1555(1, 31,  0,  0);
        gouraud_base->colors[1] = RGB1555(1,  0, 31,  0);
        gouraud_base->colors[2] = RGB1555(1,  0,  0, 31);
        gouraud_base->colors[3] = RGB1555(1, 31, 31, 31);

        vdp1_cmdt_polyline_set(cmdt_polygon);
        vdp1_cmdt_color_set(cmdt_polygon,     PRIMITIVE_COLOR);
        vdp1_cmdt_draw_mode_set(cmdt_polygon, _primitive_draw_mode);
        vdp1_cmdt_vtx_set(cmdt_polygon, &_primitive.points[0]);

        vdp1_cmdt_gouraud_base_set(cmdt_polygon, (uint32_t)gouraud_base);
}

static GfxResourceID white_square;
void Gfx_RestoreState(void) {
	InitDefaultResources();
	
	// 2x2 dummy white texture
	struct Bitmap bmp;
	BitmapCol pixels[4] = { BitmapColor_RGB(255, 0, 0), BITMAPCOLOR_WHITE, BITMAPCOLOR_WHITE, BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 2, 2, pixels);
	white_square = Gfx_CreateTexture(&bmp, 0, false);
}

void Gfx_FreeState(void) {
	FreeDefaultResources(); 
	Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
	if (!Gfx.Created) {
        vdp1_vram_partitions_get(&_vdp1_vram_partitions);
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

		UpdateVDP1Env();
		_cmdt_list_init();
		_primitive_init();
	}

	Gfx.MaxTexWidth  = 128;
	Gfx.MaxTexHeight = 128;
	Gfx.Created      = true;
Platform_LogConst("GFX SETUP");
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	return NULL;
}

void Gfx_BindTexture(GfxResourceID texId) {
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) {
	// TODO
}

void Gfx_SetAlphaTest(cc_bool enabled) {
}

void Gfx_SetAlphaBlending(cc_bool enabled) {
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

void Gfx_ClearBuffers(GfxBuffers buffers) {
}

void Gfx_ClearColor(PackedCol color) {
	if (color == clear_color) return;

	clear_color = color;
	UpdateVDP1Env();
}

void Gfx_SetDepthTest(cc_bool enabled) {
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	// TODO
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
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
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
static void* gfx_vertices;

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
	return vb;
}

void Gfx_UnlockVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}


static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	return Mem_TryAlloc(maxVertices, strideSizes[fmt]);
}

void Gfx_BindDynamicVb(GfxResourceID vb) { Gfx_BindVb(vb); }

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return vb; 
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) { 
	gfx_vertices = vb;
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) { Gfx_DeleteVb(vb); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static struct Matrix _view, _proj, mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&mvp, &_view, &_proj);
}

void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
}

void Gfx_EnableTextureOffset(float x, float y) {
	// TODO
}

void Gfx_DisableTextureOffset(void) {
	// TODO
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
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
	float zNear = 0.01f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	float c = (float)Cotangent(0.5f * fov);
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
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {

}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {

}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {

}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	return ERR_NOT_SUPPORTED;
}

cc_bool Gfx_WarnIfNecessary(void) {
	return false;
}

void Gfx_BeginFrame(void) {
Platform_LogConst("FRAME BEG");
}

void Gfx_EndFrame(void) {
Platform_LogConst("FRAME END");
                vdp1_cmdt_t *cmdt_polygon;
                cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX];

                vdp1_cmdt_polygon_set(cmdt_polygon);

                vdp1_cmdt_draw_mode_set(cmdt_polygon, _primitive_draw_mode);
                vdp1_cmdt_vtx_set(cmdt_polygon, &_primitive.points[0]);

                vdp1_sync_cmdt_list_put(_cmdt_list, 0);
                vdp1_sync_render();
                vdp1_sync();
                vdp2_sync();
                vdp2_sync_wait();

	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
}

void Gfx_OnWindowResize(void) {
	// TODO
}

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Saturn --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
