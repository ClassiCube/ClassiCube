#include "Core.h"
#if defined CC_BUILD_GL && defined CC_BUILD_GLMODERN
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
/* OpenGL 2.0 backend (alternative modern-ish backend) */

#if defined CC_BUILD_WIN
/* Avoid pointless includes */
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#include <windows.h>
#include <GL/gl.h>
#elif defined CC_BUILD_IOS
#include <OpenGLES/ES2/gl.h>
#elif defined CC_BUILD_MACOS
#include <OpenGL/gl.h>
#elif defined CC_BUILD_GLES
#include <GLES2/gl2.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

/* Windows gl.h only supplies up to OpenGL 1.1 headers */
#ifdef CC_BUILD_WIN
/* === BEGIN OPENGL HEADERS === */
#define GL_ARRAY_BUFFER          0x8892
#define GL_ELEMENT_ARRAY_BUFFER  0x8893
#define GL_STATIC_DRAW           0x88E4
#define GL_DYNAMIC_DRAW          0x88E8

#define GL_FRAGMENT_SHADER       0x8B30
#define GL_VERTEX_SHADER         0x8B31
#define GL_COMPILE_STATUS        0x8B81
#define GL_LINK_STATUS           0x8B82
#define GL_INFO_LOG_LENGTH       0x8B84

static void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
static void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint* buffers);
static void (APIENTRY *glGenBuffers)(GLsizei n, GLuint *buffers);
static void (APIENTRY *glBufferData)(GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage);
static void (APIENTRY *glBufferSubData)(GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data);

static GLuint (APIENTRY* glCreateShader)(GLenum type);
static void   (APIENTRY* glDeleteShader)(GLuint shader);
static void   (APIENTRY* glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
static void   (APIENTRY* glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
static void   (APIENTRY* glShaderSource)(GLuint shader, GLsizei count, const char* const* string, const GLint* length);

static void (APIENTRY* glAttachShader)(GLuint program, GLuint shader);
static void (APIENTRY* glBindAttribLocation)(GLuint program, GLuint index, const char* name);
static void (APIENTRY* glCompileShader)(GLuint shader);
static void (APIENTRY* glDetachShader)(GLuint program, GLuint shader);
static void (APIENTRY* glLinkProgram)(GLuint program);

static GLuint (APIENTRY* glCreateProgram)(void);
static void   (APIENTRY* glDeleteProgram)(GLuint program);
static void   (APIENTRY* glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
static void   (APIENTRY* glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
static void   (APIENTRY* glUseProgram)(GLuint program);

static void (APIENTRY *glDisableVertexAttribArray)(GLuint index);
static void (APIENTRY *glEnableVertexAttribArray)(GLuint index);
static void (APIENTRY *glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

static GLint (APIENTRY *glGetUniformLocation)(GLuint program, const char* name);
static void  (APIENTRY *glUniform1f)(GLint location, GLfloat v0);
static void  (APIENTRY *glUniform2f)(GLint location, GLfloat v0, GLfloat v1);
static void  (APIENTRY *glUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
static void  (APIENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

#define GLSym(sym) { DYNAMICLIB_QUOTE(sym), (void**)& ## sym }
static const struct DynamicLibSym core_funcs[] = {
	GLSym(glBindBuffer), GLSym(glDeleteBuffers), GLSym(glGenBuffers), GLSym(glBufferData), GLSym(glBufferSubData),
	GLSym(glCreateShader), GLSym(glDeleteShader), GLSym(glGetShaderiv), GLSym(glGetShaderInfoLog), GLSym(glShaderSource),
	GLSym(glAttachShader), GLSym(glBindAttribLocation), GLSym(glCompileShader), GLSym(glDetachShader), GLSym(glLinkProgram),
	GLSym(glCreateProgram), GLSym(glDeleteProgram), GLSym(glGetProgramiv), GLSym(glGetProgramInfoLog), GLSym(glUseProgram),
	GLSym(glDisableVertexAttribArray), GLSym(glEnableVertexAttribArray), GLSym(glVertexAttribPointer),
	GLSym(glGetUniformLocation), GLSym(glUniform1f), GLSym(glUniform2f), GLSym(glUniform3f), GLSym(glUniformMatrix4fv),
};

/* === END OPENGL HEADERS === */
#endif

#include "_GLShared.h"
static GfxResourceID white_square;


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
	return id;
}

void Gfx_BindIb(GfxResourceID ib) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)ib); }

void Gfx_DeleteIb(GfxResourceID* ib) {
	GLuint id = (GLuint)(*ib);
	if (!id) return;
	glDeleteBuffers(1, &id);
	*ib = 0;
}


/*########################################################################################################################*
*------------------------------------------------------Vertex buffers-----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return GL_GenAndBind(GL_ARRAY_BUFFER);
}

void Gfx_BindVb(GfxResourceID vb) { 
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb); 
}

void Gfx_DeleteVb(GfxResourceID* vb) {
	GLuint id = (GLuint)(*vb);
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
	return id;
}

void Gfx_BindDynamicVb(GfxResourceID vb) {
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb); 
}

void Gfx_DeleteDynamicVb(GfxResourceID* vb) {
	GLuint id = (GLuint)(*vb);
	if (id) glDeleteBuffers(1, &id);
	*vb = 0;
}

void* Gfx_LockDynamicVb(GfxResourceID vb, VertexFormat fmt, int count) {
	return FastAllocTempMem(count * strideSizes[fmt]);
}

void Gfx_UnlockDynamicVb(GfxResourceID vb) {
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb);
	glBufferSubData(GL_ARRAY_BUFFER, 0, tmpSize, tmpData);
}

void Gfx_SetDynamicVbData(GfxResourceID vb, void* vertices, int vCount) {
	cc_uint32 size = vCount * gfx_stride;
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vb);
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
static cc_bool gfx_alphaTest, gfx_texTransform;
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
	if (!shader) Logger_Abort("Failed to create shader");

	temp = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &temp);

	if (temp > 1) {
		glGetShaderInfoLog(shader, 2047, NULL, logInfo);
		logInfo[2047] = '\0';
		Window_ShowDialog("Failed to compile shader", logInfo);
	}
	Logger_Abort("Failed to compile shader");
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
	if (!program) Logger_Abort("Failed to create program");
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
	Logger_Abort("Failed to compile program");
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
	glBindTexture(GL_TEXTURE_2D, (GLuint)texId);
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

void Gfx_SetTexturing(cc_bool enabled) { }
void Gfx_SetAlphaTest(cc_bool enabled) { gfx_alphaTest = enabled; SwitchProgram(); }

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	cc_bool enabled = !depthOnly;
	Gfx_SetColWriteMask(enabled, enabled, enabled, enabled);
}


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW)       _view = *matrix;
	if (type == MATRIX_PROJECTION) _proj = *matrix;

	Matrix_Mul(&_mvp, &_view, &_proj);
	DirtyUniform(UNI_MVP_MATRIX);
	ReloadUniforms();
}
void Gfx_LoadIdentityMatrix(MatrixType type) {
	Gfx_LoadMatrix(type, &Matrix_Identity);
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
#ifdef CC_BUILD_WIN
	GLContext_GetAll(core_funcs, Array_Elems(core_funcs));
#endif

#ifdef CC_BUILD_GLES
	// OpenGL ES 2.0 doesn't support custom mipmaps levels
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

static void Gfx_FreeState(void) {
	int i;
	FreeDefaultResources();
	gfx_activeShader = NULL;

	for (i = 0; i < Array_Elems(shaders); i++) {
		glDeleteProgram(shaders[i].program);
		shaders[i].program = 0;
	}
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
cc_bool Gfx_WarnIfNecessary(void) { return false; }


/*########################################################################################################################*
*----------------------------------------------------------Drawing--------------------------------------------------------*
*#########################################################################################################################*/
typedef void (*GL_SetupVBFunc)(void);
typedef void (*GL_SetupVBRangeFunc)(int startVertex);
static GL_SetupVBFunc gfx_setupVBFunc;
static GL_SetupVBRangeFunc gfx_setupVBRangeFunc;

static void GL_SetupVbColoured(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_COLOURED, (void*)0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_COLOURED, (void*)12);
}

static void GL_SetupVbTextured(void) {
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, (void*)0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_TEXTURED, (void*)12);
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, (void*)16);
}

static void GL_SetupVbColoured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_COLOURED;
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_COLOURED, (void*)(offset));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_COLOURED, (void*)(offset + 12));
}

static void GL_SetupVbTextured_Range(int startVertex) {
	cc_uint32 offset = startVertex * SIZEOF_VERTEX_TEXTURED;
	glVertexAttribPointer(0, 3, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, (void*)(offset));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true,  SIZEOF_VERTEX_TEXTURED, (void*)(offset + 12));
	glVertexAttribPointer(2, 2, GL_FLOAT,         false, SIZEOF_VERTEX_TEXTURED, (void*)(offset + 16));
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

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex) {
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
		glDrawElements(GL_TRIANGLES, ICOUNT(verticesCount), GL_UNSIGNED_SHORT, (void*)(startVertex * 3));
	}
}
#endif
