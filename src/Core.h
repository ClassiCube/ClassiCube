#ifndef CC_CORE_H
#define CC_CORE_H
/* 
Core fixed-size integer types, automatic platform detection, and common small structs
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
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

/* Lowest 4 bits are for backends in same group */
/* Rest of the bits indicate the unique group type */

#define CC_WIN_BACKEND_TERMINAL 0x0001
#define CC_WIN_BACKEND_SDL_MASK 0x0010
#define CC_WIN_BACKEND_SDL2     0x0011
#define CC_WIN_BACKEND_SDL3     0x0012
#define CC_WIN_BACKEND_X11      0x0021
#define CC_WIN_BACKEND_WIN32    0x0041
#define CC_WIN_BACKEND_COCOA    0x0081
#define CC_WIN_BACKEND_BEOS     0x0101
#define CC_WIN_BACKEND_ANDROID  0x0201

#define CC_GFX_BACKEND_SOFTGPU   0x0001
#define CC_GFX_BACKEND_GL_MASK   0x0010
#define CC_GFX_BACKEND_GL1       0x0011
#define CC_GFX_BACKEND_GL2       0x0012
#define CC_GFX_BACKEND_D3D9      0x0021
#define CC_GFX_BACKEND_D3D11     0x0041
#define CC_GFX_BACKEND_VULKAN    0x0081

#define CC_BUILD_NETWORKING
#define CC_BUILD_FREETYPE
#define CC_BUILD_RESOURCES
#define CC_BUILD_PLUGINS
#define CC_BUILD_ANIMATIONS
#define CC_BUILD_HELDBLOCK
#define CC_BUILD_FILESYSTEM
#define CC_BUILD_ADVLIGHTING
/*#define CC_BUILD_GL11*/

#ifndef CC_BUILD_MANUAL
#if defined NXDK
	/* XBox also defines _WIN32 */
	#define CC_BUILD_XBOX
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
#elif defined XENON
	/* libxenon also defines __linux__ (yes, really) */
	#define CC_BUILD_XBOX360
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#define CC_BUILD_HTTPCLIENT
#elif defined _WIN32
	#define CC_BUILD_WIN
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_SCHANNEL
	#define CC_BUILD_WINMM
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_D3D9
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_WIN32
#elif defined __ANDROID__
	#define CC_BUILD_ANDROID
	#define CC_BUILD_MOBILE
	#define CC_BUILD_POSIX
	#define CC_BUILD_GLES
	#define CC_BUILD_EGL
	#define CC_BUILD_TOUCH
	#define CC_BUILD_OPENSLES
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_ANDROID
#elif defined __serenity__
	#define CC_BUILD_SERENITY
	#define CC_BUILD_POSIX
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_SDL2
#elif defined __linux__
	#define CC_BUILD_LINUX
	#define CC_BUILD_POSIX
	#define CC_BUILD_XINPUT2
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
	#if defined CC_BUILD_RPI
		#define CC_BUILD_GLES
		#define CC_BUILD_EGL
		#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2
	#else
		#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1	
	#endif
#elif defined __APPLE__
	#define CC_BUILD_DARWIN
	#define CC_BUILD_POSIX
	#if defined __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
		#define CC_BUILD_MOBILE
		#define CC_BUILD_GLES
		#define CC_BUILD_IOS
		#define CC_BUILD_TOUCH
		#define CC_BUILD_CFNETWORK
		#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2
	#else
		#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_COCOA
		#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
		#define CC_BUILD_MACOS
		#define CC_BUILD_CURL
	#endif
	#define CC_BUILD_OPENAL
#elif defined Macintosh
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_SOFTGPU
	#define CC_BUILD_MACCLASSIC
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#undef  CC_BUILD_RESOURCES
	#undef  CC_BUILD_FREETYPE
	#undef  CC_BUILD_NETWORKING
#elif defined __sun__
	#define CC_BUILD_SOLARIS
	#define CC_BUILD_POSIX
	#define CC_BUILD_XINPUT2
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
#elif defined __FreeBSD__ || defined __DragonFly__
	#define CC_BUILD_FREEBSD
	#define CC_BUILD_POSIX
	#define CC_BUILD_BSD
	#define CC_BUILD_XINPUT2
	#define CC_BUILD_CURL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
#elif defined __OpenBSD__
	#define CC_BUILD_OPENBSD
	#define CC_BUILD_POSIX
	#define CC_BUILD_BSD
	#define CC_BUILD_XINPUT2
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
#elif defined __NetBSD__
	#define CC_BUILD_NETBSD
	#define CC_BUILD_POSIX
	#define CC_BUILD_BSD
	#define CC_BUILD_XINPUT2
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
#elif defined __HAIKU__
	#define CC_BUILD_HAIKU
	#define CC_BUILD_POSIX
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define CC_BACKTRACE_BUILTIN
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_BEOS
#elif defined __BEOS__
	#define CC_BUILD_BEOS
	#define CC_BUILD_POSIX
	#define CC_BUILD_GL11
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_OPENAL
	#define CC_BACKTRACE_BUILTIN
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_BEOS
#elif defined __sgi
	#define CC_BUILD_IRIX
	#define CC_BUILD_POSIX
	#define CC_BUILD_CURL
	#define CC_BUILD_OPENAL
	#define CC_BIG_ENDIAN
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL1
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_X11
#elif defined __EMSCRIPTEN__
	#define CC_BUILD_WEB
	#define CC_BUILD_GLES
	#define CC_BUILD_TOUCH
	#define CC_BUILD_WEBAUDIO
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_MINFILES
	#define CC_BUILD_COOPTHREADED
	#undef  CC_BUILD_FREETYPE
	#undef  CC_BUILD_RESOURCES
	#undef  CC_BUILD_PLUGINS
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2
#elif defined __psp__
	#define CC_BUILD_PSP
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_OPENAL
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
#elif defined __3DS__
	#define CC_BUILD_3DS
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_TOUCH
	#define CC_BUILD_DUALSCREEN
#elif defined GEKKO
	#define CC_BUILD_GCWII
	#define CC_BUILD_CONSOLE
	#ifndef HW_RVL
		#define CC_BUILD_LOWMEM
	#endif
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
#elif defined __vita__
	#define CC_BUILD_PSVITA
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_OPENAL
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_TOUCH
#elif defined _arch_dreamcast
	#define CC_BUILD_DREAMCAST
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
	#define CC_BUILD_SMALLSTACK
	#undef  CC_BUILD_RESOURCES
#elif defined PLAT_PS3
	#define CC_BUILD_PS3
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_OPENAL
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
#elif defined N64
	#define CC_BIG_ENDIAN
	#define CC_BUILD_N64
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#define CC_BUILD_SMALLSTACK
	#undef  CC_BUILD_RESOURCES
	#undef  CC_BUILD_NETWORKING
	#undef  CC_BUILD_FILESYSTEM
#elif defined PLAT_PS2
	#define CC_BUILD_PS2
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_OPENAL
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
#elif defined PLAT_NDS
	#define CC_BUILD_NDS
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_TOUCH
	#define CC_BUILD_SMALLSTACK
	#undef  CC_BUILD_ANIMATIONS /* Very costly in FPU less system */
	#undef  CC_BUILD_ADVLIGHTING
#elif defined __WIIU__
	#define CC_BUILD_WIIU
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_OPENAL
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_SPLITSCREEN
	#define CC_BUILD_TOUCH
#elif defined __SWITCH__
	#define CC_BUILD_SWITCH
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_HTTPCLIENT
	#define CC_BUILD_BEARSSL
	#define CC_BUILD_TOUCH
	#define CC_BUILD_GLES
	#define CC_BUILD_EGL
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_GL2
#elif defined PLAT_PS1
	#define CC_BUILD_PS1
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_TINYMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#undef  CC_BUILD_RESOURCES
	#undef  CC_BUILD_NETWORKING
	#undef  CC_BUILD_ANIMATIONS /* Very costly in FPU less system */
	#undef  CC_BUILD_HELDBLOCK  /* Very costly in FPU less system */
	#undef  CC_BUILD_ADVLIGHTING
	#undef  CC_BUILD_FILESYSTEM
#elif defined OS2
	#define CC_BUILD_OS2
	#define CC_BUILD_POSIX
	#define CC_BUILD_CURL
	#define CC_BUILD_FREETYPE
	#define DEFAULT_GFX_BACKEND CC_GFX_BACKEND_SOFTGPU
	#define DEFAULT_WIN_BACKEND CC_WIN_BACKEND_SDL2
#elif defined PLAT_SATURN
	#define CC_BUILD_SATURN
	#define CC_BUILD_CONSOLE
	#define CC_BUILD_LOWMEM
	#define CC_BUILD_TINYMEM
	#define CC_BUILD_COOPTHREADED
	#define CC_BUILD_NOMUSIC
	#define CC_BUILD_NOSOUNDS
	#define CC_BUILD_SMALLSTACK
	#define CC_BUILD_TINYSTACK
	#undef  CC_BUILD_RESOURCES
	#undef  CC_BUILD_NETWORKING
	#undef  CC_BUILD_ANIMATIONS /* Very costly in FPU less system */
	#undef  CC_BUILD_HELDBLOCK  /* Very costly in FPU less system */
	#undef  CC_BUILD_ADVLIGHTING
	#undef  CC_BUILD_FILESYSTEM
#endif
#endif

/* Use platform default unless override is provided via command line/makefile/etc */
#if defined DEFAULT_WIN_BACKEND && !defined CC_WIN_BACKEND
	#define CC_WIN_BACKEND DEFAULT_WIN_BACKEND
#endif
#if defined DEFAULT_GFX_BACKEND && !defined CC_GFX_BACKEND
	#define CC_GFX_BACKEND DEFAULT_GFX_BACKEND
#endif

#ifdef CC_BUILD_CONSOLE
#undef CC_BUILD_FREETYPE
#undef CC_BUILD_PLUGINS
#endif

#ifdef CC_BUILD_NETWORKING
#define CUSTOM_MODELS
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
typedef union cc_pointer_ { cc_uintptr val; void* ptr; } cc_pointer;

typedef struct Rect2D_  { int x, y, width, height; } Rect2D;
typedef struct TextureRec_ { float u1, v1, u2, v2; } TextureRec;

typedef struct cc_string_ {
	char* buffer;       /* Pointer to characters, NOT NULL TERMINATED */
	cc_uint16 length;   /* Number of characters used */
	cc_uint16 capacity; /* Max number of characters  */
} cc_string;
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF

typedef void* GfxResourceID;

/* Contains the information to describe a 2D textured quad. */
struct Texture {
	GfxResourceID ID;
	short x, y; cc_uint16 width, height;
	TextureRec uv;
};
#endif

