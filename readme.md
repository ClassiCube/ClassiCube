ClassicalSharp is a custom Minecraft Classic client written in C# that works on both Windows, Linux and OSX.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](https://cloud.githubusercontent.com/assets/6509348/10800494/288b4f00-7e06-11e5-8344-5df33625cc8b.png)


You can grab the latest stable binaries [here](https://github.com/UnknownShadow200/ClassicalSharp/releases).

You can grab the very latest OpenGL build [here](http://cs.classicube.net/latest.zip) and the latest Direct3D 9 build [here](http://cs.classicube.net/latest.DirectX.zip).

#### What ClassicalSharp is
* Works with both minecraft.net and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL or Direct3D 9.
* Provides single-player support. (only flatgrass generator, but can load .dat, .cw and .fcm maps)

It **does not** work with 'modern/premium' Minecraft servers.

#### Requirements
* Windows: .NET framework 2.0 or Mono. (Vista and later have .NET framework 2.0 built in)
ClassicalSharp has been verified to work on Windows 2000, unsure about ME and 98.
* Linux: Mono.
* Mac OS X: Mono. (Tested, but not thoroughly and may still crash)

#### Instructions
Initially, you will need to run launcher.exe to download the required assets from minecraft.net. 
Just click 'OK' to the dialog box that appears when you start the launcher.

**Singleplayer**
Run classicalsharp.exe.

**Multiplayer**
Run launcher.exe. You can connect to LAN/locally hosted servers, minecraft.net servers, and classicube.net servers through the launcher.

Alternatively, you can use [CSLauncher](https://github.com/umby24/CSLauncher/releases) instead of the default launcher included with ClassicalSharp - 
this launcher is very similar to ClassiCube's launcher and is simpler to use. (Thanks Umby24 for making this launcher)

###### *Mono specific*
*You must use either build using the Mono compiler or define `__MonoCS__` when building, otherwise you will get runtime errors when decompressing the map using Mono.*

*Also when using older mono versions, you may need to run `mozroots --import --sync` to import trusted root certificates, otherwise you will get an 'Error writing headers' exception.*

#### Tips
* Press escape (after joining a world) to switch to the pause menu.
* Pause menu -> Key bindings lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to off will minimise CPU usage, while on will maximimise chunk loading speed.
* Press F to cycle view distance. A smaller number of visible chunks can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see a list of all built in commands, type `/client commands`.
* To see help for a given built in command, type `/client help <command name>`.