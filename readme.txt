ClassicalSharp is a custom MineCraft Classic client written in C#. 
It is not affiliated with (or supported by) Mojang AB, MineCraft, or Microsoft in any way.

You can get the latest binaries from github.com/UnknownShadow200/ClassicalSharp/releases

=== What ClassicalSharp is === 
* Works with both MineCraft.net and ClassiCube.net
* Lightweight, minimal memory usage compared to standard client.
* Should work with effectively all graphics cards that support OpenGL.

It does not:
* Work with 'modern/premium' MineCraft servers.
* Provide single-player support.

=== Requirements ===
* either A) The .NET Framework 2.0. 
  (Windows Vista and later already have this built in)
* or B) In theory, Mono (I haven't tested though) should also work.

=== Instructions ===
To use ClassicalSharp, you can either
A) Use the launcher
B) Pass command line arguments directly to classicalsharp.exe

* Note that the first time you run the launcher, a dialog box will pop up with the message 
"Some required resources weren't found." Just click OK. (This is because I cannot redistribute 
the assets of Minecraft Classic with the application as they are the copyrighted property of Mojang)

The launcher interface should be straightforward. If you are confused about how to use the launcher, 
please read "launcher instructions.txt"

=== Key combinations ===
Press escape (after joining the first world) to switch to the pause menu. 
The pause menu lists all of the key combinations used by the client.
Key combinations can be reassigned in the pause menu by clicking on a key combination.
Press escape or click "Back to game" to return to the game.

* Note that if the server has disabled hacks, some of the key combinations will have no affect.
* Pressing F6 to change view distance can improve performance by limiting the number of visible chunks.
* Pressing F7 to toggle VSync on or off. On minimises CPU usage, whereas off maximises chunk loading speed.