Although ClassiCube strives to be as platform independent as possible, in some cases it will need to use system specific code

For instance:
* Texture creation for 3D graphics rendering
* Buffer allocation for audio output
* High resolution time measurement
* Window creation

For simplicity, related system specific code is grouped together as a Module (e.g. `Audio`), which can then be implemented using a backend (e.g. `WinMM`, `OpenAL`, `OpenSL ES`, etc)

#### Note: By default, ClassiCube automatically selects the recommended backends for the system. <br> It is recommended that you do *NOT* change the backends unless you know exactly what you are doing.

However, on some systems there are potentially multiple backends for a Module. For example on Windows:
* OpenGL could be used instead of Direct3D 9 for the 3D rendering backend
* SDL could be used instead of the native WinAPI for the window backend

TODO finish this

TODO introduction (explaining platform specific modules, and how classicube has to integrate with one of them)

There are two ways of changing the backend that gets used for the system:
1) Changing the defines in `Core.h`
2) Adding `-DCC_BUILD_MANUAL` to compilation flags and then manually defining module backends via additional compilation flags

When manually compiling the source code, 1) is usually the easiest. <br>
For automated scripts compiling every single commit, 2) is the recommended approach
TODO: Move this into table


### 3D Graphics backends
* CC_BUILD_D3D9 - Direct3D 9
* CC_BUILD_D3D11 - Direct3D 11
* CC_BUILD_GL - OpenGL

The OpenGL backend can be further customised:
* CC_BUILD_GL11 - (must also have CC_BUILD_GL defined)
* CC_BUILD_GLMODERN - (must also have CC_BUILD_GL defined)
* CC_BUILD_GLES (must also have CC_BUILD_GL defined)

### OpenGL context backends
* CC_BUILD_EGL
* CC_BUILD_WGL

### HTTP backends
* CC_BUILD_CURL
* CC_BUILD_HTTPCLIENT - custom HTTP client
* CC_BUILD_WININET
* CC_BUILD_CFNETWORK

### SSL backends
* CC_BUILD_SCHANNEL
* CC_BUILD_BEARSSL

### Window backends
* CC_BUILD_SDL
* CC_BUILD_X11
* CC_BUILD_WINGUI

### Platform backends
* CC_BUILD_POSIX
* CC_BUILD_WIN

TODO fill in rest