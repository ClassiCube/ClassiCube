TODO finish this

TODO introduction (explaining platform specific modules)

Two ways
1) Changing the defines in `Core.h`
2) Adding `-DCC_BUILD_MANUAL` to compilation flags and manually defining module backend 

Graphics backends
CC_BUILD_GL
CC_BUILD_GLMODERN
CC_BUILD_D3D9
CC_BUILD_3D11

CC_BUILD_GL11
CC_BUILD_GLES

OpenGL context backends
CC_BUILD_EGL
CC_BUILD_WGL

HTTP backends
CC_BUILD_WININET
CC_BUILD_CURL
CC_BUILD_CFNETWORK
CC_BUILD_HTTPCLIENT

Window backends
CC_BUILD_WINGUI
CC_BUILD_X11
CC_BUILD_SDL

Platform backends
CC_BUILD_WIN
CC_BUILD_POSIX

TODO fill in rest