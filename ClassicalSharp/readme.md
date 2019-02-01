ClassicalSharp is a custom Minecraft Classic client written in C# that works on Windows, Linux and OSX.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

![screenshot_n](http://i.imgur.com/FCiwl27.png)


You can grab the latest stable binaries [here](https://github.com/UnknownShadow200/ClassicalSharp/releases).

You can grab the very latest OpenGL build [here](http://cs.classicube.net/latest.zip) and the latest Direct3D 9 build [here](http://cs.classicube.net/latest.DirectX.zip).

#### What ClassicalSharp is
* Works with both ~~minecraft.net~~(classic has been removed by Mojang) and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL or Direct3D 9.
* Provides single-player support, and both a flatgrass and vanilla-type map generator.

It **does not** work with 'modern/premium' Minecraft servers.

#### Requirements
* Windows: .NET framework 2.0 or Mono. (Vista and later have .NET framework 2.0 built in)
ClassicalSharp has been verified to work on Windows 2000 and later. The OpenGL 1.1 build has been verified to work in a Windows 98 virtual machine, however has not been tested on real hardware running Windows 98.
* Linux and Mac OSX: Either Wine or Mono.

#### Instructions
Initially, you will need to run launcher.exe to download the required assets from minecraft.net. 
Just click 'OK' to the dialog menu that appears when you start the launcher.

**Singleplayer**
Run ClassicalSharp.exe.

**Multiplayer**
Run Launcher.exe. You can connect to LAN/locally hosted servers, ~~minecraft.net servers,~~ and classicube.net servers through the launcher.

###### *Mono specific*
*If you are using Wine, you need to mark both ClassicalSharp.exe and Launcher.exe as executable, then type this into the terminal: `./Launcher.exe`
If you are using Mono, you just need to type `mono Launcher.exe` into the terminal.*

*You may get TrustFailure/SendFailure errors trying to download https:// texture packs, [see here](https://github.com/duplicati/duplicati/wiki/SSL-TLS-support-in-Mono) for how to fix.*

*For wine, you'll need to navigate to the internal folder of mono (usually ~/.wine/drive_c/windows/mono/mono-2.0/lib/mono/4.5/), and use say 'wine mozroots.exe' instead.*

###### *Windows specific*
*If you are stuck using the built-in OpenGL 1.1 software renderer, you can use the MESA software renderer from either [here](http://download.qt.io/development_releases/prebuilt/llvmpipe/windows/) or [here](https://wiki.qt.io/Cross_compiling_Mesa_for_Windows) for slightly better performance. Typically though, this occurs because you have not installed GPU drivers.*

#### Tips
* Press escape (after joining a world) or pause to switch to the pause menu.
* Pause menu -> Options -> Controls lists all of the key combinations used by the client. 
* Note that toggling 'vsync' to on will minimise CPU usage, while off will maximimise chunk loading speed.
* Press F to cycle view distance. A smaller number of visible chunks can improve performance.

* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* To see the list of built in commands, type `/client`.
* To see help for a given built in command, type `/client help <command name>`.