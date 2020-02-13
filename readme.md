ClassiCube is a custom Minecraft Classic and ClassiCube client written in C that works on Windows, OSX, Linux, BSD, Solaris, and in a browser.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](http://i.imgur.com/FCiwl27.png)


You can grab the latest stable binaries [from here](https://www.classicube.net/download/) and the very latest builds [from here](https://www.classicube.net/nightlies/).

#### What ClassiCube is
* Works with both ~~minecraft.net~~(classic has been removed by Mojang) and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL or Direct3D 9.
* Provides single-player support, and both a flatgrass and vanilla-type map generator.

It **does not** work with 'modern/premium' Minecraft servers.

#### Requirements
* Windows: 2000 or later. (Windows 98 with KernelEx also *technically* works)
* OSX: OSX 10.5 or later. (Can be compiled to work with 10.4 though)
* Linux: libcurl and libopenal.

#### Instructions
Initially, you will need to run ClassiCube.exe to download the required assets from minecraft.net. 
Just click 'OK' to the dialog menu that appears when you start the launcher.

**Singleplayer**
Run ClassiCube.exe, then click Singleplayer at the main menu.

**Multiplayer**
Run ClassiCube.exe. You can connect to LAN/locally hosted servers, ~~minecraft.net servers,~~ and classicube.net servers through the launcher.

###### *Windows specific*
*If you are stuck using the built-in OpenGL 1.1 software renderer, you can use the MESA software renderer from either [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/) or [here](https://wiki.qt.io/Cross_compiling_Mesa_for_Windows) for slightly better performance. Typically though, this occurs because you have not installed GPU drivers.*

### Compiling

#### Windows

##### Compiling with Visual Studio:
Open ClassiCube.sln and compile it.

If you get a ```The Windows SDK version 5.1 was not found``` compilation error, [see here for how to fix](misc/compile-fixes.md#visual-studio-unsupported-platform-toolset)

##### Compiling with MinGW-w64
I am assuming you used the installer from https://sourceforge.net/projects/mingw-w64/
1. Install MinGW-W64
2. Use either *Run Terminal* from Start Menu or run *mingw-w64.bat* in the installation folder
3. Compile with the same flags as under **Cross Compiling for windows**, but use *gcc* instead of *i586-mingw32msvc-gcc*

##### Compiling with MinGW
I am assuming you used the installer from http://www.mingw.org/
1. Install MinGW. You need mingw32-base-bin and msys-base-bin packages.
2. Run *msys.bat* in the *C:\MinGW\msys\1.0* folder.
3. Compile with the same flags as under **Cross Compiling for windows**, but use *gcc* instead of *i586-mingw32msvc-gcc*

#### Linux

Install appropriate libs as required. For ubuntu these are: libx11-dev, libgl1-mesa-dev, libopenal-dev, libcurl4-gnutls-dev or libcurl4-openssl-dev

```gcc *.c -o ClassiCube -lm -lpthread -lX11 -lGL -lcurl -lopenal -ldl```

##### Cross compiling for windows:

```i586-mingw32msvc-gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9```

##### Raspberry pi
Although the regular linux compiliation flags will work fine, to take full advantage of the hardware:

```gcc *.c -o ClassiCube -DCC_BUILD_RPI -lm -lpthread -lX11 -lEGL -lGLESv2 -lcurl -lopenal -ldl```

#### Mac OSX (32 bit)

```gcc *.c -o ClassiCube -framework Carbon -framework AGL -framework OpenAL -framework OpenGL -lcurl```

#### Mac OSX (64 bit)

```gcc *.c -o ClassiCube -framework Cocoa -framework OpenAL -framework OpenGL -lcurl -lobjc```

#### FreeBSD

```clang *.c -o ClassiCube -I /usr/local/include -L /usr/local/lib -lm -lpthread -lX11 -lGL -lcurl -lopenal -lexecinfo```

#### OpenBSD

Install libexecinfo package if needed.

```gcc *.c -o ClassiCube -I /usr/X11R6/include -I /usr/local/include -L /usr/X11R6/lib -L /usr/local/lib -lX11 -lGL -lcurl -lopenal -lexecinfo```

#### NetBSD

```gcc *.c -o ClassiCube -I /usr/X11R7/include -I /usr/pkg/include -L /usr/X11R7/lib -L /usr/pkg/lib  -lpthread -lX11 -lGL -lcurl -lopenal -lexecinfo```

#### Alpine Linux

```gcc *.c -o ClassiCube -lm -lpthread -lX11 -lGL -lcurl -lopenal -ldl -lexecinfo```

#### Solaris

```gcc *.c -o ClassiCube -lm -lsocket -lX11 -lGL -lcurl -lopenal```

NOTE: You have to change entry->d_type == DT_DIR to Directory_Exists(&path) (TODO do this automatically)

#### Web

```emcc *.c -s FETCH=1 -s ALLOW_MEMORY_GROWTH=1 --preload-file texpacks/default.zip```

The generated javascript file has some issues. [See here for how to fix](misc/compile-fixes.md#webclient-patches)

##### Other

You'll have to write the necessary code. You should read portability.md in misc folder.

### Known compilation errors

[Fixes for compilation errors when using musl or old glibc for C standard library](misc/compile-fixes.md#common-compilation-errors)

### Documentation

Functions and variables in .h files are mostly documented.

Further information (e.g. portablity, style) for the game's source code can be found in the misc folder.

#### Tips
* Press escape (after joining a world) or pause to switch to the pause menu.
* Pause menu -> Options -> Controls lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to on will minimise CPU usage, while off will maximimise chunk loading speed.
* Press F to cycle view distance. A smaller number of visible chunks can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see the list of built in commands, type `/client`.
* To see help for a given built in command, type `/client help <command name>`.
