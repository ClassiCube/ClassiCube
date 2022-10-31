ClassiCube is a custom Minecraft Classic and ClassiCube client written in C that works on Windows, macOS, Linux, Android, iOS, FreeBSD, NetBSD, OpenBSD, Solaris, Haiku, and in a web browser.<br>
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](http://i.imgur.com/FCiwl27.png)


You can download the game [from here](https://www.classicube.net/download/) and the very latest builds [from here](https://www.classicube.net/nightlies/).

## Information

#### What ClassiCube is
* A complete re-implementation of Minecraft Classic 0.30, with **optional** additions
* Lightweight, minimal memory usage compared to original Minecraft Classic
* Much better performance than original Minecraft Classic
* Works with effectively all graphics cards that support OpenGL or Direct3D 9

#### What ClassiCube isn't
* It does not work with Minecraft Java or Bedrock edition servers
* It does not have a survival mode (nor will such a mode be added)
* It does not reimplement Minecraft Classic versions before 0.30

#### System requirements
* Windows: 95 or later
* macOS: 10.5 or later (can be compiled to work with 10.3/10.4 though)
* Linux: libcurl and libopenal
* Android: 2.3 or later

**Note:** When running from within VirtualBox, disable Mouse Integration, otherwise the camera will not work properly

#### Instructions
Initially, you will need to run ClassiCube.exe to download the required assets from minecraft.net. 
Just click 'OK' to the dialog menu that appears when you start the launcher.

**Singleplayer**
Run ClassiCube.exe, then click Singleplayer at the main menu.

**Multiplayer**
Run ClassiCube.exe. You can connect to LAN/locally hosted servers, ~~minecraft.net servers,~~ and classicube.net servers through the launcher.

###### *Stuck with OpenGL 1.1 due to old graphics hardware?*
If you're on Windows, you should first try using the MESA software renderer from [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/). Typically though, this occurs because you have not installed GPU drivers.

Otherwise, you will have to [compile the game yourself](#using-visual-studio-command-line). Don't forget to add `-DCC_BUILD_GL11` to the compilation command line so that the compiled game supports OpenGL 1.1.

## Compiling - Windows

##### Using Visual Studio
Open ClassiCube.sln *(File -> Open -> Project/Solution)* and compile it *(Build -> Build Solution)*.

If you get a ```The Windows SDK version 5.1 was not found``` compilation error, [see here for how to fix](doc/compile-fixes.md#visual-studio-unsupported-platform-toolset)

##### Using Visual Studio (command line)
1. Use 'Developer Tools for Visual Studio' from Start Menu
2. Navigate to directory with game's source code
3. Enter `cl.exe *.c /link user32.lib gdi32.lib crypt32.lib ws2_32.lib wininet.lib winmm.lib dbghelp.lib shell32.lib comdlg32.lib /out:ClassiCube.exe`

##### Using MinGW-w64
I am assuming you used the installer from https://sourceforge.net/projects/mingw-w64/
1. Install MinGW-W64
2. Use either *Run Terminal* from Start Menu or run *mingw-w64.bat* in the installation folder
3. Navigate to directory with game's source code
4. Enter `gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32`

##### Using MinGW
I am assuming you used the installer from http://www.mingw.org/
1. Install MinGW. You need mingw32-base-bin and msys-base-bin packages.
2. Run *msys.bat* in the *C:\MinGW\msys\1.0* folder.
3. Navigate to directory with game's source code
4. Enter `gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32`

##### Using TCC
I am assuming you used `tcc-0.9.27-win64-bin.zip` from https://bellard.org/tcc/
1. Extract the .zip file
2. In `ExtMath.C`, change `fabsf` to `fabs` and `sqrtf` to `sqrtf`
3. In TCC's `include/math.h`, remove the inline definition for `fabs` at around line 217
4. In TCC's `lib/kernel32.def`, add missing `RtlCaptureContext`
5. Add missing include files from `winapi-full-for-0.9.27.zip` as required
6. ???

## Compiling - Linux

##### Using gcc/clang

Install appropriate libs as required. For ubuntu these are: libx11-dev, libxi-dev and libgl1-mesa-dev

```gcc *.c -o ClassiCube -rdynamic -lm -lpthread -lX11 -lXi -lGL -ldl```

##### Cross compiling for Windows (32 bit):

```i686-w64-mingw32-gcc *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp```

##### Cross compiling for Windows (64 bit):

```x86_64-w64-mingw32-gcc *.c -o ClassiCube.exe -mwindows -lwinmm -limagehlp```

##### Raspberry Pi
Although the regular linux compiliation flags will work fine, to take full advantage of the hardware:

```gcc *.c -o ClassiCube -DCC_BUILD_RPI -rdynamic -lm -lpthread -lX11 -lXi -lEGL -lGLESv2 -ldl```

## Compiling - macOS

##### Using gcc/clang (32 bit)

```cc *.c -o ClassiCube -framework Carbon -framework AGL -framework OpenGL -framework IOKit```

##### Using gcc/clang (64 bit)

```cc *.c interop_cocoa.m -o ClassiCube -framework Cocoa -framework OpenGL -framework IOKit -lobjc```

## Compiling - for Android

##### Using Android Studio GUI

Open `android` folder in Android Studio (TODO explain more detailed)

##### Using command line (gradle)

Run `gradlew` in android folder (TODO explain more detailed)

## Compiling - for iOS

iOS version will have issues as it's incomplete and only tested in iOS Simulator

##### Using Xcode GUI

Import `ios/CCIOS.xcodeproj` project into Xcode (TODO explain more detailed)

##### Using command line (Xcode)

`xcodebuild -sdk iphoneos -configuration Debug` (TODO explain more detailed)

## Compiling - other desktop OSes

#### FreeBSD

Install libexecinfo, curl and openal-soft package if needed

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

```gcc *.c -o ClassiCube -lm -lsocket -lX11 -lXi -lGL```

#### Haiku

Install openal_devel and libexecinfo_devel package if needed

```cc *.c Window_Haiku.cpp -o ClassiCube -lm -lexecinfo -lGL -lnetwork -lstdc++ -lbe -lgame -ltracker```

#### SerenityOS

Install SDL2 port if needed

```cc *.c -o ClassiCube -lgl -lSDL2```

## Compiling - other

#### Web

```emcc *.c -s ALLOW_MEMORY_GROWTH=1 --js-library interop_web.js```

The generated javascript file has some issues. [See here for how to fix](doc/compile-fixes.md#webclient-patches)

##### Other

You'll have to write the necessary code. You should read portability.md in doc folder.

## Documentation

Functions and variables in .h files are mostly documented.

Further information (e.g. style) for the game's source code can be found in the doc and misc folders.

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
* [RenderDoc](https://renderdoc.org/) - graphics debugging

## Sound Credits
ClassiCube uses sounds from [Freesound.org](https://freesound.org)<br>
Full credits are listed in [doc/sound-credits.md](doc/sound-credits.md)
