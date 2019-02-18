ClassiCube is a custom Minecraft Classic and ClassiCube client written in C that works on Windows, Linux and OSX.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](http://i.imgur.com/FCiwl27.png)


You can grab the latest stable binaries [here](https://cs.classicube.net/c_client/release).

You can grab the very builds [here](http://cs.classicube.net/c_client/latest)

#### What ClassiCube is
* Works with both ~~minecraft.net~~(classic has been removed by Mojang) and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL or Direct3D 9.
* Provides single-player support, and both a flatgrass and vanilla-type map generator.

It **does not** work with 'modern/premium' Minecraft servers.

#### Requirements
* Windows: XP or later.
* OSX: Only tested on OSX 10.5, may or may not work on later versions.
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

###### *Linux specific*
*You will likely need to do `chmod +x ClassiCube` before running the game.

#### Tips
* Press escape (after joining a world) or pause to switch to the pause menu.
* Pause menu -> Options -> Controls lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to on will minimise CPU usage, while off will maximimise chunk loading speed.
* Press F to cycle view distance. A smaller number of visible chunks can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see the list of built in commands, type `/client`.
* To see help for a given built in command, type `/client help <command name>`.

### Compiling on linux

Install appropriate libs as required. For ubuntu this means: libx11-dev, libgl1-mesa-dev, libopenal-dev, libcurl4-gnutls-dev or libcurl4-openssl-dev

Compiling for linux: 

```gcc *.c -o ClassiCube -lX11 -lpthread -lGL -lm -lcurl -lopenal -ldl```

Cross compiling for windows:

```i586-mingw32msvc-gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9```

Explicitly:

```i586-mingw32msvc-gcc *.c -DCC_BUILD_MANUAL -DCC_BUILD_WIN -DCC_BUILD_D3D9 -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -ld3d9```

```i586-mingw32msvc-gcc *.c -DCC_BUILD_MANUAL -DCC_BUILD_WIN -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -limagehlp -lcrypt32 -lopengl32```

### Documentation

Functions and variables in .h files are mostly documented.

General information (e.g. portablity) for the game's source code can be found in the misc folder.