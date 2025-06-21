Although ClassiCube strives to be as platform independent as possible, in some cases it will need to use system specific code

For instance:
* Texture creation for 3D graphics rendering
* Buffer allocation for audio output
* High resolution time measurement
* Window creation

For simplicity, related system specific code is grouped together as a Module (e.g. `Audio`), which can then be implemented using a backend (e.g. `WinMM`, `OpenAL`, `OpenSL ES`, etc)

#### Note: By default, ClassiCube automatically selects the recommended backends for the system. <br> It is recommended that you do *NOT* change the backends unless you know exactly what you are doing.

Some systems may provide multiple potential backends for a Module. For example on Windows:
* OpenGL could be used instead of Direct3D 9 for the 3D rendering backend
* SDL could be used instead of the native WinAPI for the window backend

TODO finish this

TODO introduction (explaining platform specific modules, and how classicube has to integrate with one of them)

There are two ways of changing the backend that gets used for the system:
1) Changing the default defines in `Core.h`
2) Using additional compilation flags to override default module backend(s)
3) Adding `-DCC_BUILD_MANUAL` to compilation flags and then manually defining all module backends via additional compilation flags

When manually compiling the source code, 1) is usually the easiest. <br>
For automated scripts compiling every single commit, 2) is the recommended approach
TODO: Move this into table


### 3D Graphics backends (`CC_GFX_BACKEND`)
* CC_GFX_BACKEND_SOFTGPU - Software rasteriser
* CC_GFX_BACKEND_D3D9 - Direct3D 9
* CC_GFX_BACKEND_D3D11 - Direct3D 11
* CC_GFX_BACKEND_GL1 - OpenGL 1.2/1.5
* CC_GFX_BACKEND_GL2 - OpenGL 2 (shaders)

The OpenGL backend can be further customised:
* CC_BUILD_GL11 (must be using CC_GFX_BACKEND_GL1)
* CC_BUILD_GLES (must be using CC_GFX_BACKEND_GL2)

### OpenGL context backends
* CC_BUILD_EGL
* CC_BUILD_WGL

### HTTP backends (`CC_NET_BACKEND`)
* CC_NET_BACKEND_BUILTIN - custom HTTP client
* CC_NET_BACKEND_LIBCURL
* CC_BUILD_CFNETWORK

### SSL backends (`CC_SSL_BACKEND`)
* CC_SSL_BACKEND_NONE
* CC_SSL_BACKEND_BEARSSL
* CC_SSL_BACKEND_SCHANNEL

### Window backends (`CC_WIN_BACKEND`)
* CC_WIN_BACKEND_TERMINAL
* CC_WIN_BACKEND_SDL2
* CC_WIN_BACKEND_SDL3
* CC_WIN_BACKEND_X11
* CC_WIN_BACKEND_WIN32

### Audio backends (`CC_AUD_BACKEND`)
* CC_AUD_BACKEND_OPENAL
* CC_AUD_BACKEND_WINMM
* CC_AUD_BACKEND_OPENSLES

### Platform backends
* CC_BUILD_POSIX
* CC_BUILD_WIN

TODO fill in rest
