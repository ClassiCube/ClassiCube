# ClassiCube 

**ClassiCube** is a custom Minecraft Classic-compatible client, built entirely from scratch in C.  
It is not affiliated with Mojang AB, Minecraft, or Microsoft in any way. 

## What is ClassiCube? 

ClassiCube aims to faithfully recreate the 2009 Minecraft Classic client while providing **optional** enhancements to improve the gameplay experience. The game can run on a wide variety of systems, including desktop, web, mobile, and even some consoles.

### Key Features 
<details>
<summary><strong>Click to expand</strong></summary>

- **High Performance**: Enjoy significantly better performance and lower memory usage than the original Minecraft Classic.
- **Customization**: Take advantage of optional features like custom blocks, models, and environment colors.
- **Partial Compatibility**: Supports features from Minecraft Classic versions before 0.30.
- **Wide Hardware Support**: Works with almost all graphics cards that support OpenGL or Direct3D 9.
- **Cross-Platform**: Runs on Windows, macOS, Linux, Android, iOS, and most web browsers.
- **Console Support**: Also runs on OpenBSD, FreeBSD, NetBSD, Solaris, Haiku, IRIX, SerenityOS, and even some consoles (in varying stages of development).

</details>

Please note that **ClassiCube** does not aim to replicate modern versions of Minecraft. It will never support survival mode, Minecraft account logins, or connections to official Minecraft servers.

You can **download ClassiCube** [here](https://www.classicube.net/download/) and access the latest builds [here](https://www.classicube.net/nightlies/). 

![classic](https://github.com/ClassiCube/ClassiCube/assets/6509348/eedee53f-f53e-456f-b51c-92c62079eee0)
![enhanced](https://github.com/ClassiCube/ClassiCube/assets/6509348/b2fe0e2b-5d76-41ab-909f-048d0ad15f37)

---

## We Need Your Help! 

ClassiCube aims to replicate the original Minecraft Classic experience by **strictly adhering to a [clean room](https://en.wikipedia.org/wiki/Clean_room_design) reverse engineering approach**.

If you're interested in documenting or verifying the behavior of the original Minecraft Classic, please reach out to me (`unknownshadow200` on Discord). 

---

## How to Play 🎮

### Getting Started 
- Run `ClassiCube.exe` to download the required assets from `minecraft.net` and `classicube.net`. When the dialog menu appears, click **OK**.

### Singleplayer Mode 
- Run `ClassiCube.exe`, then click **Singleplayer** in the main menu.

### Multiplayer Mode 
- Run `ClassiCube.exe` and connect to LAN servers or classicube.net servers (you'll need a [ClassiCube account](https://www.classicube.net/)).

> **Note**: When using VirtualBox, disable Mouse Integration to avoid camera issues.

### Stuck on OpenGL 1.1? 
The most common reason for being stuck on OpenGL 1.1 is outdated or non-functional GPU drivers. If possible, try updating your GPU drivers.

- **Windows**: You can still run ClassiCube using the MESA software renderer. Download it [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/) for better performance.
- **Other OS**: You may need to [compile the game yourself](#Compiling). Don’t forget to add `-DCC_BUILD_GL11` to the compilation command to support OpenGL 1.1.

---

## Supported Systems 

ClassiCube runs on a wide range of platforms:

- **Windows** (95 and later)
- **macOS** (10.5 or later; can be compiled for 10.3/10.4)
- **Linux** (requires `libcurl` and `libopenal`)
- **Android** (2.3 or later)
- **iOS** (8.0 or later)
- **Web browsers**, including Internet Explorer 11

And also runs on various other systems such as:
* Raspberry Pi - needs <code>libcurl</code> and <code>libopenal</code>
* FreeBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages (if you have a GitHub account, can [download from here](https://github.com/ClassiCube/ClassiCube/actions/workflows/build_freebsd.yml))
* NetBSD - needs <code>libexecinfo</code>, <code>curl</code> and <code>openal-soft</code> packages (if you have a GitHub account, can [download from here](https://github.com/ClassiCube/ClassiCube/actions/workflows/build_netbsd.yml))
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

---

# Compiling Guide 

*Note: The instructions below automatically compile ClassiCube with recommended defaults for each platform. If you wish to override these defaults (not recommended), such as compiling the OpenGL build on Windows, refer to the [overriding defaults guide](doc/overriding-defaults.md) for further details.*

## Compiling - Windows 

### Using Visual Studio
1. Open `ClassiCube.sln` (*File -> Open -> Project/Solution*).
2. Compile it by selecting *Build -> Build Solution*.

If you encounter the error `The Windows SDK version 5.1 was not found`, refer to [this guide for fixing it](doc/compile-fixes.md#visual-studio-unsupported-platform-toolset).

### Using Visual Studio (Command Line)
1. Open *Developer Tools for Visual Studio* from the Start Menu.
2. Navigate to the directory containing ClassiCube's source code.
3. Run the following command:  
   ```cl.exe *.c /link user32.lib gdi32.lib winmm.lib dbghelp.lib shell32.lib comdlg32.lib /out:ClassiCube.exe```

### Using MinGW-w64
Assuming you installed MinGW-w64 from [this source](https://sourceforge.net/projects/mingw-w64/):
1. Install MinGW-w64.
2. Open *Run Terminal* from the Start Menu or run `mingw-w64.bat` in the installation folder.
3. Navigate to the ClassiCube source code directory.
4. Run:  
   ```gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm```

### Using MinGW
Assuming you installed MinGW from [this source](https://osdn.net/projects/mingw/):
1. Install MinGW. Make sure to include the `mingw32-base-bin` and `msys-base-bin` packages.
2. Run `msys.bat` located in the *C:\MinGW\msys\1.0* folder.
3. Navigate to the ClassiCube source code directory.
4. Run:  
   ```gcc -fno-math-errno *.c -o ClassiCube.exe -mwindows -lwinmm```

### Using TCC (Tiny C Compiler)
**Setting up TCC:**
1. Download and extract `tcc-0.9.27-win64-bin.zip` from [this link](https://bellard.org/tcc/).
2. Download `winapi-full-for-0.9.27.zip` from the same site.
3. Copy the `winapi` folder and `_mingw_dxhelper.h` from `winapi-full-for-0.9.27.zip` into TCC's `include` folder.

**Compiling with TCC:**
1. Navigate to the ClassiCube source code directory.
2. Edit `ExtMath.c` by changing `fabsf` to `fabs` and `sqrtf` to `sqrt`.
3. Run:  
   ```tcc.exe -o ClassiCube.exe *.c -lwinmm -lgdi32 -luser32 -lcomdlg32 -lshell32```  
   *(You may need to specify the full path to `tcc.exe` if it's not in your environment variables.)*

---

## Compiling - Linux 

### Using gcc/clang
Install the necessary libraries. For Ubuntu, these include `libx11-dev`, `libxi-dev`, and `libgl1-mesa-dev`.  
Then, run the following command:  
```gcc -fno-math-errno src/*.c -o ClassiCube -rdynamic -lpthread -lX11 -lXi -lGL -ldl```

### Cross Compiling for Windows
**32-bit version:**  
```i686-w64-mingw32-gcc -fno-math-errno src/*.c -o ClassiCube.exe -mwindows -lwinmm```

**64-bit version:**  
```x86_64-w64-mingw32-gcc -fno-math-errno src/*.c -o ClassiCube.exe -mwindows -lwinmm```

### Raspberry Pi
For optimal performance on Raspberry Pi hardware, use this command:  
```gcc -fno-math-errno src/*.c -o ClassiCube -DCC_BUILD_RPI -rdynamic -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl```

---

## Compiling - macOS 

Run the following command to compile on macOS:  
```cc -fno-math-errno src/*.c src/*.m -o ClassiCube -framework Cocoa -framework OpenGL -framework IOKit -lobjc```  
*Note: Xcode may need to be installed prior to compiling.*

### Using Xcode GUI
1. Open the `misc/macOS/CCMAC.xcodeproj` project in Xcode.
2. Compile the project.

---

## Compiling - Android 

**Note:** If distributing a modified version, change the package ID from `com.classicube.android.client` to avoid conflicts with existing installations of ClassiCube.

### Using Android Studio GUI
1. Open the `android` folder in Android Studio.  
*(TODO: Add more detailed steps)*

### Using Command Line (Gradle)
1. Run `gradlew` in the `android` folder.  
*(TODO: Add more detailed steps)*

---

## Compiling - iOS 

The iOS version is incomplete and has only been tested on the iOS Simulator.

**Note:** If distributing a modified version, change the bundle ID from `com.classicube.ios.client` to avoid conflicts with existing installations of ClassiCube.

### Using Xcode GUI
1. Open the `ios/CCIOS.xcodeproj` project in Xcode.
2. Compile the project.

### Using Command Line (Xcode)
Run the following command:  
```xcodebuild -sdk iphoneos -configuration Debug```  
*(TODO: Add more detailed steps)*

## Compiling - Webclient 

To compile the webclient, use the following command:  
```emcc src/*.c -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=1Mb --js-library interop_web.js```

Please note that the generated JavaScript file may have some issues. For instructions on how to address these issues, refer to the [fixes guide](doc/compile-fixes.md#webclient-patches).

For more details on integrating the webclient into a website, please see the [integration guide](doc/hosting-webclient.md).

<details>
<summary><h2>Compiling - Consoles</h2></summary>

All console ports require expertise in homebrew development. If you are interested in contributing, please contact me (`unknownshadow200` on Discord).

<details>
<summary><h3>Nintendo Consoles (Click to Expand)</h3></summary>

#### Switch
To compile for the Nintendo Switch, run:  
```make switch```  
You will need [libnx](https://github.com/switchbrew/libnx) and [mesa](https://github.com/devkitPro/mesa).  

**Note:** It is highly recommended to install the precompiled devkitPro packages from [here](https://devkitpro.org/wiki/Getting_Started), including the `switch-dev`, `switch-mesa`, and `switch-glm` packages.

#### Wii U
To compile for the Wii U, run:  
```make wiiu```  
You will need [wut](https://github.com/devkitPro/wut/).  

**Note:** It is highly recommended to install the precompiled devkitPro packages from [here](https://devkitpro.org/wiki/Getting_Started), including the `wiiu-dev` group.

#### 3DS
To compile for the Nintendo 3DS, run:  
```make 3ds```  
You will need [libctru](https://github.com/devkitPro/libctru).  

**Note:** It is highly recommended to install the precompiled devkitPro packages from [here](https://devkitpro.org/wiki/Getting_Started), including the `3ds-dev` group.

#### Wii
To compile for the Wii, run:  
```make wii```  
You will need [libogc](https://github.com/devkitPro/libogc).  

**Note:** It is highly recommended to install the precompiled devkitPro packages from [here](https://devkitpro.org/wiki/Getting_Started), including the `wii-dev` group.

#### GameCube
To compile for the GameCube, run:  
```make gamecube```  
You will need [libogc](https://github.com/devkitPro/libogc).  

**Note:** It is highly recommended to install the precompiled devkitPro packages from [here](https://devkitpro.org/wiki/Getting_Started), including the `gamecube-dev` group.

#### Nintendo DS/DSi
To compile for the Nintendo DS/DSi, run:  
```make ds```  
You will need [BlocksDS](https://github.com/blocksds/sdk).

#### Nintendo 64
To compile for the Nintendo 64, run:  
```make n64```  
You will need the OpenGL branch of [libdragon](https://github.com/DragonMinded/libdragon/tree/opengl).

</details>

<details>
<summary><h3>Sony Consoles (Click to Expand)</h3></summary>

#### PlayStation Vita
To compile for the PlayStation Vita, run:  
```make vita```  
You will need [vitasdk](https://vitasdk.org/).

#### PlayStation Portable
To compile for the PlayStation Portable, run:  
```make psp```  
You will need [pspsdk](https://github.com/pspdev/pspsdk).  

**Note:** It is recommended to install the precompiled version of pspsdk from [this link](https://github.com/pspdev/pspdev/releases).

#### PlayStation 3
To compile for the PlayStation 3, run:  
```make ps3```  
You will need [PSL1GHT](https://github.com/ps3dev/PSL1GHT).

#### PlayStation 2
To compile for the PlayStation 2, run:  
```make ps2```  
You will need [ps2sdk](https://github.com/ps2dev/ps2sdk).

#### PlayStation 1
To compile for the PlayStation 1, run:  
```make ps1```  
You will need [PSn00bSDK](https://github.com/Lameguy64/PSn00bSDK/).

</details>

<details>
<summary><h3>Microsoft Consoles (Click to Expand)</h3></summary>

#### Xbox 360
To compile for the Xbox 360, run:  
```make 360```  
You will need [libxenon](https://github.com/Free60Project/libxenon).

#### Xbox (Original)
To compile for the original Xbox, run:  
```make xbox```  
You will need [nxdk](https://github.com/XboxDev/nxdk).

</details>

<details>
<summary><h3>SEGA Consoles (Click to Expand)</h3></summary>

#### Dreamcast
To compile for the SEGA Dreamcast, run:  
```make dreamcast```  
You will need [KallistiOS](https://github.com/KallistiOS/KallistiOS).

#### Saturn
To compile for the SEGA Saturn, run:  
```make saturn```  
You will need [libyaul](https://github.com/yaul-org/libyaul).

</details>

<details>
<summary><h2>Compiling - Other Platforms (Click to Expand)</h2></summary>

#### FreeBSD
Install the necessary packages: `libxi`, `libexecinfo`, `curl`, and `openal-soft` if needed.  
Run:  
```cc src/*.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### OpenBSD
Install the necessary packages: `libexecinfo`, `curl`, and `openal` if needed.  
Run:  
```cc src/*.c -o ClassiCube -I /usr/X11R6/include -I /usr/local/include -L /usr/X11R6/lib -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### NetBSD
Install the necessary packages: `libexecinfo`, `curl`, and `openal-soft` if needed.  
Run:  
```cc src/*.c -o ClassiCube -I /usr/X11R7/include -I /usr/pkg/include -L /usr/X11R7/lib -L /usr/pkg/lib -lpthread -lX11 -lXi -lGL -lexecinfo```

#### DragonflyBSD
Run:  
```cc src/*.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lXi -lGL -lexecinfo```

#### Solaris
Run:  
```gcc -fno-math-errno src/*.c -o ClassiCube -lsocket -lX11 -lXi -lGL```

#### Haiku
Install the `openal_devel` package if needed.  
Run:  
```cc -fno-math-errno src/*.c interop_BeOS.cpp -o ClassiCube -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker```

#### BeOS
Run:  
```cc -fno-math-errno src/*.c interop_BeOS.cpp -o ClassiCube -lGL -lbe -lgame -ltracker```

#### IRIX
Run:  
```gcc -fno-math-errno src/*.c -o ClassiCube -lGL -lX11 -lXi -lpthread -ldl```

#### SerenityOS
Install the SDL2 port if needed.  
Run:  
```cc src/*.c -o ClassiCube -lgl -lSDL2```

#### Classic Mac OS
Install Retro68 to compile (supports both M68k and PowerPC compiling).  
Run:  
```make macclassic_68k``` or ```make macclassic_ppc```  
(Note: The PowerPC build typically performs better.)

#### Other Systems
For other systems, you will need to write the necessary code. Refer to `portability.md` in the documentation folder for guidance.

</details>

</details>

## Documentation

ClassiCube is well-documented, with most functions and variables detailed in the `.h` files. Additional documentation is available in the `doc` and `misc` folders.

For common compilation errors and tips, see the [known issues section](doc/compile-fixes.md#common-compilation-errors).

### Tips 
- Press **Escape** to access the pause menu.
- Toggle **vsync** in the **Options** menu for better CPU performance.
- Press **F** to cycle view distance, which can improve performance.

---

## Open Source Technologies 
ClassiCube leverages many open-source technologies:
- [**curl**](https://curl.se/) – For HTTP/HTTPS requests on Linux and macOS.
- [**FreeType**](https://www.freetype.org/) ✍– Provides font rendering across all platforms.
- [**GCC**](https://gcc.gnu.org/) – The GNU Compiler Collection, used to compile the client for Linux.
- [**MinGW-w64**](http://mingw-w64.org/doku.php) 🏗– Toolchain to compile ClassiCube on Windows.
- [**Clang**](https://clang.llvm.org/)  – Another compiler for macOS and Linux.
- [**Emscripten**](https://emscripten.org/) – A compiler toolchain for running the client in web browsers.
- [**RenderDoc**](https://renderdoc.org/) – A powerful graphics debugger used for debugging rendering issues.
- [**BearSSL**](https://www.bearssl.org/) – Provides SSL/TLS support for security on consoles.
- [**libnx**](https://github.com/switchbrew/libnx) – Backend library for Nintendo Switch.
- [**Ryujinx**](https://github.com/Ryujinx/Ryujinx) – Emulator used to test the Nintendo Switch port.
- [**wut**](https://github.com/devkitPro/wut/) – Backend library for Wii U.
- [**Cemu**](https://github.com/cemu-project/Cemu) – Emulator used to test the Wii U port.
- [**libctru**](https://github.com/devkitPro/libctru) – Backend library for 3DS.
- [**citro3D**](https://github.com/devkitPro/citro3d) – Rendering backend for 3DS.
- [**Citra**](https://github.com/citra-emu/citra) – Emulator used to test the 3DS port.
- [**libogc**](https://github.com/devkitPro/libogc) – Backend library for the Wii and GameCube.
- [**libfat**](https://github.com/devkitPro/libfat) – Filesystem backend for Wii and GameCube.
- [**Dolphin**](https://github.com/dolphin-emu/dolphin) – Emulator used to test the Wii and GameCube ports.
- [**libdragon**](https://github.com/DragonMinded/libdragon) – Backend library for Nintendo 64.
- [**ares**](https://github.com/ares-emulator/ares) – Emulator used to test the Nintendo 64 port.
- [**BlocksDS**](https://github.com/blocksds/sdk) – Backend library for Nintendo DS.
- [**melonDS**](https://github.com/melonDS-emu/melonDS) – Emulator used to test the DS/DSi ports.
- [**vitasdk**](https://github.com/vitasdk) – Backend library for PS Vita.
- [**Vita3K**](https://github.com/Vita3K/Vita3K) – Emulator used to test the PS Vita port.
- [**pspsdk**](https://github.com/pspdev/pspsdk) – Backend library for PSP.
- [**PPSSPP**](https://github.com/hrydgard/ppsspp) – Emulator used to test the PSP port.
- [**PSL1GHT**](https://github.com/ps3dev/PSL1GHT) – Backend library for PS3.
- [**RPCS3**](https://github.com/RPCS3/rpcs3) – Emulator used to test the PS3 port.
- [**ps2sdk**](https://github.com/ps2dev/ps2sdk) – Backend library for PS2.
- [**PCSX2**](https://github.com/PCSX2/pcsx2) – Emulator used to test the PS2 port.
- [**PSn00bSDK**](https://github.com/Lameguy64/PSn00bSDK/) – Backend library for PS1.
- [**duckstation**](https://github.com/stenzek/duckstation) – Emulator used to test the PS1 port.
- [**libxenon**](https://github.com/Free60Project/libxenon) – Backend library for Xbox 360.
- [**nxdk**](https://github.com/XboxDev/nxdk) – Backend library for the original Xbox.
- [**xemu**](https://github.com/xemu-project/xemu) – Emulator used to test the Xbox port.
- [**cxbx-reloaded**](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded) – Emulator used to test the original Xbox port.
- [**KallistiOS**](https://github.com/KallistiOS/KallistiOS) – Backend library for the Dreamcast.
- [**GLdc**](https://github.com/Kazade/GLdc) – Rendering backend for Dreamcast.
- [**flycast**](https://github.com/flyinghead/flycast) – Emulator used to test the Dreamcast port.
- [**libyaul**](https://github.com/yaul-org/libyaul) – Backend library for SEGA Saturn.
- [**mednafen**](https://mednafen.github.io/) – Emulator used to test the Saturn port.

---

## Sound Credits 
ClassiCube uses sounds from [Freesound.org](https://freesound.org).  
For full credits, see [sound credits](doc/sound-credits.md).

---
