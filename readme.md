ClassicalSharp is a custom Minecraft Classic client written in C# that works on both Windows and Linux.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

You can grab the latest stable binaries [here](https://github.com/UnknownShadow200/ClassicalSharp/releases).
You can grab the very latest OpenGL build [here](http://cs.classicube.net/latest.zip) and the latest Direct3D 9 build [here](http://cs.classicube.net/latest.DirectX.zip).

#### What ClassicalSharp is
* Works with both minecraft.net and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL or Direct3D 9.
* Provides single-player support. (only flatgrass generator, but can load .dat and .fcm maps)

It **does not** work with 'modern/premium' Minecraft servers.

#### Requirements
* Windows: .NET framework 2.0 or Mono. (Vista and later have .NET framework 2.0 built in)
* Linux: Mono.
* Mac OS X: Mono. (Not tested at all yet - likely just crashes)

#### Instructions
Initially, you will need to run launcher.exe to download the required assets from minecraft.net. 
Just click 'OK' to the dialog box that appears when you start the launcher.

**Singleplayer**
Run classicalsharp.exe.

**Multiplayer**
Run launcher.exe. You can connect to LAN/locally hosted servers, minecraft.net servers, and classicube.net servers through the launcher.

##### Mono specific notes
*You must use either build using Mono compiler or define `__MonoCS__` when building, otherwise you will get runtime errors when decompressing the map using Mono.*
*Also, if you are using an older mono version, you may need to run `mozroots --import --sync` to import trusted root certificates, otherwise you will get an 'Error writing headers' exception.*

#### Key combinations
* Press escape (after joining a world) to switch to the pause menu. 
* Pause menu -> Key mappings lists all of the key combinations used by the client. 

Some points to note:
* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* Press F6 to cycle view distance. A smaller number of visible chunks can improve performance.
* Press F7 to toggle VSync on or off. (on minimises CPU usage)

#### Client commands
* To see a list of all built in commands, type `/client commands`.
* To see help for a given built in command, type `/client help <command name>`.