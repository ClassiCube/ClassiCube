#ifndef CC_CORE_H
#define CC_CORE_H
/* 
Core fixed-size integer types, automatic platform detection, and common small structs
Copyright 2014-2022 ClassiCube | Licensed under BSD-3
*/

#if _MSC_VER
typedef signed __int8  cc_int8;
typedef signed __int16 cc_int16;
typedef signed __int32 cc_int32;
typedef signed __int64 cc_int64;

typedef unsigned __int8  cc_uint8;
typedef unsigned __int16 cc_uint16;
typedef unsigned __int32 cc_uint32;
typedef unsigned __int64 cc_uint64;
#ifdef _WIN64
typedef unsigned __int64 cc_uintptr;
#else
typedef unsigned __int32 cc_uintptr;
#endif

#define CC_INLINE inline
#define CC_NOINLINE __declspec(noinline)
#ifndef CC_API
#define CC_API __declspec(dllexport, noinline)
#define CC_VAR __declspec(dllexport)
#endif

#define CC_HAS_TYPES
#define CC_HAS_MISC
#elif __GNUC__
/* really old GCC/clang might not have these defined */
#ifdef __INT8_TYPE__
/* avoid including <stdint.h> because it breaks defining UNICODE in Platform.c with MinGW */
typedef __INT8_TYPE__  cc_int8;
typedef __INT16_TYPE__ cc_int16;
typedef __INT32_TYPE__ cc_int32;
typedef __INT64_TYPE__ cc_int64;

#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__   cc_uint8;
typedef __UINT16_TYPE__  cc_uint16;
typedef __UINT32_TYPE__  cc_uint32;
typedef __UINT64_TYPE__  cc_uint64;
typedef __UINTPTR_TYPE__ cc_uintptr;
#else
/* clang doesn't define the __UINT8_TYPE__ */
typedef unsigned __INT8_TYPE__   cc_uint8;
typedef unsigned __INT16_TYPE__  cc_uint16;
typedef unsigned __INT32_TYPE__  cc_uint32;
typedef unsigned __INT64_TYPE__  cc_uint64;
typedef unsigned __INTPTR_TYPE__ cc_uintptr;
#endif
#define CC_HAS_TYPES
#endif

#define CC_INLINE inline
#define CC_NOINLINE __attribute__((noinline))
#ifndef CC_API
#ifdef _WIN32
#define CC_API __attribute__((dllexport, noinline))
#define CC_VAR __attribute__((dllexport))
#else
#define CC_API __attribute__((visibility("default"), noinline))
#define CC_VAR __attribute__((visibility("default")))
#endif
#endif
#define CC_HAS_MISC
#ifdef __BIG_ENDIAN__
#define CC_BIG_ENDIAN
#endif
#elif __MWERKS__
/* TODO: Is there actual attribute support for CC_API etc somewhere? */
#define CC_BIG_ENDIAN
#endif

/* Unrecognised compiler, so just go with some sensible default typdefs */
/* Don't use <stdint.h>, as good chance such a compiler doesn't support it */
#ifndef CC_HAS_TYPES
typedef signed char  cc_int8;
typedef signed short cc_int16;
typedef signed int   cc_int32;
typedef signed long long cc_int64;

typedef unsigned char  cc_uint8;
typedef unsigned short cc_uint16;
typedef unsigned int   cc_uint32;
typedef unsigned long long cc_uint64;
typedef unsigned long  cc_uintptr;
#endif
#ifndef CC_HAS_MISC
#define CC_INLINE
#define CC_NOINLINE
#define CC_API
#define CC_VAR
#endif

typedef cc_uint32 cc_codepoint;
typedef cc_uint16 cc_unichar;
typedef cc_uint8  cc_bool;
#ifdef __APPLE__
/* TODO: REMOVE THIS AWFUL AWFUL HACK */
#include <stdbool.h>
#elif __cplusplus
#else
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


#define CC_BUILD_FREETYPE
/*#define CC_BUILD_GL11*/
#ifndef CC_BUILD_MANUAL
#if defined _WIN32
#define CC_BUILD_WIN
#define CC_BUILD_D3D9
#define CC_BUILD_WINGUI
#define CC_BUILD_WININET
#define CC_BUILD_WINMM
#elif defined __ANDROID__
#define CC_BUILD_ANDROID
#define CC_BUILD_MOBILE
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_GLMODERN
#define CC_BUILD_GLES
#define CC_BUILD_EGL
#define CC_BUILD_TOUCH
#define CC_BUILD_OPENSLES
#elif defined __serenity__
#define CC_BUILD_SERENITY
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_SDL
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __linux__
#define CC_BUILD_LINUX
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_XINPUT2
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#if defined CC_BUILD_RPI
#define CC_BUILD_GLMODERN
#define CC_BUILD_GLES
#define CC_BUILD_EGL
#endif
#elif defined __APPLE__
#define CC_BUILD_DARWIN
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#if defined __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
#define CC_BUILD_MOBILE
#define CC_BUILD_GLES
#define CC_BUILD_GLMODERN
#define CC_BUILD_IOS
#define CC_BUILD_TOUCH
#define CC_BUILD_CFNETWORK
#elif defined __x86_64__ || defined __arm64__
#define CC_BUILD_COCOA
#define CC_BUILD_MACOS
#define CC_BUILD_CURL
#else
#define CC_BUILD_CARBON
#define CC_BUILD_MACOS
#define CC_BUILD_CURL
#endif
#define CC_BUILD_OPENAL
#elif defined __sun__
#define CC_BUILD_SOLARIS
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_XINPUT2
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __FreeBSD__ || defined __DragonFly__
#define CC_BUILD_FREEBSD
#define CC_BUILD_POSIX
#define CC_BUILD_BSD
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_XINPUT2
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __OpenBSD__
#define CC_BUILD_OPENBSD
#define CC_BUILD_POSIX
#define CC_BUILD_BSD
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_XINPUT2
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __NetBSD__
#define CC_BUILD_NETBSD
#define CC_BUILD_POSIX
#define CC_BUILD_BSD
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_XINPUT2
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __HAIKU__
#define CC_BUILD_HAIKU
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#elif defined __BEOS__
#define CC_BUILD_BEOS
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_GL11
#define CC_BUILD_HTTPCLIENT
#define CC_BUILD_OPENAL
#elif defined __sgi
#define CC_BUILD_IRIX
#define CC_BUILD_POSIX
#define CC_BUILD_GL
#define CC_BUILD_X11
#define CC_BUILD_CURL
#define CC_BUILD_OPENAL
#define CC_BIG_ENDIAN
#elif defined __EMSCRIPTEN__
#define CC_BUILD_WEB
#define CC_BUILD_GL
#define CC_BUILD_GLMODERN
#define CC_BUILD_GLES
#define CC_BUILD_TOUCH
#define CC_BUILD_WEBAUDIO
#define CC_BUILD_NOMUSIC
#define CC_BUILD_MINFILES
#undef  CC_BUILD_FREETYPE
#elif defined __psp__
#define CC_BUILD_HTTPCLIENT
#define CC_BUILD_OPENAL
#define CC_BUILD_PSP
#define CC_BUILD_LOWMEM
#undef CC_BUILD_FREETYPE
#elif defined __3DS__
#define CC_BUILD_HTTPCLIENT
#define CC_BUILD_OPENAL
#define CC_BUILD_3DS
#define CC_BUILD_LOWMEM
#undef CC_BUILD_FREETYPE
#elif defined GEKKO
#define CC_BUILD_HTTPCLIENT
#define CC_BUILD_OPENAL
#define CC_BUILD_GCWII
#define CC_BUILD_LOWMEM
#undef CC_BUILD_FREETYPE
#endif
#endif


#ifndef CC_BUILD_LOWMEM
#define EXTENDED_BLOCKS
#endif
#define EXTENDED_TEXTURES

#ifdef EXTENDED_BLOCKS
typedef cc_uint16 BlockID;
#else
typedef cc_uint8 BlockID;
#endif

#ifdef EXTENDED_TEXTURES
typedef cc_uint16 TextureLoc;
#else
typedef cc_uint8 TextureLoc;
#endif

typedef cc_uint8 BlockRaw;
typedef cc_uint8 EntityID;
typedef cc_uint8 Face;
typedef cc_uint32 cc_result;
typedef cc_uint64 TimeMS;

typedef struct Rect2D_  { int X, Y, Width, Height; } Rect2D;
typedef struct TextureRec_ { float U1, V1, U2, V2; } TextureRec;

typedef struct cc_string_ {
	char* buffer;       /* Pointer to characters, NOT NULL TERMINATED */
	cc_uint16 length;   /* Number of characters used */
	cc_uint16 capacity; /* Max number of characters  */
} cc_string;
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF

#if defined CC_BUILD_GL
/* NOTE: Although normally OpenGL object/resource IDs are 32 bit integers, */
/*  OpenGL 1.1 does actually use the full 64 bits for 'dynamic' vertex buffers */
typedef cc_uintptr GfxResourceID;
#else
typedef void* GfxResourceID;
#endif

/* Contains the information to describe a 2D textured quad. */
struct Texture {
	GfxResourceID ID;
	short X, Y; cc_uint16 Width, Height;
	TextureRec uv;
};
#endif
