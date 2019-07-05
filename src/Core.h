#ifndef CC_CORE_H
#define CC_CORE_H
/* Core fixed-size integer types, automatic platform detection, and common small structs.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

#if _MSC_VER
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#ifdef _WIN64
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int     uintptr_t;
#endif
typedef signed __int8  int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;

#define CC_INLINE inline
#define CC_NOINLINE __declspec(noinline)
#define CC_ALIGN_HINT(x) __declspec(align(x))
#ifndef CC_API
#define CC_API __declspec(dllexport, noinline)
#define CC_VAR __declspec(dllexport)
#endif
#elif __GNUC__
#include <stdint.h>
#define CC_INLINE inline
#define CC_NOINLINE __attribute__((noinline))
#define CC_ALIGN_HINT(x) __attribute__((aligned(x)))
#ifndef CC_API
#ifdef _WIN32
#define CC_API __attribute__((dllexport, noinline))
#define CC_VAR __attribute__((dllexport))
#else
#define CC_API __attribute__((visibility("default"), noinline))
#define CC_VAR __attribute__((visibility("default")))
#endif
#endif
#elif __MWERKS__
/* TODO: Is there actual attribute support for these somewhere? */
#include <stdint.h>
#define CC_INLINE inline
#define CC_NOINLINE
#define CC_ALIGN_HINT(x)
#define CC_API
#define CC_VAR
#else
#error "Unknown compiler. You'll need to add required definitions in Core.h!"
#endif

typedef uint16_t Codepoint;
#ifdef __APPLE__
/* TODO: REMOVE THIS AWFUL AWFUL HACK */
#include <stdbool.h>
#elif __cplusplus
#else
typedef uint8_t bool;
#define true 1
#define false 0
#endif

#ifndef NULL
#if __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

#define EXTENDED_BLOCKS
#ifdef EXTENDED_BLOCKS
typedef uint16_t BlockID;
#else
typedef uint8_t BlockID;
#endif

#define EXTENDED_TEXTURES
#ifdef EXTENDED_TEXTURES
typedef uint16_t TextureLoc;
#else
typedef uint8_t TextureLoc;
#endif

typedef uint8_t BlockRaw;
typedef uint8_t EntityID;
typedef uint8_t Face;
typedef uint32_t ReturnCode;
typedef uint64_t TimeMS;

typedef struct Rect2D_  { int X, Y, Width, Height; } Rect2D;
typedef struct Point2D_ { int X, Y; } Point2D;
typedef struct Size2D_  { int Width, Height; } Size2D;
typedef struct FontDesc_ { void* Handle; uint16_t Size, Style; } FontDesc;
typedef struct TextureRec_ { float U1, V1, U2, V2; } TextureRec;

/*#define CC_BUILD_GL11*/
#ifndef CC_BUILD_MANUAL
#if defined _WIN32
#define CC_BUILD_WIN
#define CC_BUILD_D3D9
#define CC_BUILD_WINGUI
#define CC_BUILD_WGL
#define CC_BUILD_WININET
#define CC_BUILD_WINMM
#elif defined __ANDROID__
#define CC_BUILD_ANDROID
#define CC_BUILD_LINUX
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_GLMODERN
#define CC_BUILD_LIBUNWIND
#define CC_BUILD_GLES
#define CC_BUILD_EGL
#elif defined __linux__
#define CC_BUILD_LINUX
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_GLX
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __APPLE__
#define CC_BUILD_OSX
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_CARBON
#define CC_BUILD_AGL
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __sun__
#define CC_BUILD_SOLARIS
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_GLX
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __FreeBSD__
#define CC_BUILD_FREEBSD
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_GLX
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __OpenBSD__
#define CC_BUILD_OPENBSD
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_GLX
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __NetBSD__
#define CC_BUILD_NETBSD
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_GLX
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __EMSCRIPTEN__
#define CC_BUILD_WEB
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_GLMODERN
#define CC_BUILD_GLES
#define CC_BUILD_WEBCANVAS
#define CC_BUILD_WEBGL
#endif
#endif

#ifdef CC_BUILD_D3D9
typedef void* GfxResourceID;
#define GFX_NULL NULL
#else
typedef uintptr_t GfxResourceID;
#define GFX_NULL 0
#endif

/* Contains the information to describe a 2D textured quad. */
struct Texture {
	GfxResourceID ID;
	int16_t X, Y; uint16_t Width, Height;
	TextureRec uv;
};
#endif
