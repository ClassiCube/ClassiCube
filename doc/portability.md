Although most of the code is platform-independent, some per-platform functionality is required.

By default I try to automatically define appropriate backends for your OS in Core.h. Define ```CC_BUILD_MANUAL``` to disable this.

## Before you start
* IEEE floating support is required.
* int must be 32-bits. 32-bit addressing (or more) is required.
* Support for 8/16/32/64 integer types is required. (your compiler must support 64-bit arithmetic)

In other words, the codebase can theroetically be ported to any modern-ish hardware, but not stuff like a UNIVAC machine, the SuperFX chip on the SNES, or an 8-bit microcontroller.

## Supported platforms
**Note:** Some of these machines are just virtual machines. Should still work on real hardware though.

#### Tier 1 support
These platforms are regularly tested on and have executables automatically compiled for. (see buildbot.sh)

|Platform|Machine|Notes|
|--------|-------|-----|
|Windows x86/x64 | Windows 7 |
|macOS x86/x64 | macOS 10.12 |
|Linux x86/x64 | Xubuntu 14 | 
|Web client | Chrome |

#### Tier 2 support
These machines are far less frequently tested on, but are otherwise same as tier 1 support.

|Platform|Machine|Notes|
|--------|-------|-----|
|Windows x86 | Windows 2000 |
|Windows x86 | 98 + KernelEX | Updating doesn't work
|Windows x64 | Windows 10 |
|ReactOS | ReactOS |
|Linux x64 | Arch linux |
|Linux x64 | Linux Mint |
|Linux x64 | Kubuntu |
|Linux x64 | Debian |
|Linux x64 | Fedora |
|Linux x86/x64 | Lubuntu |
|Web client | Firefox |
|Web client | Safari |
|Web client | Edge | Cursor doesn't seem to disappear

#### Tier 3 support
The game has been compiled and run on these platforms before. It may or may not still compile for them.

I don't really test these platforms at all, only when I suspect some changes to the code might impact them.

|Platform|Machine|Notes|
|--------|-------|-----|
|macOS x86 | macOS 10.4 |
|FreeBSD x86 | FreeBSD | x64 should work too |
|NetBSD x86 | NetBSD | x64 should work too |
|OpenBSD x86 | OpenBSD | x64 should work too |
|Solaris x86 | OpenIndiana | x64 should work too |
|macOS PPC | macOS 10.3 | PPC64 completely untested |
|Linux PPC | Debian | Issues with colour channels incorrectly swapped? |
|Linux ARM | Raspberry pi | ARM64 should work too |
|Linux SPARC | Debian | Didn't really work due to lack of 24-bit colours |
|Linux Alpha | Debian | 
|HaikuOS | Nightly | Requires SDL for windowing

## Porting

Listed below are the requirements for implementing each platform-dependent file.
You should try to take advantage of existing backends when porting to other platforms.
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
- ```CC_BUILD_X11``` - Use X11/XLib (unix-ish) (glX)
- ```CC_BUILD_SDL``` - Use SDL library (SDL)

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
- ```CC_BUILD_OPENAL``` - use OpenAL
- ```CC_BUILD_NOAUDIO``` - stub audio implementation (silent)

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
- ```CC_BUILD_CURL``` - use libcurl for http

Supporting connection reuse is highly recommended. (but not required)
