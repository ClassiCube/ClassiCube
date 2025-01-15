/* Buffer functions */
GL_FUNC(void,   glBindBuffer,    (GLenum target, GLuint buffer))
GL_FUNC(void,   glDeleteBuffers, (GLsizei n, const GLuint* buffers))
GL_FUNC(void,   glGenBuffers,    (GLsizei n, GLuint *buffers))
GL_FUNC(void,   glBufferData,    (GLenum target, cc_uintptr size, const GLvoid* data, GLenum usage))
GL_FUNC(void,   glBufferSubData, (GLenum target, cc_uintptr offset, cc_uintptr size, const GLvoid* data))

/* Shader functions */
GL_FUNC(GLuint, glCreateShader,     (GLenum type))
GL_FUNC(void,   glDeleteShader,     (GLuint shader))
GL_FUNC(void,   glGetShaderiv,      (GLuint shader, GLenum pname, GLint* params))
GL_FUNC(void,   glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog))
GL_FUNC(void,   glShaderSource,     (GLuint shader, GLsizei count, const char* const* string, const GLint* length))

GL_FUNC(void,   glAttachShader,       (GLuint program, GLuint shader))
GL_FUNC(void,   glBindAttribLocation, (GLuint program, GLuint index, const char* name))
GL_FUNC(void,   glCompileShader,      (GLuint shader))
GL_FUNC(void,   glDetachShader,       (GLuint program, GLuint shader))
GL_FUNC(void,   glLinkProgram,        (GLuint program))

/* Program functions */
GL_FUNC(GLuint, glCreateProgram,     (void))
GL_FUNC(void,   glDeleteProgram,     (GLuint program))
GL_FUNC(void,   glGetProgramiv,      (GLuint program, GLenum pname, GLint* params))
GL_FUNC(void,   glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog))
GL_FUNC(void,   glUseProgram,        (GLuint program))

GL_FUNC(void,   glDisableVertexAttribArray, (GLuint index))
GL_FUNC(void,   glEnableVertexAttribArray,  (GLuint index))
GL_FUNC(void,   glVertexAttribPointer,      (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer))

GL_FUNC(GLint,  glGetUniformLocation, (GLuint program, const char* name))
GL_FUNC(void,   glUniform1f,          (GLint location, GLfloat v0))
GL_FUNC(void,   glUniform2f,          (GLint location, GLfloat v0, GLfloat v1))
GL_FUNC(void,   glUniform3f,          (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))
GL_FUNC(void,   glUniformMatrix4fv,   (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value))