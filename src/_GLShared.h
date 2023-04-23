#define _GL_TEXTURE_MAX_LEVEL        0x813D
#define _GL_BGRA_EXT                 0x80E1
#define _GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

#if defined CC_BUILD_WEB || defined CC_BUILD_ANDROID
#define PIXEL_FORMAT GL_RGBA
#else
#define PIXEL_FORMAT _GL_BGRA_EXT
#endif

#if defined CC_BIG_ENDIAN
/* Pixels are stored in memory as A,R,G,B but GL_UNSIGNED_BYTE will interpret as B,G,R,A */
/* So use GL_UNSIGNED_INT_8_8_8_8_REV instead to remedy this */
#define TRANSFER_FORMAT _GL_UNSIGNED_INT_8_8_8_8_REV
#else
/* Pixels are stored in memory as B,G,R,A and GL_UNSIGNED_BYTE will interpret as B,G,R,A */
/* So fine to just use GL_UNSIGNED_BYTE here */
#define TRANSFER_FORMAT GL_UNSIGNED_BYTE
#endif


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void GLContext_GetAll(const struct DynamicLibSym* syms, int count) {
	int i;
	for (i = 0; i < count; i++) {
		*syms[i].symAddr = GLContext_GetAddress(syms[i].name);
	}
}

static void GL_UpdateVsync(void) {
	GLContext_SetFpsLimit(gfx_vsync, gfx_minFrameMs);
}

static void GLBackend_Init(void);
void Gfx_Create(void) {
	GLContext_Create();
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx.MaxTexWidth);
	Gfx.MaxTexHeight = Gfx.MaxTexWidth;
	Gfx.Created      = true;
	/* necessary for android which "loses" context when window is closed */
	Gfx.LostContext  = false;

	GLBackend_Init();
	Gfx_RestoreState();
	GL_UpdateVsync();
}

cc_bool Gfx_TryRestoreContext(void) {
	return GLContext_TryRestore();
}

void Gfx_Free(void) {
	Gfx_FreeState();
	GLContext_Free();
}

#define gl_Toggle(cap) if (enabled) { glEnable(cap); } else { glDisable(cap); }
static void* tmpData;
static int tmpSize;

static void* FastAllocTempMem(int size) {
	if (size > tmpSize) {
		Mem_Free(tmpData);
		tmpData = Mem_Alloc(size, 1, "Gfx_AllocTempMemory");
	}

	tmpSize = size;
	return tmpData;
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
static void Gfx_DoMipmaps(int x, int y, struct Bitmap* bmp, int rowWidth, cc_bool partial) {
	BitmapCol* prev = bmp->scan0;
	BitmapCol* cur;

	int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
	int lvl, width = bmp->width, height = bmp->height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)  width /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, 4, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);

		if (partial) {
			glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, PIXEL_FORMAT, TRANSFER_FORMAT, cur);
		} else {
			glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, PIXEL_FORMAT, TRANSFER_FORMAT, cur);
		}

		if (prev != bmp->scan0) Mem_Free(prev);
		prev    = cur;
		rowWidth = width;
	}
	if (prev != bmp->scan0) Mem_Free(prev);
}

GfxResourceID Gfx_CreateTexture(struct Bitmap* bmp, cc_uint8 flags, cc_bool mipmaps) {
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (!Math_IsPowOf2(bmp->width) || !Math_IsPowOf2(bmp->height)) {
		Logger_Abort("Textures must have power of two dimensions");
	}
	if (Gfx.LostContext) return 0;

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (customMipmapsLevels) {
			int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
			glTexParameteri(GL_TEXTURE_2D, _GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp->width, bmp->height, 0, PIXEL_FORMAT, TRANSFER_FORMAT, bmp->scan0);

	if (mipmaps) Gfx_DoMipmaps(0, 0, bmp, bmp->width, false);
	return texId;
}

#define UPDATE_FAST_SIZE (64 * 64)
static CC_NOINLINE void UpdateTextureSlow(int x, int y, struct Bitmap* part, int rowWidth) {
	BitmapCol buffer[UPDATE_FAST_SIZE];
	void* ptr = (void*)buffer;
	int count = part->width * part->height;

	/* cannot allocate memory on the stack for very big updates */
	if (count > UPDATE_FAST_SIZE) {
		ptr = Mem_Alloc(count, 4, "Gfx_UpdateTexture temp");
	}

	CopyTextureData(ptr, part->width << 2, part, rowWidth << 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->width, part->height, PIXEL_FORMAT, TRANSFER_FORMAT, ptr);
	if (count > UPDATE_FAST_SIZE) Mem_Free(ptr);
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
	/* TODO: Use GL_UNPACK_ROW_LENGTH for Desktop OpenGL */

	if (part->width == rowWidth) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, part->width, part->height, PIXEL_FORMAT, TRANSFER_FORMAT, part->scan0);
	} else {
		UpdateTextureSlow(x, y, part, rowWidth);
	}
	if (mipmaps) Gfx_DoMipmaps(x, y, part, rowWidth, true);
}

void Gfx_UpdateTexturePart(GfxResourceID texId, int x, int y, struct Bitmap* part, cc_bool mipmaps) {
	Gfx_UpdateTexture(texId, x, y, part, part->width, mipmaps);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GLuint id = (GLuint)(*texId);
	if (!id) return;
	glDeleteTextures(1, &id);
	*texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;
void Gfx_SetFaceCulling(cc_bool enabled)   { gl_Toggle(GL_CULL_FACE); }
void Gfx_SetAlphaBlending(cc_bool enabled) { gl_Toggle(GL_BLEND); }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void GL_ClearColor(PackedCol color) {
	glClearColor(PackedCol_R(color) / 255.0f, PackedCol_G(color) / 255.0f,
				 PackedCol_B(color) / 255.0f, PackedCol_A(color) / 255.0f);
}
void Gfx_ClearCol(PackedCol color) {
	if (color == gfx_clearColor) return;
	GL_ClearColor(color);
	gfx_clearColor = color;
}

void Gfx_SetColWriteMask(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	glColorMask(r, g, b, a);
}

void Gfx_SetDepthWrite(cc_bool enabled) { glDepthMask(enabled); }
void Gfx_SetDepthTest(cc_bool enabled) { gl_Toggle(GL_DEPTH_TEST); }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.X =  2.0f / width;
	matrix->row2.Y = -2.0f / height;
	matrix->row3.Z = -2.0f / (zFar - zNear);

	matrix->row4.X = -1.0f;
	matrix->row4.Y =  1.0f;
	matrix->row4.Z = -(zFar + zNear) / (zFar - zNear);
}

static double Cotangent(double x) { return Math_Cos(x) / Math_Sin(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = (float)Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.X =  c / aspect;
	matrix->row2.Y =  c;
	matrix->row3.Z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.W = -1.0f;
	matrix->row4.Z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.W =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* GL_GetRow(struct Bitmap* bmp, int y) { 
	/* OpenGL stores bitmap in bottom-up order, so flip order when saving */
	return Bitmap_GetRow(bmp, (bmp->height - 1) - y); 
}
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	cc_result res;
	GLint vp[4];
	
	glGetIntegerv(GL_VIEWPORT, vp); /* { x, y, width, height } */
	bmp.width  = vp[2]; 
	bmp.height = vp[3];

	bmp.scan0  = (BitmapCol*)Mem_TryAlloc(bmp.width * bmp.height, 4);
	if (!bmp.scan0) return ERR_OUT_OF_MEMORY;
	glReadPixels(0, 0, bmp.width, bmp.height, PIXEL_FORMAT, TRANSFER_FORMAT, bmp.scan0);

	res = Png_Encode(&bmp, output, GL_GetRow, false);
	Mem_Free(bmp.scan0);
	return res;
}

static void AppendVRAMStats(cc_string* info) {
	static const cc_string memExt = String_FromConst("GL_NVX_gpu_memory_info");
	GLint totalKb, curKb;
	float total, cur;

	/* NOTE: glGetString returns UTF8, but I just treat it as code page 437 */
	cc_string exts = String_FromReadonly((const char*)glGetString(GL_EXTENSIONS));
	if (!String_CaselessContains(&exts, &memExt)) return;

	glGetIntegerv(0x9048, &totalKb);
	glGetIntegerv(0x9049, &curKb);
	if (totalKb <= 0 || curKb <= 0) return;

	total = totalKb / 1024.0f; cur = curKb / 1024.0f;
	String_Format2(info, "Video memory: %f2 MB total, %f2 free\n", &total, &cur);
}

void Gfx_GetApiInfo(cc_string* info) {
	GLint depthBits;
	int pointerSize = sizeof(void*) * 8;

	glGetIntegerv(GL_DEPTH_BITS, &depthBits);
#ifdef CC_BUILD_GLMODERN
	String_Format1(info, "-- Using OpenGL Modern (%i bit) --\n", &pointerSize);
#else
	String_Format1(info, "-- Using OpenGL (%i bit) --\n", &pointerSize);
#endif
	String_Format1(info, "Vendor: %c\n",     glGetString(GL_VENDOR));
	String_Format1(info, "Renderer: %c\n",   glGetString(GL_RENDERER));
	String_Format1(info, "GL version: %c\n", glGetString(GL_VERSION));
	AppendVRAMStats(info);
	String_Format2(info, "Max texture size: (%i, %i)\n", &Gfx.MaxTexWidth, &Gfx.MaxTexHeight);
	String_Format1(info, "Depth buffer bits: %i\n",      &depthBits);
	GLContext_GetApiInfo(info);
}

void Gfx_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	gfx_minFrameMs = minFrameMs;
	gfx_vsync      = vsync;
	if (Gfx.Created) GL_UpdateVsync();
}

void Gfx_BeginFrame(void) { }
void Gfx_Clear(void) { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void Gfx_EndFrame(void) {
#ifndef CC_BUILD_GLMODERN
	if (Window_IsObscured()) {
		TickReducedPerformance();
	} else {
		EndReducedPerformance();
	}
#endif

	if (!GLContext_SwapBuffers()) Gfx_LoseContext("GLContext lost");
	if (gfx_minFrameMs) LimitFPS();
}

void Gfx_OnWindowResize(void) {
	/* TODO: Eliminate this nasty hack.. */
	Game_UpdateDimensions();
	glViewport(0, 0, Game.Width, Game.Height);
	
	/* With cocoa backend, in some cases [NSOpenGLContext update] will actually */
	/*  call glViewport with the size of the window framebuffer */
	/*  https://github.com/glfw/glfw/issues/80 */
	/* Normally this doesn't matter, but it does when game is compiled against recent */
	/*  macOS SDK *and* the display is a high DPI display - where glViewport(width, height) */
	/*  above would otherwise result in game rendering to only 1/4 of the screen */
	/*  https://github.com/UnknownShadow200/ClassiCube/issues/888 */
	GLContext_Update();
}
