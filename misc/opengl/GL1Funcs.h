/* Raster state functions */
GL_FUNC(void, glAlphaFunc,  (GLenum func, GLfloat ref))
GL_FUNC(void, glBlendFunc,  (GLenum sfactor, GLenum dfactor))
GL_FUNC(void, glClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
GL_FUNC(void, glColorMask,  (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
GL_FUNC(void, glDepthFunc,  (GLenum func))
GL_FUNC(void, glDepthMask,  (GLboolean flag))
GL_FUNC(void, glDisable,    (GLenum cap))
GL_FUNC(void, glDisableClientState, (GLenum array))
GL_FUNC(void, glEnable,     (GLenum cap))
GL_FUNC(void, glEnableClientState, (GLenum array))

/* Fog functions */
GL_FUNC(void, glFogf,  (GLenum pname, GLfloat param))
GL_FUNC(void, glFogfv, (GLenum pname, const GLfloat* params))
GL_FUNC(void, glFogi,  (GLenum pname, GLint param))
GL_FUNC(void, glFogiv, (GLenum pname, const GLint* params))

/* Transform functions */
GL_FUNC(void, glLoadIdentity, (void))
GL_FUNC(void, glLoadMatrixf,  (const GLfloat* m))
GL_FUNC(void, glMatrixMode,   (GLenum mode))
GL_FUNC(void, glViewport,     (GLint x, GLint y, GLsizei width, GLsizei height))

/* Draw functions */
GL_FUNC(void, glDrawArrays,      (GLenum mode, GLint first, GLsizei count))
GL_FUNC(void, glDrawElements,    (GLenum mode, GLsizei count, GLenum type, const GLvoid* indices))
GL_FUNC(void, glColorPointer,    (GLint size, GLenum type, GLsizei stride, GLpointer pointer))
GL_FUNC(void, glTexCoordPointer, (GLint size, GLenum type, GLsizei stride, GLpointer pointer))
GL_FUNC(void, glVertexPointer,   (GLint size, GLenum type, GLsizei stride, GLpointer pointer))

/* Misc functions */
GL_FUNC(void, glClear,      (GLuint mask))
GL_FUNC(void, glHint,       (GLenum target, GLenum mode))
GL_FUNC(void, glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels))
GL_FUNC(void, glScissor,    (GLint x, GLint y, GLsizei width, GLsizei height))

/* Texture functions */
GL_FUNC(void, glBindTexture,    (GLenum target, GLuint texture))
GL_FUNC(void, glDeleteTextures, (GLsizei n, const GLuint* textures))
GL_FUNC(void, glGenTextures,    (GLsizei n, GLuint* textures))
GL_FUNC(void, glTexImage2D,     (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels))
GL_FUNC(void, glTexSubImage2D,  (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels))
GL_FUNC(void, glTexParameteri,  (GLenum target, GLenum pname, GLint param))

/* State get functions */
GL_FUNC(GLenum,         glGetError,    (void))
GL_FUNC(void,           glGetFloatv,   (GLenum pname, GLfloat* params))
GL_FUNC(void,           glGetIntegerv, (GLenum pname, GLint* params))
GL_FUNC(const GLubyte*, glGetString,   (GLenum name))

/* Legacy display list functions */
GL_FUNC(void,   glCallList,    (GLuint list))
GL_FUNC(void,   glDeleteLists, (GLuint list, GLsizei range))
GL_FUNC(GLuint, glGenLists,    (GLsizei range))
GL_FUNC(void,   glNewList,     (GLuint list, GLenum mode))
GL_FUNC(void,   glEndList,     (void))

/* Legacy vertex draw functions */
GL_FUNC(void,   glBegin,      (GLenum mode))
GL_FUNC(void,   glEnd,        (void))
GL_FUNC(void,   glColor4ub,   (GLubyte r, GLubyte g, GLubyte b, GLubyte a))
GL_FUNC(void,   glTexCoord2f, (float u, float v))
GL_FUNC(void,   glVertex3f,   (float x, float y, float z))