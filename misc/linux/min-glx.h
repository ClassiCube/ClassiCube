/*
 * Mesa 3-D graphics library
 * 
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef GLX_H
#define GLX_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Tokens for glXChooseVisual and glXGetConfig:
 */
#define GLX_USE_GL		1
#define GLX_BUFFER_SIZE		2
#define GLX_LEVEL		3
#define GLX_RGBA		4
#define GLX_DOUBLEBUFFER	5
#define GLX_STEREO		6
#define GLX_AUX_BUFFERS		7
#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13


typedef struct __GLXcontextRec *GLXContext;
typedef XID GLXDrawable;
/* GLX 1.3 and later */
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXFBConfigID;
typedef XID GLXContextID;
typedef XID GLXWindow;

extern XVisualInfo* glXChooseVisual( Display *dpy, int screen,
				     int *attribList );

extern GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis,
				    GLXContext shareList, Bool direct );

extern void glXDestroyContext( Display *dpy, GLXContext ctx );

extern Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable,
			    GLXContext ctx);

extern void glXSwapBuffers( Display *dpy, GLXDrawable drawable );

extern Bool glXQueryVersion( Display *dpy, int *maj, int *min );

extern Bool glXIsDirect( Display *dpy, GLXContext ctx );



/* GLX 1.1 and later */
extern const char *glXQueryExtensionsString( Display *dpy, int screen );


/* GLX 1.3 and later */
extern GLXFBConfig *glXChooseFBConfig( Display *dpy, int screen,
                                       const int *attribList, int *nitems );

extern GLXFBConfig *glXGetFBConfigs( Display *dpy, int screen,
                                     int *nelements );

extern XVisualInfo *glXGetVisualFromFBConfig( Display *dpy,
                                              GLXFBConfig config );


/*
 * ARB 2. GLX_ARB_get_proc_address
 */
typedef void (*__GLXextFuncPtr)(void);
extern __GLXextFuncPtr glXGetProcAddressARB (const char *);


/* GLX 1.4 and later */
extern void (*glXGetProcAddress(const char *procname))( void );

/* GLX 1.4 function pointer typedefs */
typedef __GLXextFuncPtr (* PFNGLXGETPROCADDRESSPROC) (const char *procName);
#ifdef __cplusplus
}
#endif

#endif
