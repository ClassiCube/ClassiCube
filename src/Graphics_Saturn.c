#include "Core.h"
#if defined CC_BUILD_SATURN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
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

#define CMDS_COUNT 400

static PackedCol clear_color;
static vdp1_cmdt_t cmdts_all[CMDS_COUNT];
static int cmdts_count;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static vdp1_cmdt_t* NextPrimitive(void) {
	if (cmdts_count >= CMDS_COUNT) Logger_Abort("Too many VDP1 commands");
	return &cmdts_all[cmdts_count++];
}

static const vdp1_cmdt_draw_mode_t color_draw_mode = {
	.raw = 0x0000
};
static const vdp1_cmdt_draw_mode_t texture_draw_mode = {
    .cc_mode = PRIMITIVE_DRAW_MODE_GOURAUD_SHADING
};

static void UpdateVDP1Env(void) {
	vdp1_env_t env;
	vdp1_env_default_init(&env);

	int R = PackedCol_R(clear_color);
	int G = PackedCol_G(clear_color);
	int B = PackedCol_B(clear_color);
	env.erase_color = RGB1555(1, R >> 3, G >> 3, B >> 3);

	vdp1_env_set(&env);
}

static void CalcGouraudColours(void) {
	for (int i = 0; i < 1024; i++)
	{
		// 1024 = 10 bits, divided into RRR GGGG BBB
		int r_idx = (i & 0x007) >> 0, R = r_idx << (5 - 3);
		int g_idx = (i & 0x078) >> 3, G = g_idx << (5 - 4);
		int b_idx = (i & 0x380) >> 7, B = b_idx << (5 - 3);
		rgb1555_t gouraud = RGB1555(1, R, G, B);
		
		vdp1_gouraud_table_t* cur = &_vdp1_vram_partitions.gouraud_base[i];
		cur->colors[0] = gouraud;
		cur->colors[1] = gouraud;
		cur->colors[2] = gouraud;
		cur->colors[3] = gouraud;
	}
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
		// TODO less ram for gourad base
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

		UpdateVDP1Env();
		CalcGouraudColours();
	}

	Gfx.MinTexWidth  =  8;
	Gfx.MinTexHeight =  8;
	Gfx.MaxTexWidth  = 128;
	Gfx.MaxTexHeight = 128;
	Gfx.Created      = true;
}

void Gfx_Free(void) { 
	Gfx_FreeState();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
#define BGRA8_to_SATURN(src) \
	((src[2] & 0xF8) >> 3) | ((src[1] & 0xF8) << 2) | ((src[0] & 0xF8) << 7) | ((src[3] & 0x80) << 8)

static GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	cc_uint16* tmp = Mem_TryAlloc(bmp->width * bmp->height, 2);
	if (!tmp) return NULL;

	for (int y = 0; y < bmp->height; y++)
	{
		cc_uint32* src = bmp->scan0 + y * rowWidth;
		cc_uint16* dst = tmp        + y * bmp->width;
		
		for (int x = 0; x < bmp->width; x++) 
		{
			cc_uint8* color = (cc_uint8*)&src[x];
			dst[x] = BGRA8_to_SATURN(color);
			dst[x] = 0xFEEE;
		}
	}
	
	scu_dma_transfer(0, _vdp1_vram_partitions.texture_base, tmp, bmp->width * bmp->height * 2);
    scu_dma_transfer_wait(0);
	Mem_Free(tmp);
	return (void*)1;
}

void Gfx_BindTexture(GfxResourceID texId) {
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	// TODO
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

static void SetAlphaTest(cc_bool enabled) {
}

static void SetAlphaBlend(cc_bool enabled) {
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

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.01f;
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
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
void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) {

}

static void Transform(Vec3* result, struct VertexTextured* a, const struct Matrix* mat) {
	/* a could be pointing to result - therefore can't directly assign X/Y/Z */
	float x = a->x * mat->row1.x + a->y * mat->row2.x + a->z * mat->row3.x + mat->row4.x;
	float y = a->x * mat->row1.y + a->y * mat->row2.y + a->z * mat->row3.y + mat->row4.y;
	float z = a->x * mat->row1.z + a->y * mat->row2.z + a->z * mat->row3.z + mat->row4.z;
	float w = a->x * mat->row1.w + a->y * mat->row2.w + a->z * mat->row3.w + mat->row4.w;
	
	result->x = (x/w) *  (SCREEN_WIDTH  / 2); 
	result->y = (y/w) * -(SCREEN_HEIGHT / 2);
	result->z = (z/w) * 1024;
}

#define IsPointCulled(vec) vec.x < -2048 || vec.x > 2048 || vec.y < -2048 || vec.y > 2048 || vec.z < 0 || vec.z > 1024

static void DrawColouredQuads2D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex + i;

		int16_vec2_t points[4];
		points[0].x = (int)v[0].x - SCREEN_WIDTH / 2; points[0].y = (int)v[0].y - SCREEN_HEIGHT / 2;
		points[1].x = (int)v[1].x - SCREEN_WIDTH / 2; points[1].y = (int)v[1].y - SCREEN_HEIGHT / 2;
		points[2].x = (int)v[2].x - SCREEN_WIDTH / 2; points[2].y = (int)v[2].y - SCREEN_HEIGHT / 2;
		points[3].x = (int)v[3].x - SCREEN_WIDTH / 2; points[3].y = (int)v[3].y - SCREEN_HEIGHT / 2;

		int R = PackedCol_R(v->Col);
		int G = PackedCol_G(v->Col);
		int B = PackedCol_B(v->Col);

		vdp1_cmdt_t* cmd;

		cmd = NextPrimitive();
		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     RGB1555(1, R >> 3, G >> 3, B >> 3));
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

static void DrawTexturedQuads2D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;

		int16_vec2_t points[4];
		points[0].x = (int)v[0].x - SCREEN_WIDTH / 2; points[0].y = (int)v[0].y - SCREEN_HEIGHT / 2;
		points[1].x = (int)v[1].x - SCREEN_WIDTH / 2; points[1].y = (int)v[1].y - SCREEN_HEIGHT / 2;
		points[2].x = (int)v[2].x - SCREEN_WIDTH / 2; points[2].y = (int)v[2].y - SCREEN_HEIGHT / 2;
		points[3].x = (int)v[3].x - SCREEN_WIDTH / 2; points[3].y = (int)v[3].y - SCREEN_HEIGHT / 2;

		int R = PackedCol_R(v->Col);
		int G = PackedCol_G(v->Col);
		int B = PackedCol_B(v->Col);

		vdp1_cmdt_t* cmd;

		cmd = NextPrimitive();
		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     RGB1555(1, R >> 3, G >> 3, B >> 3));
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);

		/*cmd = NextPrimitive();
		vdp1_cmdt_distorted_sprite_set(cmd);
		vdp1_cmdt_char_size_set(cmd, 8, 8);
		vdp1_cmdt_char_base_set(cmd, (vdp1_vram_t)_vdp1_vram_partitions.texture_base);
		vdp1_cmdt_draw_mode_set(cmd, texture_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);*/
	}
}

static void DrawColouredQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexColoured* v = (struct VertexColoured*)gfx_vertices + startVertex + i;

		Vec3 coords[4];
		Transform(&coords[0], &v[0], &mvp);
		Transform(&coords[1], &v[1], &mvp);
		Transform(&coords[2], &v[2], &mvp);
		Transform(&coords[3], &v[3], &mvp);

		int16_vec2_t points[4];
		points[0].x = coords[0].x; points[0].y = coords[0].y;
		points[1].x = coords[1].x; points[1].y = coords[1].y;
		points[2].x = coords[2].x; points[2].y = coords[2].y;
		points[3].x = coords[3].x; points[3].y = coords[3].y;

		if (IsPointCulled(coords[0])) continue;
		if (IsPointCulled(coords[1])) continue;
		if (IsPointCulled(coords[2])) continue;
		if (IsPointCulled(coords[3])) continue;

		int R = PackedCol_R(v->Col);
		int G = PackedCol_G(v->Col);
		int B = PackedCol_B(v->Col);

		vdp1_cmdt_t* cmd;

		cmd = NextPrimitive();
		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     RGB1555(1, R >> 3, G >> 3, B >> 3));
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);
	}
}

static void DrawTexturedQuads3D(int verticesCount, int startVertex) {
	for (int i = 0; i < verticesCount; i += 4) 
	{
		struct VertexTextured* v = (struct VertexTextured*)gfx_vertices + startVertex + i;

		Vec3 coords[4];
		Transform(&coords[0], &v[0], &mvp);
		Transform(&coords[1], &v[1], &mvp);
		Transform(&coords[2], &v[2], &mvp);
		Transform(&coords[3], &v[3], &mvp);

		int16_vec2_t points[4];
		points[0].x = coords[0].x; points[0].y = coords[0].y;
		points[1].x = coords[1].x; points[1].y = coords[1].y;
		points[2].x = coords[2].x; points[2].y = coords[2].y;
		points[3].x = coords[3].x; points[3].y = coords[3].y;

		if (IsPointCulled(coords[0])) continue;
		if (IsPointCulled(coords[1])) continue;
		if (IsPointCulled(coords[2])) continue;
		if (IsPointCulled(coords[3])) continue;

		int R = PackedCol_R(v->Col);
		int G = PackedCol_G(v->Col);
		int B = PackedCol_B(v->Col);

		vdp1_cmdt_t* cmd;

		cmd = NextPrimitive();
		vdp1_cmdt_polygon_set(cmd);
		vdp1_cmdt_color_set(cmd,     RGB1555(1, R >> 3, G >> 3, B >> 3));
		vdp1_cmdt_draw_mode_set(cmd, color_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);

		/*cmd = NextPrimitive();
		vdp1_cmdt_distorted_sprite_set(cmd);
		vdp1_cmdt_char_size_set(cmd, 8, 8);
		vdp1_cmdt_char_base_set(cmd, (vdp1_vram_t)_vdp1_vram_partitions.texture_base);
		vdp1_cmdt_draw_mode_set(cmd, texture_draw_mode);
		vdp1_cmdt_vtx_set(cmd, 		 points);*/
	}
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
	if (gfx_rendering2D) {
		if (gfx_format == VERTEX_FORMAT_TEXTURED) {
			DrawTexturedQuads2D(verticesCount, startVertex);
		} else {
			DrawColouredQuads2D(verticesCount, startVertex);
		}
	} else {
		if (gfx_format == VERTEX_FORMAT_TEXTURED) {
			DrawTexturedQuads3D(verticesCount, startVertex);
		} else {
			DrawColouredQuads3D(verticesCount, startVertex);
		}
	}
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	Gfx_DrawVb_IndexedTris_Range(verticesCount, 0);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawTexturedQuads3D(verticesCount, startVertex);
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
	cmdts_count = 0;

	static const int16_vec2_t system_clip_coord  = { SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
	static const int16_vec2_t local_coord_center = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

	vdp1_cmdt_t* cmd;

	cmd = NextPrimitive();
	vdp1_cmdt_system_clip_coord_set(cmd);
	vdp1_cmdt_vtx_system_clip_coord_set(cmd, system_clip_coord);

	cmd = NextPrimitive();
	vdp1_cmdt_local_coord_set(cmd);
	vdp1_cmdt_vtx_local_coord_set(cmd, local_coord_center);
}

void Gfx_EndFrame(void) {
	Platform_LogConst("FRAME END");
	vdp1_cmdt_t* cmd;

	cmd = NextPrimitive();
	vdp1_cmdt_end_set(cmd);

	vdp1_cmdt_list_t cmdt_list;
	cmdt_list.cmdts = cmdts_all;
    cmdt_list.count = cmdts_count;
	vdp1_sync_cmdt_list_put(&cmdt_list, 0);

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

void Gfx_SetViewport(int x, int y, int w, int h) { }

void Gfx_GetApiInfo(cc_string* info) {
	String_AppendConst(info, "-- Using Saturn --\n");
	PrintMaxTextureInfo(info);
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
