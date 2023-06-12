#### Note: By default, ClassiCube automatically selects the recommended backends for the platform. <br> It is recommended that you do *NOT* change the backends unless you know exactly what you are doing.

TODO finish this

TODO introduction (explaining platform specific modules, and how classicube has to integrate with one of them)

Two ways
1) Changing the defines in `Core.h`
2) Adding `-DCC_BUILD_MANUAL` to compilation flags and then manually defining module backends via additional compilation flags

When manually compiling the source code, 1) is usually the easiest. <br>
For automated scripts compiling every single commit, 2) is the recommended approach


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

### Window backends
* CC_BUILD_SDL
* CC_BUILD_X11
* CC_BUILD_WINGUI

### Platform backends
* CC_BUILD_POSIX
* CC_BUILD_WIN

TODO fill in rest