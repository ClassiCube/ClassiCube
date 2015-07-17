ClassicalSharp is a custom Minecraft Classic client written in C# that works on both Windows and Linux.
**It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.**

You can get the latest binaries [here](https://github.com/UnknownShadow200/ClassicalSharp/releases).

### Note on the project
Due to time constraints, I am no longer really actively working on the project. Reported bugs and the like will probably still be fixed and small changes implemented, but **work on version 2.0 has stopped for now.** 

Version 0.8 (released along with this message) implements the entire CPE specification (excluding TextHotKey), and should be stable and fast enough for you to use.

#### What ClassicalSharp is
* Works with both minecraft.net and classicube.net accounts.
* Lightweight, minimal memory usage compared to the standard client.
* Works with effectively all graphics cards that support OpenGL.

It does not:
* Work with 'modern/premium' Minecraft servers.
* Provide single-player support.

#### Requirements
* Windows: .NET framework 2.0 or Mono. (Vista and later have .NET framework 2.0 built in)
* Linux: Mono.
* Mac OS X: Mono. (Not tested at all yet - likely just crashes)

*When building from source and targeting Mono, either use the Mono compiler or
define `__MonoCS__` when building, otherwise you will get runtime errors when decompressing the map on Mono.*

#### Instructions
The simple way to use ClassicalSharp is to use the launcher application. You can connect to LAN/locally hosted servers, minecraft.net servers, and classicube.net servers through the launcher.

Note that the first time you run the launcher, a dialog box will pop up saying: *"Some required resources weren't found. Would you like to download them now?"* Just click OK. 
(This is necessary because I cannot legally redistribute the assets of Minecraft Classic with the application)

*Alternatively, you can pass command line arguments directly to the client. These are expected to be in the form: `<username> <mppass> <ip> <port> <skin server>`, where skin server is optional.*

#### Key combinations
* Press escape (after joining a world) to switch to the pause menu. 
* The pause menu lists all of the key combinations used by the client. 
  These key combinations can be reassigned by clicking on a key combination.
* Press escape or click "Back to game" to return to the game.

Some points to note:
* If the server has disabled hacks, key combinations such as fly and speed will not do anything.
* Press F6 to change view distance. A smaller number of visible chunks can improve performance.
* Press F7 to toggle VSync on (minimises CPU usage) or off (maximises chunk loading speed).

#### Client commands
* To see a list of all built in commands, type `/client commands`.
* To see help for a given built in command, type `/client help <command name>`.