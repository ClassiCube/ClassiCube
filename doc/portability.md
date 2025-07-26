Although most of the code is platform-independent, some per-platform functionality is required.

By default `Core.h` tries to automatically define appropriate backends for your system. Define ```CC_BUILD_MANUAL``` to disable this.

Note: Updating doesn't work properly in Windows 95 or Windows 98

## Before you start
* IEEE floating-point support is required. (Can be emulated in software, but will affect performance)
* The `int` data type must be 32-bits.
* 32-bit addressing (or more) is required.
* Support for 8/16/32/64 integer types is required. (your compiler must support 64-bit arithmetic)
* At least around 2 MB of RAM is required at a minimum
* At least 128 kb for main thread stack size

In summary, the codebase can theroetically be ported to any modern-ish hardware, but not stuff like a UNIVAC machine, the SuperFX chip on the SNES, or an 8-bit microcontroller.

## Porting

Listed below are the requirements for implementing each platform-dependent file.<br>
When porting to other platforms, you should try to leverage existing backends when possible.<br>
Only cross platform backends are listed below.

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
- ```CC_BUILD_POSIX``` - Use posix API

posix note: Some functions are not covered. (stopwatch, getting exe path, open url in browser)
These must still be implemented for each operating system

### Window
Create a window, show a dialog window, set window contents, keyboard/mouse input

Also monitor size, clipboard, cursor, raw relative mouse movement (optional)

Define:
- ```DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11 ``` - Use X11/XLib (unix-ish) (glX)
- ```DEFAULT_WIN_BACKEND CC_WIN_BACKEND_SDL2``` - Use SDL2 library (SDL2)

If using OpenGL, also OpenGL context management

### Logger
Dump registers and backtrace, log unhandled errors (e.g. invalid memory read)

Define:
- ```CC_BUILD_POSIX``` - use POSIX api

posix note: Register access is highly dependent on OS and architecture.

(e.g. Linux uses &r.gregs[REG_EAX] while FreeBSD uses &r.mc_eax)

### Audio
Play multiple audio streams with varying sample rates

Define:
- ```DEFAULT_AUD_BACKEND CC_AUD_BACKEND_OPENAL``` - use OpenAL
- ```CC_BUILD_NOAUDIO``` - stub audio implementation (silent)

### 3D Graphics
Texturing, depth buffer, alpha, etc (See Graphics.h for full list)

Define:
- ```DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1``` - Use legacy OpenGL (1.5/1.2 + ARB_VERTEX_BUFFER_OBJECT)
  - ```CC_BUILD_GL11``` - Use OpenGL 1.1 features only
- ```DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2``` - Use modern OpenGL shaders
  - ```CC_BUILD_GLES``` - Makes these shaders compatible with OpenGL ES
- ```DEFAULT_GFX_BACKEND CC_GFX_BACKEND_SOFTGPU``` - Use built in software rasteriser

### HTTP
HTTP, HTTPS, and setting request/getting response headers

Define:
- ```DEFAULT_NET_BACKEND CC_NET_BACKEND_BUILTIN``` - use built in simple HTTP backend

Supporting connection reuse is highly recommended. (but not required)

### SSL
SSL and TLS support, plus basic certificate validation

Define:
- ```DEFAULT_SSL_BACKEND CC_SSL_BACKEND_BEARSSL``` - use BearSSL for SSL/TLS
