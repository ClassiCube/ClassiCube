Although most of the code is platform-independent, some per-platform functionality is required.

By default I try to automatically define appropriate backends for your OS in Core.h. Define ```CC_BUILD_MANUAL``` to disable this.

Listed below are the requirements for implementing each platform-dependent file.
You should try to take advantage of existing backends when porting to other platforms.

### Platform
General platform specific functionality.

- Get exe path, start/exit process, open url in browser
- Dynamic library management
- Allocate, free, copy, set memory
- Current system time, stopwatch time
- File I/O, Directory I/O, Socket I/O
- Threading, signalable wait, mutex
- Drawing/measuring native font
- Native font text drawing and measuring
- Encrypt/decrypt data, getting command line args

Define:
- ```CC_BUILD_WIN``` - Use Win32 API (Windows)
- ```CC_BUILD_POSIX``` - Use posix API

posix note: Some functions are not covered. (stopwatch, getting exe path, open url in browser)
These must still be implemented for each operating system

### Window
Create a window, show a dialog window, set window contents, keyboard/mouse input

Also monitor size, clipboard, cursor, raw relative mouse movement (optional)

Define:
- ```CC_BUILD_WINGUI``` - Use Win32 API (Windows)
- ```CC_BUILD_X11``` - Use X11/XLib
- ```CC_BUILD_SDL``` - Use SDL library
- ```CC_BUILD_WEBCANVAS``` - Use emscripten canvas

If using OpenGL, also OpenGL context creation

Define:
- ```CC_BUILD_WGL``` - Use WGL (Windows OpenGL)
- ```CC_BUILD_GLX``` - Use glX (X11/XLib)
- ```CC_BUILD_EGL``` - Use EGL (mainly for GLES)
- ```CC_BUILD_SDL``` - Use SDL library
- ```CC_BUILD_WEBGL``` - Use emscripten WebGL

### Logger
Dump registers and backtrace, log unhandled errors (e.g. invalid memory read)

Define:
- ```CC_BUILD_WIN``` - Use Win32 api
- ```CC_BUILD_POSIX``` - Use POSIX api

posix note: Register access is highly dependent on OS and architecture.

(e.g. Linux uses &r.gregs[REG_EAX] while FreeBSD uses &r.mc_eax)

### Audio
Play multiple audio streams with varying sample rates

Define:
- ```CC_BUILD_WINMM``` - Use WinMM (Windows)
- ```CC_BUILD_OpenAL``` - Use OpenAL

### 3D Graphics
Texturing, depth buffer, alpha, etc (See Graphics.h for full list)

Define:
- ```CC_BUILD_D3D9``` - Use Direct3D9
- ```CC_BUILD_GL``` - Use OpenGL (1.5/1.2 + ARB_VERTEX_BUFFER_OBJECT)
- ```CC_BUILD_GL11``` - Use OpenGL 1.1 features only
- ```CC_BUILD_GLMODERN``` - Use modern OpenGL shaders
- ```CC_BUILD_GLES``` - Makes these shaders compatible with OpenGL ES

### Http
HTTP, HTTPS, and setting request/getting response headers

Define:
- ```CC_BUILD_WININET``` - use WinINet for http (Windows)
- ```CC_BUILD_CURL``` - use libcurl for http

Supporting connection reuse is highly recommended. (but not required)
