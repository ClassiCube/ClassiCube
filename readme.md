ClassiCube is a custom Minecraft Classic compatible client written in C from scratch.<br>
> [!IMPORTANT]
**ClassiCube is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.** <br><br>
**ClassiCube is not trying to replicate modern Minecraft.**<br>
**It will never support survival, Minecraft accounts, or modern Minecraft servers.**

# What ClassiCube is

ClassiCube aims to replicate the 2009 Minecraft Classic client while offering **optional** enhancements to improve gameplay. ClassiCube can run on [many systems](#supported-systems), including desktop, web, mobile, and even some consoles.
<details>
<summary><b>Features (click to expand)</b></summary>

* Much better performance and minimal memory usage compared to original Minecraft Classic
* Optional enhancements including custom blocks, models, and environment colors
* Partially supports some features of Minecraft Classic versions before 0.30
* Works with effectively all graphics cards that support OpenGL or Direct3D 9
* Runs on Windows, macOS, Linux, Android, iOS, and in a web browser
* Also runs on OpenBSD, FreeBSD, NetBSD, Solaris, Haiku, IRIX, SerenityOS
* Although in various stages of early development, also runs on various consoles

</details>

You can **download ClassiCube** [here](https://www.classicube.net/download/) and the very latest builds [here](https://www.classicube.net/nightlies/).

![classic](https://github.com/ClassiCube/ClassiCube/assets/6509348/eedee53f-f53e-456f-b51c-92c62079eee0)

![enhanced](https://github.com/ClassiCube/ClassiCube/assets/6509348/b2fe0e2b-5d76-41ab-909f-048d0ad15f37)


# We need your help

ClassiCube strives to replicate the original Minecraft Classic experience by **strictly adhering to [clean room](https://en.wikipedia.org/wiki/Clean_room_design) reverse engineering approach**.

If you're interested in documenting or verifying the behaviour of the original Minecraft Classic, please get in contact on the [ClassiCube Discord](https://classicube.net/discord)


# How to play
Initially, you will need to run ClassiCube.exe to download the required assets from minecraft.net and classicube.net.<br>
Just click 'OK' to the dialog menu that appears when you start the launcher.

> **Note:** When running from within VirtualBox, disable Mouse Integration, otherwise the in-game camera won't work properly.

**Singleplayer mode**
Run ClassiCube.exe, then click Singleplayer at the main menu.

**Multiplayer mode**
Run ClassiCube.exe. You can connect to LAN/locally hosted servers, and classicube.net servers if you have a [ClassiCube account](https://www.classicube.net/).

#### *Stuck on OpenGL 1.1?*
The most common reason for being stuck on OpenGL 1.1 is non-working GPU drivers - so if possible, you should try either installing or updating the drivers for your GPU.

Otherwise:
* On Windows, you can still run the OpenGL build of ClassiCube anyways. <br> 
(You can try downloading and using the MESA software renderer from [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/) for slightly better performance though)
* On other operating systems, you will have to [compile the game yourself](#Compiling). <br> 
Don't forget to add `-DCC_BUILD_GL11` to the compilation command line so that the compiled game supports OpenGL 1.1.

# Supported systems

ClassiCube runs on:
* Windows - 95 and later
* macOS - 10.5 or later (can be compiled for 10.3/10.4 though)
* Linux - needs `libcurl` and `libopenal`
* Android - 2.3 or later
* iOS - 8.0 or later
* Most web browsers (even runs on IE11)

And also runs on:
* Raspberry Pi - needs <code>libcurl</code> and <code>libopenal</code>
* FreeBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages (can [download from here](https://www.classicube.net/download/#dl-fbsd))
* NetBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages (can [download from here](https://www.classicube.net/download/#dl-nbsd))
* OpenBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal</code> packages
* Solaris - needs <code>curl</code> and <code>openal</code> packages
* Haiku - needs <code>openal</code> package (if you have a GitHub account, can [download from here](https://github.com/ClassiCube/ClassiCube/actions/workflows/build_haiku.yml))
* BeOS - untested on actual hardware
* IRIX - needs <code>curl</code> and <code>openal</code> packages
* SerenityOS - needs <code>SDL2</code>
* Classic Mac OS (System 7 and later)
* Dreamcast - unfinished, but usable (can [download from here](https://www.classicube.net/download/dreamcast))
* Saturn - unfinished, major rendering and **stability issues** (can [download from here](https://www.classicube.net/download/saturn))
* Switch - unfinished, but usable (can [download from here](https://www.classicube.net/download/switch))
* Wii U - unfinished, major issues, **broken on real hardware** (can [download from here](https://www.classicube.net/download/wiiu))
* Wii - unfinished, but usable (can [download from here](https://www.classicube.net/download/wii))
* GameCube - unfinished, but usable (can [download from here](https://www.classicube.net/download/gamecube))
* Nintendo 64 - unfinished, moderate rendering issues (can [download from here](https://www.classicube.net/download/n64))
* 3DS - unfinished, but usable (can [download from here](https://www.classicube.net/download/3ds))
* DS/DSi - unfinished, rendering issues  (can [download from here](https://www.classicube.net/download/nds))
* PS Vita - unfinished, rendering issues (can [download from here](https://www.classicube.net/download/vita))
* PSP - unfinished, rendering issues (can [download from here](https://www.classicube.net/download/psp))
* PS3 - unfinished, rendering issues (can [download from here](https://www.classicube.net/download/ps3))
* PS2 - unfinished, major rendering and **stability issues** (can [download from here](https://www.classicube.net/download/ps2))
* PS1 - unfinished, major rendering and **stability issues** (can [download from here](https://www.classicube.net/download/ps1))
* Xbox 360 - completely unfinished, **broken on real hardware** (can [download from here](https://www.classicube.net/download/360))
* Xbox - unfinished, major rendering issues (can [download from here](https://www.classicube.net/download/xbox))

# Compiling 

*Note: The instructions below automatically compile ClassiCube with the recommended defaults for the platform. <br>
If you (not recommended) want to override the defaults (e.g. to compile OpenGL build on Windows), see [here](doc/overriding-defaults.md) for details.*

## Compiling - Windows

##### Using Visual Studio
1. Open ClassiCube.sln *(File -> Open -> Project/Solution)*
2. Compile/Build it *(Build -> Build Solution)*.

If you get a `The Windows SDK version 5.1 was not found` compilation error, [see here for how to fix](doc/compile-fixes.md#visual-studio-unsupported-platform-toolset)

##### Using Visual Studio (command line)
1. Use 'Developer Tools for Visual Studio' from Start Menu
2. Navigate to the directory with ClassiCube's source code
3. Run `cl.exe *.c /link user32.lib gdi32.lib winmm.lib dbghelp.lib shell32.lib comdlg32.lib /out:ClassiCube.exe`

##### Using MinGW-w64
Assuming that you used the installer from https://sourceforge.net/projects/mingw-w64/ :
1. Install MinGW-W64
2. Use either *Run Terminal* from Start Menu or run *mingw-w64.bat* in the installation folder
3. Navigate to the directory with ClassiCube's source code
4. Run `gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm`

##### Using MinGW
Assuming that you used the installer from https://osdn.net/projects/mingw/ :
1. Install MinGW. You need mingw32-base-bin and msys-base-bin packages.
2. Run *msys.bat* in the *C:\MinGW\msys\1.0* folder.
2. Navigate to the directory with ClassiCube's source code
4. Run `gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm`

##### Using TCC (Tiny C Compiler)
Setting up TCC:
1. Download and extract `tcc-0.9.27-win64-bin.zip` from https://bellard.org/tcc/
2. Download `winapi-full-for-0.9.27.zip` from https://bellard.org/tcc/ 
3. Copy `winapi` folder and `_mingw_dxhelper.h` from `winapi-full-for-0.9.27.zip` into TCC's `include` folder

Compiling with TCC:
1. Navigate to the directory with ClassiCube's source code
2. In `ExtMath.c`, change `fabsf` to `fabs` and `sqrtf` to `sqrt`
3. Run `tcc.exe -o ClassiCube.exe *.c -lwinmm -lgdi32 -luser32 -lcomdlg32 -lshell32`<br>
(Note: You may need to specify the full path to `tcc.exe` instead of just `tcc.exe`)

## Compiling - Linux

##### Using gcc/clang
1. Install X11, XInput2, and OpenGL development libraries if necessary. <br>
For Ubuntu, these are the `libx11-dev`, `libxi-dev` and `libgl1-mesa-dev` packages
2. Run either:
    * `make linux` or
    * `cc -fno-math-errno src/*.c -o ClassiCube -rdynamic -lpthread -lX11 -lXi -lGL -ldl`

##### Cross compiling for Windows (32 bit):
1. Install MinGW-w64 if necessary. (Ubuntu: `gcc-mingw-w64` package)
2. Run ```i686-w64-mingw32-gcc -fno-math-errno src/*.c -o ClassiCube.exe -mwindows -lwinmm```

##### Cross compiling for Windows (64 bit):
1. Install MinGW-w64 if necessary. (Ubuntu: `gcc-mingw-w64` package)
2. Run ```x86_64-w64-mingw32-gcc -fno-math-errno src/*.c -o ClassiCube.exe -mwindows -lwinmm```

##### Raspberry Pi
Although the regular linux compiliation flags will work fine, to take full advantage of the hardware:

```gcc -fno-math-errno src/*.c -o ClassiCube -DCC_BUILD_RPI -rdynamic -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl```

## Compiling - macOS
1. Install a C compiler if necessary. The easiest way of obtaining one is by installing **Xcode**.
2. Run either:
    * `make darwin` or
    * `cc -fno-math-errno src/*.c src/*.m -o ClassiCube -framework Cocoa -framework OpenGL -framework IOKit -lobjc`

##### Using Xcode GUI

1. Open the `misc/macOS/CCMAC.xcodeproj` project in Xcode
2. Compile the project

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

1. Open the `misc/ios/CCIOS.xcodeproj` project in Xcode
2. Compile the project

##### Using command line (Xcode)

`xcodebuild -sdk iphoneos -configuration Debug` (TODO explain more detailed)

## Compiling - webclient

1. Install emscripten if necessary.
2. Run either:
    * `make web` or
    * `emcc src/*.c -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=1Mb --js-library interop_web.js`

The generated javascript file has some issues. [See here for how to fix](doc/compile-fixes.md#webclient-patches)

For details on how to integrate the webclient into a website, see [here](doc/hosting-webclient.md)

<details>
<summary><h2>Compiling - consoles</h2></summary>

All console ports need assistance from someone experienced with homebrew development - if you're interested, please get in contact on the [ClassiCube Discord.](https://classicube.net/discord)

<details>
<summary><h3>Nintendo consoles (click to expand)</h3></summary>

#### Switch

Run `make switch`. You'll need [libnx](https://github.com/switchbrew/libnx) and [mesa](https://github.com/devkitPro/mesa)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `switch-dev` group and the `switch-mesa switch-glm` packages)**

#### Wii U

Run `make wiiu`. You'll need [wut](https://github.com/devkitPro/wut/)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `wiiu-dev` group)**

#### 3DS

Run `make 3ds`. You'll need [libctru](https://github.com/devkitPro/libctru)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `3ds-dev` group)**

#### Wii

Run `make wii`. You'll need [libogc](https://github.com/devkitPro/libogc)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `wii-dev` group)**

#### GameCube

Run `make gamecube`. You'll need [libogc](https://github.com/devkitPro/libogc)

**NOTE: It is highly recommended that you install the precompiled devkitpro packages from [here](https://devkitpro.org/wiki/Getting_Started) - you need the `gamecube-dev` group)**

#### Nintendo DS/DSi

Run `make ds`. You'll need [BlocksDS](https://github.com/blocksds/sdk)

#### Nintendo 64

Run `make n64`. You'll need the opengl branch of [libdragon](https://github.com/DragonMinded/libdragon/tree/opengl)

</details>


<details>
<summary><h3>Sony consoles (click to expand)</h3></summary>

#### PlayStation Vita

Run `make vita`. You'll need [vitasdk](https://vitasdk.org/)

#### PlayStation Portable

Run `make psp`. You'll need [pspsdk](https://github.com/pspdev/pspsdk)

**NOTE: It is recommended that you install the precompiled pspsdk version from [here](https://github.com/pspdev/pspdev/releases)**

#### PlayStation 3

Run `make ps3`. You'll need [PSL1GHT](https://github.com/ps3dev/PSL1GHT)

#### PlayStation 2

Run `make ps2`. You'll need [ps2sdk](https://github.com/ps2dev/ps2sdk)

#### PlayStation 1

Run `make ps1`. You'll need [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK/)

</details>


<details>
<summary><h3>Microsoft consoles (click to expand)</h3></summary>

#### Xbox 360

Run `make 360`. You'll need [libxenon](https://github.com/Free60Project/libxenon)

#### Xbox (original)

Run `make xbox`. You'll need [nxdk](https://github.com/XboxDev/nxdk)

</details>


<details>
<summary><h3>SEGA consoles (click to expand)</h3></summary>

### SEGA consoles

#### Dreamcast

Run `make dreamcast`. You'll need [KallistiOS](https://github.com/KallistiOS/KallistiOS)

#### Saturn

Run `make saturn`. You'll need [libyaul](https://github.com/yaul-org/libyaul)

</details>

</details>


<details>
<summary><h2>Compiling - other platforms (click to expand)</h2></summary>

#### FreeBSD

1. Install `libxi`, `libexecinfo`, `curl` and `openal-soft` packages if needed
2. Run either:
    * `make freebsd` or
    * `cc src/*.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo`

#### OpenBSD

1. Install `libexecinfo`, `curl` and `openal` packages if needed
2. Run either:
    * `make opensd` or
    * `cc src/*.c -o ClassiCube -I /usr/X11R6/include -I /usr/local/include -L /usr/X11R6/lib -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo`

#### NetBSD

1. Install `libexecinfo`, `curl` and `openal-soft` packages if needed
2. Run either:
    * `make netbsd` or
    * `cc src/*.c -o ClassiCube -I /usr/X11R7/include -I /usr/pkg/include -L /usr/X11R7/lib -L /usr/pkg/lib  -lpthread -lX11 -lXi -lGL -lexecinfo`

#### DragonflyBSD

1. Install `libxi`, `libexecinfo`, `curl` and `openal-soft` packages if needed
2. Run either:
    * `make dragonfly` or
    * `cc src/*.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo`

#### Solaris

1. Install required packages if needed
2. Run either:
    * `make sunos` or
    * `gcc -fno-math-errno src/*.c -o ClassiCube -lsocket -lX11 -lXi -lGL`

#### Haiku

1. Install `gcc`, `haiku_devel`, `openal_devel` packages if needed
2. Run either:
    * `make haiku` or
    * `cc -fno-math-errno src/*.c interop_BeOS.cpp -o ClassiCube -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker`

#### BeOS

1. Install a C compiler
2. Run either:
    * `make beos` or
    * `cc -fno-math-errno src/*.c interop_BeOS.cpp -o ClassiCube -lGL -lbe -lgame -ltracker`

#### IRIX

1. Install required packages if needed
2. Run either:
    * `make irix` or
    * gcc -fno-math-errno src/*.c -o ClassiCube -lGL -lX11 -lXi -lpthread -ldl`

#### SerenityOS

1. Install SDL2 port if needed
2. Run either:
    * `make serenityos` or
    * `cc src/*.c -o ClassiCube -lgl -lSDL2`

#### Classic Mac OS

1. Install Retro68
2. Run either
    * ```make macclassic_68k``` (For a M68k build)
    * ```make macclassic_ppc``` (For a PPC build)

The PowerPC build will usually perform much better

#### Other systems

You'll have to write the necessary code. You should read `portability.md` in doc folder.

</details>

## Documentation

Functions and variables in .h files are mostly documented.

Further information (e.g. style) for ClassiCube's source code can be found in the doc and misc folders.

#### Known compilation errors

[Fixes for compilation errors when using musl or old glibc for C standard library](doc/compile-fixes.md#common-compilation-errors)

#### Tips
* Press escape (after joining a world) or pause to switch to the pause menu.
* Pause menu -> Options -> Controls lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to on will minimise CPU usage, while off will maximimise chunk loading speed.
* Press F to cycle view distance. Lower view distances can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see the list of built in commands, type `/client`.
* To see help for a given built in command, type `/client help <command name>`.


<details>
<summary><h2>Open source technologies (click to expand)</h2></summary>

* [curl](https://curl.se/) - HTTP/HTTPS for linux and macOS
* [FreeType](https://www.freetype.org/) - Font handling for all platforms
* [GCC](https://gcc.gnu.org/) - Compiles client for linux
* [MinGW-w64](http://mingw-w64.org/doku.php) - Compiles client for windows
* [Clang](https://clang.llvm.org/) - Compiles client for macOS
* [Emscripten](https://emscripten.org/) - Compiles client for web
* [RenderDoc](https://renderdoc.org/) - Graphics debugging
* [BearSSL](https://www.bearssl.org/) - SSL/TLS support on consoles
* [libnx](https://github.com/switchbrew/libnx) - Backend for Switch
* [Ryujinx](https://github.com/Ryujinx/Ryujinx) - Emulator used to test Switch port
* [wut](https://github.com/devkitPro/wut/) - Backend for Wii U
* [Cemu](https://github.com/cemu-project/Cemu) - Emulator used to test Wii U port
* [libctru](https://github.com/devkitPro/libctru) - Backend for 3DS
* [citro3D](https://github.com/devkitPro/citro3d) - Rendering backend for 3DS
* [Citra](https://github.com/citra-emu/citra) - Emulator used to test 3DS port
* [libogc](https://github.com/devkitPro/libogc) - Backend for Wii and GameCube
* [libfat](https://github.com/devkitPro/libfat) - Filesystem backend for Wii/GC
* [Dolphin](https://github.com/dolphin-emu/dolphin) - Emulator used to test Wii/GC port
* [libdragon](https://github.com/DragonMinded/libdragon) - Backend for Nintendo 64
* [ares](https://github.com/ares-emulator/ares) - Emulator used to test Nintendo 64 port
* [BlocksDS](https://github.com/blocksds/sdk) - Backend for Nintendo DS
* [melonDS](https://github.com/melonDS-emu/melonDS) - Emulator used to test Nintendo DS port
* [vitasdk](https://github.com/vitasdk) - Backend for PS Vita
* [Vita3K](https://github.com/Vita3K/Vita3K) - Emulator used to test Vita port
* [pspsdk](https://github.com/pspdev/pspsdk) - Backend for PSP
* [PPSSPP](https://github.com/hrydgard/ppsspp) - Emulator used to test PSP port
* [PSL1GHT](https://github.com/ps3dev/PSL1GHT) - Backend for PS3
* [RPCS3](https://github.com/RPCS3/rpcs3) - Emulator used to test PS3 port
* [ps2sdk](https://github.com/ps2dev/ps2sdk) - Backend for PS2
* [PCSX2](https://github.com/PCSX2/pcsx2) - Emulator used to test PS2 port
* [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK/) - Backend for PS1
* [duckstation](https://github.com/stenzek/duckstation) - Emulator used to test PS1 port
* [libxenon](https://github.com/Free60Project/libxenon) - Backend for Xbox 360
* [nxdk](https://github.com/XboxDev/nxdk) - Backend for Xbox
* [xemu](https://github.com/xemu-project/xemu) - Emulator used to test Xbox port
* [cxbx-reloaded](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded) - Emulator used to test Xbox port
* [KallistiOS](https://github.com/KallistiOS/KallistiOS) - Backend for Dreamcast
* [GLdc](https://github.com/Kazade/GLdc) - Basis of rendering backend for Dreamcast
* [flycast](https://github.com/flyinghead/flycast) - Emulator used to test Dreamcast port
* [libyaul](https://github.com/yaul-org/libyaul) - Backend for Saturn
* [mednafen](https://mednafen.github.io/) - Emulator used to test Saturn port

</details>

## Sound Credits
ClassiCube uses sounds from [Freesound.org](https://freesound.org)<br>
Full credits are listed in [doc/sound-credits.md](doc/sound-credits.md)
