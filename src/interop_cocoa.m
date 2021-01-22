#include "Logger.h"
#include "Platform.h"
#include "Window.h"
#include <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>

static NSOpenGLContext* ctx;
extern id viewHandle;

static NSOpenGLPixelFormat* MakePixelFormat(cc_bool fullscreen) {
	NSOpenGLPixelFormatAttribute attribs[7] = {
		NSOpenGLPFAColorSize,    0,
		NSOpenGLPFADepthSize,    24,
		NSOpenGLPFADoubleBuffer, 0, 0
	};

	attribs[1] = DisplayInfo.Depth;
	attribs[5] = fullscreen ? NSOpenGLPFAFullScreen : 0;
	return [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
}

void GLContext_Create(void) {
	NSOpenGLPixelFormat* fmt;
	fmt = MakePixelFormat(true);
	if (!fmt) {
		Platform_LogConst("Failed to create full screen pixel format.");
		Platform_LogConst("Trying again to create a non-fullscreen pixel format.");
		fmt = MakePixelFormat(false);
	}
	if (!fmt) Logger_Abort("Choosing pixel format");

	ctx = [NSOpenGLContext alloc];
	ctx = [ctx initWithFormat:fmt shareContext:NULL];
	if (!ctx) Logger_Abort("Failed to create OpenGL context");

	[ctx setView:viewHandle];
	[fmt release];
	[ctx makeCurrentContext];
	[ctx update];
}

void GLContext_Update(void) {
	// TODO: Why does this crash on resizing
	[ctx update];
}

void GLContext_Free(void) { 
	[NSOpenGLContext clearCurrentContext];
	[ctx clearDrawable];
	[ctx release];
}

cc_bool GLContext_SwapBuffers(void) {
	[ctx flushBuffer];
	return true;
}

void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) {
	int value = vsync ? 1 : 0;
	[ctx setValues:&value forParameter: NSOpenGLContextParameterSwapInterval];
}
