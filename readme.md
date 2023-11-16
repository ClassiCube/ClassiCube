ClassiCube is a custom Minecraft Classic compatible client written in C from scratch.<br>
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](http://i.imgur.com/FCiwl27.png)

# Information

ClassiCube aims to replicate the 2009 Minecraft Classic client while also:
* offering **optional** additions to improve gameplay
* providing much better performance for a large range of hardware
* running on many different systems (desktop, web, mobile, consoles)

ClassiCube is not trying to replicate modern Minecraft versions - and therefore it doesn't (and won't) support:
* survival mode
* logging in with Microsoft or Mojang accounts
* connecting to Minecraft Java Edition or Bedrock Edition servers
<br/>

You can download ClassiCube [from here](https://www.classicube.net/download/) and the very latest builds [from here](https://www.classicube.net/nightlies/).

### We need your help

ClassiCube strives to replicate the original Minecraft Classic experience by **strictly following a [clean room](https://en.wikipedia.org/wiki/Clean_room_design) reverse engineering approach**.

If you're interested in documenting or verifying the behaviour of the original Minecraft Classic, please get in contact with me. (`unknownshadow200` on Discord)

### What ClassiCube is
* A complete re-implementation of Minecraft Classic 0.30, with **optional** additions
* Partially supports some features of Minecraft Classic versions before 0.30
* Lightweight, minimal memory usage compared to original Minecraft Classic
* Much better performance than original Minecraft Classic
* Works with effectively all graphics cards that support OpenGL or Direct3D 9
* Runs on Windows, macOS, Linux, Android, iOS, and in a web browser
* Also runs on OpenBSD, FreeBSD, NetBSD, Solaris, Haiku, IRIX, SerenityOS
* Although still work in progresses, also runs on various consoles

### Instructions
Initially, you will need to run ClassiCube.exe to download the required assets from minecraft.net and classicube.net.<br>
Just click 'OK' to the dialog menu that appears when you start the launcher.

**Singleplayer**
Run ClassiCube.exe, then click Singleplayer at the main menu.

**Multiplayer**
Run ClassiCube.exe. You can connect to LAN/locally hosted servers, and classicube.net servers if you have a [ClassiCube account](https://www.classicube.net/).

<br/>

**Note:** When running from within VirtualBox, disable Mouse Integration, otherwise the camera will not work properly

#### *Stuck on OpenGL 1.1?*
The most common reason for being stuck on OpenGL 1.1 is non-working GPU drivers - so if possible, you should try either installing or updating the drivers for your GPU.

Otherwise:
* On Windows, you can still run the OpenGL build of ClassiCube anyways. (You can try downloading and using the MESA software renderer from [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/) for slightly better performance though)
* On other operating systems, you will have to [compile the game yourself](#Compiling). Don't forget to add `-DCC_BUILD_GL11` to the compilation command line so that the compiled game supports OpenGL 1.1.

## Supported systems

ClassiCube runs on:
* Windows - 95 and later
* macOS - 10.5 or later (but can be compiled to work with 10.3/10.4 though)
* Linux - needs `libcurl` and `libopenal`
* Android - 2.3 or later
* iOS - 10.3 or later
* Most web browsers (even runs on IE11)

And also runs on:
* Raspberry Pi - needs <code>libcurl</code> and <code>libopenal</code>
* FreeBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages
* NetBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages
* OpenBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal</code> packages
* Solaris - needs <code>curl</code> and <code>openal</code> packages
* Haiku - needs <code>openal</code> package (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_haiku.yml))
* BeOS - untested on actual hardware
* IRIX - needs <code>curl</code> and <code>openal</code> packages
* SerenityOS - needs <code>SDL2</code>
* 3DS - unfinished, but usable (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_3ds.yml))
* Wii - unfinished, but usable (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_wiigc.yml))
* GameCube - unfinished, but usable (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_wiigc.yml))
* Dreamcast - unfinished, but renders (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_dreamcast.yml))
* PSP - unfinished, rendering issues (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_psp.yml))
* PS Vita - unfinished, rendering issues (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_vita.yml))
* Xbox - unfinished, major rendering issues (if you have a GitHub account, can [download from here](https://github.com/UnknownShadow200/ClassiCube/actions/workflows/build_xbox.yml))
* PS3 - unfinished, rendering issues
* Nintendo 64 - unfinished, major rendering issues

# Compiling 

*Note: The various instructions below automatically compile ClassiCube with the recommended defaults for the platform. <br>
If you (not recommended) want to override the defaults (e.g. to compile OpenGL build on Windows), see [here](doc/overriding-defaults.md) for details.*

## Compiling - Windows

##### Using Visual Studio
Open ClassiCube.sln *(File -> Open -> Project/Solution)* and compile it *(Build -> Build Solution)*.

If you get a `The Windows SDK version 5.1 was not found` compilation error, [see here for how to fix](doc/compile-fixes.md#visual-studio-unsupported-platform-toolset)

##### Using Visual Studio (command line)
1. Use 'Developer Tools for Visual Studio' from Start Menu
2. Navigate to the directory with ClassiCube's source code
3. Enter `cl.exe *.c /link user32.lib gdi32.lib winmm.lib dbghelp.lib shell32.lib comdlg32.lib /out:ClassiCube.exe`

##### Using MinGW-w64
I am assuming you used the installer from https://sourceforge.net/projects/mingw-w64/
1. Install MinGW-W64
2. Use either *Run Terminal* from Start Menu or run *mingw-w64.bat* in the installation folder
3. Navigate to the directory with ClassiCube's source code
4. Enter `gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp`

##### Using MinGW
I am assuming you used the installer from https://osdn.net/projects/mingw/
1. Install MinGW. You need mingw32-base-bin and msys-base-bin packages.
2. Run *msys.bat* in the *C:\MinGW\msys\1.0* folder.
2. Navigate to the directory with ClassiCube's source code
4. Enter `gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp`

##### Using TCC (Tiny C Compiler)
I am assuming you used `tcc-0.9.27-win64-bin.zip` from https://bellard.org/tcc/
1. Extract the .zip file
2. In TCC's `lib/kernel32.def`, add missing `RtlCaptureContext` at line 554 (In between `RtlAddFunctionTable` and `RtlDeleteFunctionTable`)
3. Copy `winapi` folder and `_mingw_dxhelper.h` from `winapi-full-for-0.9.27.zip` into TCC's `include` folder
4. Navigate to the directory with ClassiCube's source code
5. In `ExtMath.c`, change `fabsf` to `fabs` and `sqrtf` to `sqrtf`
6. Enter `tcc.exe -o ClassiCube.exe *.c -lwinmm -limagehlp -lgdi32 -luser32 -lcomdlg32 -lshell32`

## Compiling - Linux

##### Using gcc/clang

Install appropriate libs as required. For ubuntu these are: libx11-dev, libxi-dev and libgl1-mesa-dev

```gcc -fno-math-errno *.c -o ClassiCube -rdynamic -lpthread -lX11 -lXi -lGL -ldl```

##### Cross compiling for Windows (32 bit):

```i686-w64-mingw32-gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp```

##### Cross compiling for Windows (64 bit):

```x86_64-w64-mingw32-gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp```

##### Raspberry Pi
Although the regular linux compiliation flags will work fine, to take full advantage of the hardware:

```gcc -fno-math-errno *.c -o ClassiCube -DCC_BUILD_RPI -rdynamic -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl```

## Compiling - macOS

##### Using gcc/clang (32 bit)

```cc -fno-math-errno *.c -o ClassiCube -framework Carbon -framework AGL -framework OpenGL -framework IOKit```

##### Using gcc/clang (64 bit)

```cc -fno-math-errno *.c interop_cocoa.m -o ClassiCube -framework Cocoa -framework OpenGL -framework IOKit -lobjc```

## Compiling - for Android

NOTE: If you are distributing a modified version, **please change the package ID from `com.classicube.android.client` to something else** - otherwise Android users won't be able to have both ClassiCube and your modified version installed at the same time on their Android device

##### Using Android Studio GUI

Open `android` folder in Android Studio (TODO explain more detailed)

##### Using command line (gradle)

Run `gradlew` in android folder (TODO explain more detailed)

## Compiling - for iOS

iOS version will have issues as it's incomplete and only tested in iOS Simulator

NOTE: If you are distributing a modified version, **please change the bundle ID from `com.classicube.ios.client` to something else** - otherwise iOS users won't be able to have both ClassiCube and your modified version installed at the same time on their iOS device

##### Using Xcode GUI

Import `ios/CCIOS.xcodeproj` project into Xcode (TODO explain more detailed)

##### Using command line (Xcode)

`xcodebuild -sdk iphoneos -configuration Debug` (TODO explain more detailed)

## Compiling - other desktop OSes

#### FreeBSD

Install libxi, libexecinfo, curl and openal-soft package if needed

```cc *.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### OpenBSD

Install libexecinfo, curl and openal package if needed

```cc *.c -o ClassiCube -I /usr/X11R6/include -I /usr/local/include -L /usr/X11R6/lib -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### NetBSD

Install libexecinfo, curl and openal-soft package if needed

```cc *.c -o ClassiCube -I /usr/X11R7/include -I /usr/pkg/include -L /usr/X11R7/lib -L /usr/pkg/lib  -lpthread -lX11 -lXi -lGL -lexecinfo```

#### DragonflyBSD

```cc *.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### Solaris

```gcc -fno-math-errno *.c -o ClassiCube -lsocket -lX11 -lXi -lGL```

#### Haiku

Install openal_devel package if needed

```cc -fno-math-errno *.c interop_BeOS.cpp -o ClassiCube -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker```

#### BeOS

```cc -fno-math-errno *.c interop_BeOS.cpp -o ClassiCube -lGL -lbe -lgame -ltracker```

#### IRIX

```gcc -fno-math-errno -lGL -lX11 -lXi -lpthread -ldl```

#### SerenityOS

Install SDL2 port if needed

```cc *.c -o ClassiCube -lgl -lSDL2```

## Compiling - other

#### Web

```emcc *.c -s ALLOW_MEMORY_GROWTH=1 --js-library interop_web.js```

The generated javascript file has some issues. [See here for how to fix](doc/compile-fixes.md#webclient-patches)

#### 3DS

Run `make 3ds`. You'll need [libctru](https://github.com/devkitPro/libctru)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `3ds-dev` group)**

The 3DS port needs assistance from someone experienced with 3DS homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### Wii

Run `make wii`. You'll need [libogc](https://github.com/devkitPro/libogc)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `wii-dev` group)**

The Wii port needs assistance from someone experienced with Wii homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### GameCube

Run `make gamecube`. You'll need [libogc](https://github.com/devkitPro/libogc)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `gamecube-dev` group)**

The GC port needs assistance from someone experienced with GameCube homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### PlayStation Portable

Run `make psp`. You'll need [pspsdk](https://github.com/pspdev/pspsdk)

**NOTE: It is recommended that you install the precompiled pspsdk version from [here](https://github.com/pspdev/pspdev/releases)**

The PSP port needs assistance from someone experienced with PSP homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### PlayStation Vita

Run `make vita`. You'll need [vitasdk](https://vitasdk.org/)

The Vita port needs assistance from someone experienced with Vita homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### PlayStation 3

Run `make ps3`. You'll need [PSL1GHT](https://github.com/ps3dev/PSL1GHT)

The PS3 port needs assistance from someone experienced with PS3 homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### Xbox

Run `make xbox`. You'll need [nxdk](https://github.com/XboxDev/nxdk)

The Xbox port needs assistance from someone experienced with Xbox homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### Dreamcast

Run `make dreamcast`. You'll need [KallistiOS](https://github.com/KallistiOS/KallistiOS)

The Dreamcast port needs assistance from someone experienced with Dreamcast homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

#### Nintendo 64

Run `make n64`. You'll need the opengl branch of [libdragon](https://github.com/DragonMinded/libdragon/tree/opengl)

The Nintendo 64 port needs assistance from someone experienced with Nintendo 64 homebrew development - if you're interested, please get in contact with me. (`unknownshadow200` on Discord)

You'll also need to stub out `WorkerLoop` function in `Http_Worker.c` for now

##### Other

You'll have to write the necessary code. You should read portability.md in doc folder.

## Documentation

Functions and variables in .h files are mostly documented.

Further information (e.g. style) for ClassiCube's source code can be found in the doc and misc folders.

#### Known compilation errors

[Fixes for compilation errors when using musl or old glibc for C standard library](doc/compile-fixes.md#common-compilation-errors)

#### Tips
* Press escape (after joining a world) or pause to switch to the pause menu.
* Pause menu -> Options -> Controls lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to on will minimise CPU usage, while off will maximimise chunk loading speed.
* Press F to cycle view distance. A smaller number of visible chunks can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see the list of built in commands, type `/client`.
* To see help for a given built in command, type `/client help <command name>`.


## Open source technologies

* [curl](https://curl.se/) - HTTP/HTTPS for linux and macOS
* [FreeType](https://www.freetype.org/) - Font handling for all platforms
* [GCC](https://gcc.gnu.org/) - Compiles client for linux
* [MinGW-w64](http://mingw-w64.org/doku.php) - Compiles client for windows
* [Clang](https://clang.llvm.org/) - Compiles client for macOS
* [Emscripten](https://emscripten.org/) - Compiles client for web
* [RenderDoc](https://renderdoc.org/) - Graphics debugging
* [BearSSL](https://www.bearssl.org/) - SSL/TLS support on consoles
* [libctru](https://github.com/devkitPro/libctru) - Backend for 3DS
* [citro3D](https://github.com/devkitPro/citro3d) - Rendering backend for 3DS
* [Citra](https://github.com/citra-emu/citra) - Emulator used to test 3DS port
* [pspsdk](https://github.com/pspdev/pspsdk) - Backend for PSP
* [PPSSPP](https://github.com/hrydgard/ppsspp) - Emulator used to test PSP port
* [vitasdk](https://github.com/vitasdk) - Backend for PS Vita
* [Vita3K](https://github.com/Vita3K/Vita3K) - Emulator used to test Vita port
* [PSL1GHT](https://github.com/ps3dev/PSL1GHT) - Backend for PS3
* [RPCS3](https://github.com/RPCS3/rpcs3) - Emulator used to test PS3 port
* [libogc](https://github.com/devkitPro/libogc) - Backend for Wii and GameCube
* [libfat](https://github.com/devkitPro/libfat) - Filesystem backend for Wii/GC
* [Dolphin](https://github.com/dolphin-emu/dolphin) - Emulator used to test Wii/GC port
* [KallistiOS](https://github.com/KallistiOS/KallistiOS) - Backend for Dreamcast
* [GLdc](https://github.com/Kazade/GLdc) - Basis of rendering backend for Dreamcast
* [nullDC](https://github.com/skmp/nulldc) - Emulator used to test Dreamcast port
* [flycast](https://github.com/flyinghead/flycast) - Emulator used to test Dreamcast port
* [nxdk](https://github.com/XboxDev/nxdk) - Backend for Xbox
* [xemu](https://github.com/xemu-project/xemu) - Emulator used to test Xbox port
* [cxbx-reloaded](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded) - Emulator used to test Xbox port
* [libdragon](https://github.com/DragonMinded/libdragon) - Backend for Nintendo 64
* [cen64](https://github.com/n64dev/cen64) - Emulator used to test Nintendo 64 port
* [ares](https://github.com/ares-emulator/ares) - Emulator used to test Nintendo 64 port


## Sound Credits
ClassiCube uses sounds from [Freesound.org](https://freesound.org)<br>
Full credits are listed in [doc/sound-credits.md](doc/sound-credits.md)
