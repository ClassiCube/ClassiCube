/* Silence deprecation warnings on modern macOS/iOS */
#define GL_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION

#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_GL2
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include "Menus.h"

/* OpenGL 2.0 backend (alternative modern-ish backend) */
#include "../misc/opengl/GLCommon.h"

/* e.g. GLAPI void APIENTRY glFunction(int value); */
#define GL_FUNC(retType, name, args) GLAPI retType APIENTRY name args;
#include "../misc/opengl/GL1Funcs.h"

/* Functions must be dynamically linked on Windows */
#ifdef CC_BUILD_WIN
/* e.g. static void (APIENTRY *_glFunction)(int value); */
#undef  GL_FUNC
#define GL_FUNC(retType, name, args) static retType (APIENTRY *name) args;
#include "../misc/opengl/GL2Funcs.h"

static const struct DynamicLibSym core_funcs[] = {
	/* e.g. { DYNAMICLIB_QUOTE(name), (void**)&_ ## name }, */
	#undef  GL_FUNC
	#define GL_FUNC(retType, name, args) { DYNAMICLIB_QUOTE(name), (void**)&name },
	#include "../misc/opengl/GL2Funcs.h"
};
#else
	#include "../misc/opengl/GL2Funcs.h"
#endif

#include "../misc/opengl/GL1Macros.h"

#include "_GLShared.h"
static void GLBackend_Init(void);

static GfxResourceID white_square;
static int postProcess;
enum PostProcess { POSTPROCESS_NONE, POSTPROCESS_GRAYSCALE };
static const char* const postProcess_Names[2] = { "NONE", "GRAYSCALE" };

void Gfx_Create(void) {
	GLContext_Create();
#ifdef CC_BUILD_WIN
	GLContext_GetAll(core_funcs, Array_Elems(core_funcs));
#endif
	Gfx.BackendType = CC_GFX_BACKEND_GL2;
	
	GL_InitCommon();
	GLBackend_Init();
	Gfx_RestoreState();
	GLContext_SetVSync(gfx_vsync);
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GLuint GL_GenAndBind(GLenum target) {
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(target, id);
	return id;
}

GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	cc_uint16 indices[GFX_MAX_INDICES];
	GLuint id      = GL_GenAndBind(GL_ELEMENT_ARRAY_BUFFER);
	cc_uint32 size = count * sizeof(cc_uint16);

	fillFunc(indices, count, obj);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	return uint_to_ptr(id);
}

void Gfx_BindIb(GfxResourceID ib) { 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ptr_to_uint(ib)); 
}

void Gfx_DeleteIb(GfxResourceID* ib) {
	GLuint id = ptr_to_uint(*ib);
	if (!id) return;
	glDeleteBuffers(1, &id);
	*ib = 0;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	GLuint id = GL_GenAndBind(GL_ARRAY_BUFFER);
	return uint_to_ptr(id);
}

void Gfx_BindVb(GfxResourceID vb) { 
	glBindBuffer(GL_ARRAY_BUFFER, ptr_to_uint(vb)); 
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = ptr_to_uint(*vb);
	if (id) glDeleteBuffers(1, &id);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockVb(GfxResourceID vb) {
	glBufferData(GL_ARRAY_BUFFER, tmpSize, tmpData, GL_STATIC_DRAW);
}


/*########################################################################################################################*
*--------------------------------------------------Dynamic vertex buffers-------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocDynamicVb(VertexFormat fmt, int maxVertices) {
	GLuint id      = GL_GenAndBind(GL_ARRAY_BUFFER);
	cc_uint32 size = maxVertices * strideSizes[fmt];

	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	return uint_to_ptr(id);
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	glBindBuffer(GL_ARRAY_BUFFER, ptr_to_uint(vb)); 
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	GLuint id = ptr_to_uint(*vb);
	if (id) glDeleteBuffers(1, &id);
	*vb = 0;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	glBindBuffer(GL_ARRAY_BUFFER, ptr_to_uint(vb));
	glBufferSubData(GL_ARRAY_BUFFER, 0, tmpSize, tmpData);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	cc_uint32 size = vCount * gfx_stride;
	glBindBuffer(GL_ARRAY_BUFFER, ptr_to_uint(vb));
	glBufferSubData(GL_ARRAY_BUFFER, 0, size, vertices);
}


/*########################################################################################################################*
*------------------------------------------------------OpenGL modern------------------------------------------------------*
*#########################################################################################################################*/
#define FTR_TEXTURE_UV (1 << 0)
#define FTR_ALPHA_TEST (1 << 1)
#define FTR_TEX_OFFSET (1 << 2)
#define FTR_LINEAR_FOG (1 << 3)
#define FTR_DENSIT_FOG (1 << 4)
#define FTR_HASANY_FOG (FTR_LINEAR_FOG | FTR_DENSIT_FOG)
#define FTR_FS_MEDIUMP (1 << 7)

#define UNI_MVP_MATRIX (1 << 0)
#define UNI_TEX_OFFSET (1 << 1)
#define UNI_FOG_COL    (1 << 2)
#define UNI_FOG_END    (1 << 3)
#define UNI_FOG_DENS   (1 << 4)
#define UNI_MASK_ALL   0x1F

/* cached uniforms (cached for multiple programs */
static struct Matrix _view, _proj, _mvp;
static cc_bool gfx_texTransform;
static float _texX, _texY;
static PackedCol gfx_fogColor;
static float gfx_fogEnd = -1.0f, gfx_fogDensity = -1.0f;
static int gfx_fogMode = -1;

/* shader programs (emulate fixed function) */
static struct GLShader {
	int features;     /* what features are enabled for this shader */
	int uniforms;     /* which associated uniforms need to be resent to GPU */
	GLuint program;   /* OpenGL program ID (0 if not yet compiled) */
	int locations[5]; /* location of uniforms (not constant) */
} shaders[6 * 3] = {
	/* no fog */
	{ 0              },
	{ 0              | FTR_ALPHA_TEST },
	{ FTR_TEXTURE_UV },
	{ FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_TEXTURE_UV | FTR_TEX_OFFSET },
	{ FTR_TEXTURE_UV | FTR_TEX_OFFSET | FTR_ALPHA_TEST },
	/* linear fog */
	{ FTR_LINEAR_FOG | 0              },
	{ FTR_LINEAR_FOG | 0              | FTR_ALPHA_TEST },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_TEX_OFFSET },
	{ FTR_LINEAR_FOG | FTR_TEXTURE_UV | FTR_TEX_OFFSET | FTR_ALPHA_TEST },
	/* density fog */
	{ FTR_DENSIT_FOG | 0              },
	{ FTR_DENSIT_FOG | 0              | FTR_ALPHA_TEST },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_ALPHA_TEST },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_TEX_OFFSET },
	{ FTR_DENSIT_FOG | FTR_TEXTURE_UV | FTR_TEX_OFFSET | FTR_ALPHA_TEST },
};
static struct GLShader* gfx_activeShader;

/* Generates source code for a GLSL vertex shader, based on shader's flags */
static void GenVertexShader(const struct GLShader* shader, cc_string* dst) {
	int uv = shader->features & FTR_TEXTURE_UV;
	int tm = shader->features & FTR_TEX_OFFSET;

	String_AppendConst(dst,         "attribute vec3 in_pos;\n");
	String_AppendConst(dst,         "attribute vec4 in_col;\n");
	if (uv) String_AppendConst(dst, "attribute vec2 in_uv;\n");
	String_AppendConst(dst,         "varying vec4 out_col;\n");
	if (uv) String_AppendConst(dst, "varying vec2 out_uv;\n");
	String_AppendConst(dst,         "uniform mat4 mvp;\n");
	if (tm) String_AppendConst(dst, "uniform vec2 texOffset;\n");

	String_AppendConst(dst,         "void main() {\n");
	String_AppendConst(dst,         "  gl_Position = mvp * vec4(in_pos, 1.0);\n");
	String_AppendConst(dst,         "  out_col = in_col;\n");
	if (uv) String_AppendConst(dst, "  out_uv  = in_uv;\n");
	if (tm) String_AppendConst(dst, "  out_uv  = out_uv + texOffset;\n");
	String_AppendConst(dst,         "}");
}

static void AddPostProcessing(cc_string* dst) {
	switch (postProcess) {
	case POSTPROCESS_GRAYSCALE:
		String_AppendConst(dst, "  float gray = 0.21 * col.r + 0.71 * col.g + 0.07 * col.b;\n");
		String_AppendConst(dst, "  col = vec4(gray, gray, gray, col.a);\n");
		break;
	}
}

/* Generates source code for a GLSL fragment shader, based on shader's flags */
static void GenFragmentShader(const struct GLShader* shader, cc_string* dst) {
	int uv = shader->features & FTR_TEXTURE_UV;
	int al = shader->features & FTR_ALPHA_TEST;
	int fl = shader->features & FTR_LINEAR_FOG;
	int fd = shader->features & FTR_DENSIT_FOG;
	int fm = shader->features & FTR_HASANY_FOG;

#ifdef CC_BUILD_GLES
	int mp = shader->features & FTR_FS_MEDIUMP;
	if (mp) String_AppendConst(dst, "precision mediump float;\n");
	else    String_AppendConst(dst, "precision highp float;\n");
#endif

	String_AppendConst(dst,         "varying vec4 out_col;\n");
	if (uv) String_AppendConst(dst, "varying vec2 out_uv;\n");
	if (uv) String_AppendConst(dst, "uniform sampler2D texImage;\n");
	if (fm) String_AppendConst(dst, "uniform vec3 fogCol;\n");
	if (fl) String_AppendConst(dst, "uniform float fogEnd;\n");
	if (fd) String_AppendConst(dst, "uniform float fogDensity;\n");

	String_AppendConst(dst,         "void main() {\n");
	if (uv) String_AppendConst(dst, "  vec4 col = texture2D(texImage, out_uv) * out_col;\n");
	else    String_AppendConst(dst, "  vec4 col = out_col;\n");
	if (al) String_AppendConst(dst, "  if (col.a < 0.5) discard;\n");
	if (fm) String_AppendConst(dst, "  float depth = 1.0 / gl_FragCoord.w;\n");
	if (fl) String_AppendConst(dst, "  float f = clamp((fogEnd - depth) / fogEnd, 0.0, 1.0);\n");
	if (fd) String_AppendConst(dst, "  float f = clamp(exp(fogDensity * depth), 0.0, 1.0);\n");
	if (fm) String_AppendConst(dst, "  col.rgb = mix(fogCol, col.rgb, f);\n");
	
	if (fl || fd || fm) AddPostProcessing(dst);
	String_AppendConst(dst,         "  gl_FragColor = col;\n");
	String_AppendConst(dst,         "}");
}

/* Tries to compile GLSL shader code */
static GLint CompileShader(GLint shader, const cc_string* src) {
	const char* str = src->buffer;
	int len = src->length;
	GLint temp;

	glShaderSource(shader, 1, &str, &len);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &temp);
	return temp;
}

/* Logs information then aborts program */
static void ShaderFailed(GLint shader) {
	char logInfo[2048];
	GLint temp;
	if (!shader) Process_Abort("Failed to create shader");

	temp = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &temp);

	if (temp > 1) {
		glGetShaderInfoLog(shader, 2047, NULL, logInfo);
		logInfo[2047] = '\0';
		Window_ShowDialog("Failed to compile shader", logInfo);
	}
	Process_Abort("Failed to compile shader");
}

/* Tries to compile vertex and fragment shaders, then link into an OpenGL program */
static void CompileProgram(struct GLShader* shader) {
	char tmpBuffer[2048]; cc_string tmp;
	GLuint vs, fs, program;
	GLint temp;

	vs = glCreateShader(GL_VERTEX_SHADER);
	if (!vs) { Platform_LogConst("Failed to create vertex shader"); return; }
	
	String_InitArray(tmp, tmpBuffer);
	GenVertexShader(shader, &tmp);
	if (!CompileShader(vs, &tmp)) ShaderFailed(vs);

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fs) { Platform_LogConst("Failed to create fragment shader"); glDeleteShader(vs); return; }

	tmp.length = 0;
	GenFragmentShader(shader, &tmp);
	if (!CompileShader(fs, &tmp)) {
		/* Sometimes fails 'highp precision is not supported in fragment shader' */
		/* So try compiling shader again without highp precision */
		shader->features |= FTR_FS_MEDIUMP;

		tmp.length = 0;
		GenFragmentShader(shader, &tmp);
		if (!CompileShader(fs, &tmp)) ShaderFailed(fs);
	}


	program  = glCreateProgram();
	if (!program) Process_Abort("Failed to create program");
	shader->program = program;

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	/* Force in_pos/in_col/in_uv attributes to be bound to 0,1,2 locations */
	/* Although most browsers assign the attributes in this order anyways, */
	/* the specification does not require this. (e.g. Safari doesn't) */
	glBindAttribLocation(program, 0, "in_pos");
	glBindAttribLocation(program, 1, "in_col");
	glBindAttribLocation(program, 2, "in_uv");

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &temp);

	if (temp) {
		glDetachShader(program, vs);
		glDetachShader(program, fs);

		glDeleteShader(vs);
		glDeleteShader(fs);

		shader->locations[0] = glGetUniformLocation(program, "mvp");
		shader->locations[1] = glGetUniformLocation(program, "texOffset");
		shader->locations[2] = glGetUniformLocation(program, "fogCol");
		shader->locations[3] = glGetUniformLocation(program, "fogEnd");
		shader->locations[4] = glGetUniformLocation(program, "fogDensity");
		return;
	}
	temp = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &temp);

	if (temp > 0) {
		glGetProgramInfoLog(program, 2047, NULL, tmpBuffer);
		tmpBuffer[2047] = '\0';
		Window_ShowDialog("Failed to compile program", tmpBuffer);
	}
	Process_Abort("Failed to compile program");
}

/* Marks a uniform as changed on all programs */
static void DirtyUniform(int uniform) {
	int i;
	for (i = 0; i < Array_Elems(shaders); i++) {
		shaders[i].uniforms |= uniform;
	}
}

/* Sends changed uniforms to the GPU for current program */
static void ReloadUniforms(void) {
	struct GLShader* s = gfx_activeShader;
	if (!s) return; /* NULL if context is lost */

	if (s->uniforms & UNI_MVP_MATRIX) {
		glUniformMatrix4fv(s->locations[0], 1, false, (float*)&_mvp);
		s->uniforms &= ~UNI_MVP_MATRIX;
	}
	if ((s->uniforms & UNI_TEX_OFFSET) && (s->features & FTR_TEX_OFFSET)) {
		glUniform2f(s->locations[1], _texX, _texY);
		s->uniforms &= ~UNI_TEX_OFFSET;
	}
	if ((s->uniforms & UNI_FOG_COL) && (s->features & FTR_HASANY_FOG)) {
		glUniform3f(s->locations[2], PackedCol_R(gfx_fogColor) / 255.0f, PackedCol_G(gfx_fogColor) / 255.0f,
									 PackedCol_B(gfx_fogColor) / 255.0f);
		s->uniforms &= ~UNI_FOG_COL;
	}
	if ((s->uniforms & UNI_FOG_END) && (s->features & FTR_LINEAR_FOG)) {
		glUniform1f(s->locations[3], gfx_fogEnd);
		s->uniforms &= ~UNI_FOG_END;
	}
	if ((s->uniforms & UNI_FOG_DENS) && (s->features & FTR_DENSIT_FOG)) {
		/* See https://docs.microsoft.com/en-us/previous-versions/ms537113(v%3Dvs.85) */
		/* The equation for EXP mode is exp(-density * z), so just negate density here */
		glUniform1f(s->locations[4], -gfx_fogDensity);
		s->uniforms &= ~UNI_FOG_DENS;
	}
}

/* Switches program to one that duplicates current fixed function state */
/* Compiles program and reloads uniforms if needed */
static void SwitchProgram(void) {
	struct GLShader* shader;
	int index = 0;

	if (gfx_fogEnabled) {
		index += 6;                       /* linear fog */
		if (gfx_fogMode >= 1) index += 6; /* exp fog */
	}

	if (gfx_format == VERTEX_FORMAT_TEXTURED) index += 2;
	if (gfx_texTransform) index += 2;
	if (gfx_alphaTest)    index += 1;

	shader = &shaders[index];
	if (shader == gfx_activeShader) { ReloadUniforms(); return; }
	if (!shader->program) CompileProgram(shader);

	gfx_activeShader = shader;
	glUseProgram(shader->program);
	ReloadUniforms();
}


/*########################################################################################################################*
*---------------------------------------------------------Textures--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_BindTexture(GfxResourceID texId) {
	/* Texture 0 has different behaviour depending on backend */
	/*   Desktop OpenGL  - pure white 1x1 texture */
	/*   WebGL/OpenGL ES - pure black 1x1 texture */
	/* So for consistency, always use a 1x1 pure white texture */
	if (!texId) texId = white_square;
	glBindTexture(GL_TEXTURE_2D, ptr_to_uint(texId));
}


/*########################################################################################################################*
*-----------------------------------------------------State management----------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled) { gfx_fogEnabled = enabled; SwitchProgram(); }
void Gfx_SetFogCol(PackedCol color) {
	if (color == gfx_fogColor) return;
	gfx_fogColor = color;
	DirtyUniform(UNI_FOG_COL);
	ReloadUniforms();
}

void Gfx_SetFogDensity(float value) {
	if (gfx_fogDensity == value) return;
	gfx_fogDensity = value;
	DirtyUniform(UNI_FOG_DENS);
	ReloadUniforms();
}

void Gfx_SetFogEnd(float value) {
	if (gfx_fogEnd == value) return;
	gfx_fogEnd = value;
	DirtyUniform(UNI_FOG_END);
	ReloadUniforms();
}

void Gfx_SetFogMode(FogFunc func) {
	if (gfx_fogMode == func) return;
	gfx_fogMode = func;
	SwitchProgram();
}

static void SetAlphaTest(cc_bool enabled) { SwitchProgram(); }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	SetColorWrite(enabled & gfx_colorMask[0], enabled & gfx_colorMask[1], 
				  enabled & gfx_colorMask[2], enabled & gfx_colorMask[3]);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&_mvp, &_view, &_proj);
	DirtyUniform(UNI_MVP_MATRIX);
	ReloadUniforms();
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	Gfx_LoadMatrix(MATRIX_VIEW, view);
	Gfx_LoadMatrix(MATRIX_PROJ, proj);
	Matrix_Mul(mvp, view, proj);
}

void Gfx_EnableTextureOffset(float x, float y) {
	_texX = x; _texY = y;
	gfx_texTransform = true;
	DirtyUniform(UNI_TEX_OFFSET);
	SwitchProgram();
}

void Gfx_DisableTextureOffset(void) {
	gfx_texTransform = false;
	SwitchProgram();
}


/*########################################################################################################################*
*-------------------------------------------------------State setup-------------------------------------------------------*
*#########################################################################################################################*/
static void GLBackend_Init(void) {
#ifdef CC_BUILD_GLES
	// OpenGL ES 2.0 doesn't support custom mipmaps levels, but 3.2 does
	// Note that GL_MAJOR_VERSION and GL_MINOR_VERSION were not actually
	//  implemented until 3.0.. but hopefully older GPU drivers out there
	//  don't try and set a value even when it's unsupported
	#define _GL_MAJOR_VERSION 33307
	#define _GL_MINOR_VERSION 33308
	
	GLint major = 0, minor = 0;
	glGetIntegerv(_GL_MAJOR_VERSION, &major);
	glGetIntegerv(_GL_MINOR_VERSION, &minor);
	customMipmapsLevels = major >= 3 && minor >= 2;

	static const cc_string bgra_ext   = String_FromConst("EXT_texture_format_BGRA8888");
	static const cc_string bgra_apl = String_FromConst("APPLE_texture_format_BGRA8888");
	cc_string extensions = String_FromReadonly((const char*)_glGetString(GL_EXTENSIONS));
	
	cc_bool has_ext_bgra = String_CaselessContains(&extensions, &bgra_ext);
	cc_bool has_apl_bgra = String_CaselessContains(&extensions, &bgra_apl);
	Platform_Log2("BGRA support - Ext: %t, Apple: %t", &has_ext_bgra, &has_apl_bgra);
	convert_rgba = PIXEL_FORMAT != GL_RGBA && !has_ext_bgra && !has_apl_bgra;
#else
    customMipmapsLevels = true;
    const GLubyte* ver  = glGetString(GL_VERSION);
    int major = ver[0] - '0', minor = ver[2] - '0';
    if (major >= 2) return;

    // OpenGL 1.x.. will likely either not work or perform poorly
    cc_string str; char strBuffer[1024];
    String_InitArray_NT(str, strBuffer);
    String_Format2(&str,"Modern OpenGL build requires at least OpenGL 2.0\n" \
                        "Your system only supports OpenGL %i.%i however\n\n" \
                        "As such ClassiCube will likely perform poorly or not work\n" \
                        "It is recommended you use the Normal OpenGL build instead\n",
                        &major, &minor);
    strBuffer[str.length] = '\0';
    Window_ShowDialog("Compatibility warning", strBuffer);
#endif
}

static void DeleteShaders(void) {
	int i;
	gfx_activeShader = NULL;

	for (i = 0; i < Array_Elems(shaders); i++) {
		glDeleteProgram(shaders[i].program);
		shaders[i].program = 0;
	}
}

static void Gfx_FreeState(void) {
	FreeDefaultResources();
	DeleteShaders();
	Gfx_DeleteTexture(&white_square);
}

static void Gfx_RestoreState(void) {
	InitDefaultResources();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	gfx_format = -1;

	DirtyUniform(UNI_MASK_ALL);
	GL_ClearColor(gfx_clearColor);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);

	/* 1x1 dummy white texture */
	struct Bitmap bmp;
	BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
	Bitmap_Init(bmp, 1, 1, pixels);
	Gfx_RecreateTexture(&white_square, &bmp, 0, false);
}

cc_bool Gfx_WarnIfNecessary(void) { 
	cc_string renderer = String_FromReadonly((const char*)glGetString(GL_RENDERER));

	if (String_ContainsConst(&renderer, "llvmpipe")) {
		Chat_AddRaw("&cSoftware rendering is being used, performance will greatly suffer.");
		Chat_AddRaw("&cVSync may also not work.");
		Chat_AddRaw("&cYou may need to install video card drivers.");
		return true;
	}
	return false;
}

static int  GetPostProcess(void) { return postProcess; }
static void SetPostProcess(int v) {
	postProcess = v;
	DeleteShaders();
	SwitchProgram();
	DirtyUniform(UNI_MASK_ALL);
}

cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) {
	MenuOptionsScreen_AddEnum(s, "Post process", 
		postProcess_Names, Array_Elems(postProcess_Names),
		GetPostProcess, SetPostProcess, NULL);
	return false;
}


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;

static void GL_SetupVbColoured(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_COLOURED, uint_to_ptr( 0));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_COLOURED, uint_to_ptr(12));
}

static void GL_SetupVbTextured(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, uint_to_ptr( 0));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_TEXTURED, uint_to_ptr(12));
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, uint_to_ptr(16));
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_COLOURED, uint_to_ptr(offset     ));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_COLOURED, uint_to_ptr(offset + 12));
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, uint_to_ptr(offset     ));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_TEXTURED, uint_to_ptr(offset + 12));
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, uint_to_ptr(offset + 16));
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	if (fmt == gfx_format) return;
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];

	if (fmt == VERTEX_FORMAT_TEXTURED) {
		glEnableVertexAttribArray(2);
		gfx_setupVBFunc      = GL_SetupVbTextured;
		gfx_setupVBRangeFunc = GL_SetupVbTextured_Range;
	} else {
		glDisableVertexAttribArray(2);
		gfx_setupVBFunc      = GL_SetupVbColoured;
		gfx_setupVBRangeFunc = GL_SetupVbColoured_Range;
	}
	SwitchProgram();
}

void Gfx_DrawVb_Lines(int verticesCount) {
	gfx_setupVBFunc();
	glDrawArrays(GL_LINES, 0, verticesCount);
}

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	gfx_setupVBRangeFunc(startVertex);
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	gfx_setupVBFunc();
	glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
}

void Gfx_BindVb_Textured(GfxResourceID vb) {
	Gfx_BindVb(vb);
	GL_SetupVbTextured();
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	if (startVertex + verticesCount > GFX_MAX_VERTICES) {
		GL_SetupVbTextured_Range(startVertex);
		glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, NULL);
		GL_SetupVbTextured();
	} else {
		/* ICOUNT(startVertex) * 2 = startVertex * 3  */
		glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, uint_to_ptr(startVertex * 3));
	}
}
#endif
