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

#define uint_to_ptr(raw) ((void*)((cc_uintptr)(raw)))
#define ptr_to_uint(raw) ((GLuint)((cc_uintptr)(raw)))


/*########################################################################################################################*
*---------------------------------------------------------General---------------------------------------------------------*
*#########################################################################################################################*/
static void GLContext_GetAll(const struct DynamicLibSym* syms, int count) {
	int i;
	for (i = 0; i < count; i++) 
	{
		*syms[i].symAddr = GLContext_GetAddress(syms[i].name);
	}
}

static void GL_InitCommon(void) {
	_glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Gfx.MaxTexWidth);
	Gfx.MaxTexHeight = Gfx.MaxTexWidth;
	Gfx.Created      = true;
	/* necessary for android which "loses" context when window is closed */
	Gfx.LostContext  = false;
}

cc_bool Gfx_TryRestoreContext(void) {
	return GLContext_TryRestore();
}

void Gfx_Free(void) {
	Gfx_FreeState();
	GLContext_Free();
}

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
static cc_bool convert_rgba;

static void ConvertRGBA(void* dst, void* src, int numPixels) {
	cc_uint8* d = (cc_uint8*)dst;
	cc_uint8* s = (cc_uint8*)src;
	int i;

	for (i = 0; i < numPixels; i++, d += 4, s += 4) {
		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
		d[3] = s[3];
	}
}

static void CallTexSubImage2D(int lvl, int x, int y, int width, int height, void* pixels) {
	void* tmp;
	if (!convert_rgba) {
		_glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, PIXEL_FORMAT, TRANSFER_FORMAT, pixels);
		return;
	}

	tmp = Mem_TryAlloc(width * height, 4);
	if (!tmp) return;

	ConvertRGBA(tmp, pixels, width * height);
	_glTexSubImage2D(GL_TEXTURE_2D, lvl, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	Mem_Free(tmp);
}

static void CallTexImage2D(int lvl, int width, int height, void* pixels) {
	void* tmp;
	if (!convert_rgba) {
		_glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, PIXEL_FORMAT, TRANSFER_FORMAT, pixels);
		return;
	}

	tmp = Mem_TryAlloc(width * height, 4);
	if (!tmp) return;

	ConvertRGBA(tmp, pixels, width * height);
	_glTexImage2D(GL_TEXTURE_2D, lvl, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
	Mem_Free(tmp);
}

static void Gfx_DoMipmaps(int x, int y, struct Bitmap* bmp, int rowWidth, cc_bool partial) {
	BitmapCol* prev = bmp->scan0;
	BitmapCol* cur;

	int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
	int lvl, width = bmp->width, height = bmp->height;

	for (lvl = 1; lvl <= lvls; lvl++) {
		x /= 2; y /= 2;
		if (width > 1)  width /= 2;
		if (height > 1) height /= 2;

		cur = (BitmapCol*)Mem_Alloc(width * height, BITMAPCOLOR_SIZE, "mipmaps");
		GenMipmaps(width, height, cur, prev, rowWidth);

		if (partial) {
			CallTexSubImage2D(lvl, x, y, width, height, cur);
		} else {
			CallTexImage2D(lvl, width, height, cur);
		}

		if (prev != bmp->scan0) Mem_Free(prev);
		prev    = cur;
		rowWidth = width;
	}
	if (prev != bmp->scan0) Mem_Free(prev);
}

/* TODO: Use GL_UNPACK_ROW_LENGTH for Desktop OpenGL instead */
#define UPDATE_FAST_SIZE (64 * 64)
static CC_NOINLINE void UpdateTextureSlow(int x, int y, struct Bitmap* part, int rowWidth, cc_bool full) {
	BitmapCol buffer[UPDATE_FAST_SIZE];
	void* ptr = (void*)buffer;
	int count = part->width * part->height;

	/* cannot allocate memory on the stack for very big updates */
	if (count > UPDATE_FAST_SIZE) {
		ptr = Mem_Alloc(count, 4, "Gfx_UpdateTexture temp");
	}

	CopyTextureData(ptr, part->width * BITMAPCOLOR_SIZE,
					part, rowWidth   * BITMAPCOLOR_SIZE);

	if (full) {
		CallTexImage2D(0, part->width, part->height, ptr);
	} else {
		CallTexSubImage2D(0, x, y, part->width, part->height, ptr);
	}
	if (count > UPDATE_FAST_SIZE) Mem_Free(ptr);
}

GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	GfxResourceID texId = NULL;
	_glGenTextures(1, (GLuint*)&texId);
	_glBindTexture(GL_TEXTURE_2D, ptr_to_uint(texId));
	_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & TEXTURE_FLAG_BILINEAR) ? GL_LINEAR : GL_NEAREST);

	if (mipmaps) {
		_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		if (customMipmapsLevels) {
			int lvls = CalcMipmapsLevels(bmp->width, bmp->height);
			_glTexParameteri(GL_TEXTURE_2D, _GL_TEXTURE_MAX_LEVEL, lvls);
		}
	} else {
		_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & TEXTURE_FLAG_BILINEAR) ? GL_LINEAR : GL_NEAREST);
	}

	if (bmp->width == rowWidth) {
		CallTexImage2D(0, bmp->width, bmp->height, bmp->scan0);
	} else {
		UpdateTextureSlow(0, 0, bmp, rowWidth, true);
	}

	if (mipmaps) Gfx_DoMipmaps(0, 0, bmp, rowWidth, false);
	return texId;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
	_glBindTexture(GL_TEXTURE_2D, ptr_to_uint(texId));

	if (part->width == rowWidth) {
		CallTexSubImage2D(0, x, y, part->width, part->height, part->scan0);
	} else {
		UpdateTextureSlow(x, y, part, rowWidth, false);
	}

	if (mipmaps) Gfx_DoMipmaps(x, y, part, rowWidth, true);
}

void Gfx_DeleteTexture(GfxResourceID* texId) {
	GLuint id = ptr_to_uint(*texId);
	if (id) _glDeleteTextures(1, &id);
	*texId = 0;
}

void Gfx_EnableMipmaps(void) { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
static PackedCol gfx_clearColor;

void Gfx_SetFaceCulling(cc_bool enabled) {
	if (enabled) { _glEnable(GL_CULL_FACE); } else { _glDisable(GL_CULL_FACE); }
}

static void SetAlphaBlend(cc_bool enabled) { 
	if (enabled) { _glEnable(GL_BLEND); } else { _glDisable(GL_BLEND); }
}
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void GL_ClearColor(PackedCol color) {
	_glClearColor(PackedCol_R(color) / 255.0f, PackedCol_G(color) / 255.0f,
				  PackedCol_B(color) / 255.0f, PackedCol_A(color) / 255.0f);
}
void Gfx_ClearColor(PackedCol color) {
	if (color == gfx_clearColor) return;
	GL_ClearColor(color);
	gfx_clearColor = color;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	_glColorMask(r, g, b, a);
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	_glDepthMask(enabled);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	if (enabled) { _glEnable(GL_DEPTH_TEST); } else { _glDisable(GL_DEPTH_TEST); }
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glortho */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z = -2.0f / (zFar - zNear);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = -(zFar + zNear) / (zFar - zNear);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;
	float c = Cotangent(0.5f * fov);

	/* Transposed, source https://learn.microsoft.com/en-us/windows/win32/opengl/glfrustum */
	/* For a FOV based perspective matrix, left/right/top/bottom are calculated as: */
	/*   left = -c * aspect, right = c * aspect, bottom = -c, top = c */
	/* Calculations are simplified because of left/right and top/bottom symmetry */
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = -(zFar + zNear) / (zFar - zNear);
	matrix->row3.w = -1.0f;
	matrix->row4.z = -(2.0f * zFar * zNear) / (zFar - zNear);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*-----------------------------------------------------------Misc----------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* GL_GetRow(struct Bitmap* bmp, int y, void* ctx) { 
	/* OpenGL stores bitmap in bottom-up order, so flip order when saving */
	return Bitmap_GetRow(bmp, (bmp->height - 1) - y); 
}
cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	cc_result res;
	GLint vp[4];
	
	_glGetIntegerv(GL_VIEWPORT, vp); /* { x, y, width, height } */
	bmp.width  = vp[2]; 
	bmp.height = vp[3];

	bmp.scan0  = (BitmapCol*)Mem_TryAlloc(bmp.width * bmp.height, BITMAPCOLOR_SIZE);
	if (!bmp.scan0) return ERR_OUT_OF_MEMORY;
	_glReadPixels(0, 0, bmp.width, bmp.height, PIXEL_FORMAT, TRANSFER_FORMAT, bmp.scan0);

	res = Png_Encode(&bmp, output, GL_GetRow, false, NULL);
	Mem_Free(bmp.scan0);
	return res;
}

static void AppendVRAMStats(cc_string* info) {
	static const cc_string memExt = String_FromConst("GL_NVX_gpu_memory_info");
	GLint totalKb, curKb;
	float total, cur;

	/* NOTE: glGetString returns UTF8, but I just treat it as code page 437 */
	cc_string exts = String_FromReadonly((const char*)_glGetString(GL_EXTENSIONS));
	if (!String_CaselessContains(&exts, &memExt)) return;

	_glGetIntegerv(0x9048, &totalKb);
	_glGetIntegerv(0x9049, &curKb);
	if (totalKb <= 0 || curKb <= 0) return;

	total = totalKb / 1024.0f; cur = curKb / 1024.0f;
	String_Format2(info, "Video memory: %f2 MB total, %f2 free\n", &total, &cur);
}

void Gfx_GetApiInfo(cc_string* info) {
	GLint depthBits = 0;
	int pointerSize = sizeof(void*) * 8;

	_glGetIntegerv(GL_DEPTH_BITS, &depthBits);
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
	String_Format1(info, "-- Using OpenGL Modern (%i bit) --\n", &pointerSize);
#else
	String_Format1(info, "-- Using OpenGL (%i bit) --\n", &pointerSize);
#endif
	String_Format1(info, "Vendor: %c\n",     _glGetString(GL_VENDOR));
	String_Format1(info, "Renderer: %c\n",   _glGetString(GL_RENDERER));
	String_Format1(info, "GL version: %c\n", _glGetString(GL_VERSION));
	AppendVRAMStats(info);
	PrintMaxTextureInfo(info);
	String_Format1(info, "Depth buffer bits: %i\n", &depthBits);
	GLContext_GetApiInfo(info);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
	if (Gfx.Created) GLContext_SetVSync(gfx_vsync);
}

void Gfx_BeginFrame(void) { }
void Gfx_ClearBuffers(GfxBuffers buffers) {
	int targets = 0;
	if (buffers & GFX_BUFFER_COLOR) targets |= GL_COLOR_BUFFER_BIT;
	if (buffers & GFX_BUFFER_DEPTH) targets |= GL_DEPTH_BUFFER_BIT;
	
	_glClear(targets); 
}

void Gfx_EndFrame(void) {
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL1
	if (Window_IsObscured()) {
		TickReducedPerformance();
	} else {
		EndReducedPerformance();
	}
#endif
	/* TODO always run ?? */

	if (!GLContext_SwapBuffers()) Gfx_LoseContext("GLContext lost");
}

void Gfx_OnWindowResize(void) {
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	/* With cocoa backend, in some cases [NSOpenGLContext update] will actually */
	/*  call glViewport with the size of the window framebuffer */
	/*  https://github.com/glfw/glfw/issues/80 */
	/* Normally this doesn't matter, but it does when game is compiled against recent */
	/*  macOS SDK *and* the display is a high DPI display - where glViewport(width, height) */
	/*  above would otherwise result in game rendering to only 1/4 of the screen */
	/*  https://github.com/ClassiCube/ClassiCube/issues/888 */
	GLContext_Update();
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	_glViewport(x, y, w, h);
}

void Gfx_SetScissor(int x, int y, int w, int h) {
	cc_bool enabled = x != 0 || y != 0 || w != Game.Width || h != Game.Height;
	if (enabled) { _glEnable(GL_SCISSOR_TEST); } else { _glDisable(GL_SCISSOR_TEST); }

	_glScissor(x, Game.Height - h - y, w, h);
}
