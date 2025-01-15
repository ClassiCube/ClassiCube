/* Raster state functions */
#define _glAlphaFunc  glAlphaFunc
#define _glBlendFunc  glBlendFunc
#define _glClearColor glClearColor
#define _glColorMask  glColorMask
#define _glDepthFunc  glDepthFunc
#define _glDepthMask  glDepthMask
#define _glDisable    glDisable
#define _glDisableClientState glDisableClientState
#define _glEnable     glEnable
#define _glEnableClientState glEnableClientState

/* Fog functions */
#define _glFogf  glFogf
#define _glFogfv glFogfv
#define _glFogi  glFogi
#define _glFogiv glFogiv

/* Transform functions */
#define _glLoadIdentity glLoadIdentity
#define _glLoadMatrixf  glLoadMatrixf
#define _glMatrixMode   glMatrixMode
#define _glViewport     glViewport

/* Draw functions */
#define _glDrawArrays      glDrawArrays
#define _glDrawElements    glDrawElements
#define _glColorPointer    glColorPointer
#define _glTexCoordPointer glTexCoordPointer
#define _glVertexPointer   glVertexPointer

/* Misc functions */
#define _glClear      glClear
#define _glHint       glHint
#define _glReadPixels glReadPixels
#define _glScissor    glScissor

/* Texture functions */
#define _glBindTexture    glBindTexture
#define _glDeleteTextures glDeleteTextures
#define _glGenTextures    glGenTextures
#define _glTexImage2D     glTexImage2D
#define _glTexSubImage2D  glTexSubImage2D
#define _glTexParameteri  glTexParameteri

/* State get functions */
#define _glGetError    glGetError
#define _glGetFloatv   glGetFloatv
#define _glGetIntegerv glGetIntegerv
#define _glGetString   glGetString

/* Legacy display list functions */
#define _glCallList    glCallList
#define _glDeleteLists glDeleteLists
#define _glGenLists    glGenLists
#define _glNewList     glNewList
#define _glEndList     glEndList

/* Legacy vertex draw functions */
#define _glBegin      glBegin
#define _glEnd        glEnd
#define _glColor4ub   glColor4ub
#define _glTexCoord2f glTexCoord2f
#define _glVertex3f   glVertex3f