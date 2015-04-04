ClassicalSharp is a custom Minecraft Classic client written in C#. 
It is not affiliated with (or supported by) Mojang AB, Minecraft, or Microsoft in any way.

You can get the latest binaries from github.com/UnknownShadow200/ClassicalSharp/releases

=== What ClassicalSharp is === 
* Works with both Minecraft.net and ClassiCube.net
* Lightweight, minimal memory usage compared to standard client.
* Should work with effectively all graphics cards that support OpenGL.

It does not:
* Work with 'modern/premium' Minecraft servers.
* Provide singleplayer support.

=== Requirements ===
* ClassicalSharp requires the .NET Framework 2.0. 
  (Windows Vista and later already have this built in)
* Mono (I haven't tested though) should also work.


=== Instructions ===
To use ClassicalSharp, you can either
A) Use the launcher
B) Pass command line arguments directly to classicalsharp.exe

* Note that the first time you run the launcher, a dialog box will pop up with the message 
"Some required resources weren't found." Just click OK. (This is because I cannot redistribute 
the assets of Minecraft Classic with the application as they are the copyrighted property of Mojang)

The launcher is divided into 3 tabs. They are:
1) Local tab
This is for connecting to classic servers that are hosted on the same computer as you, 
or that are in the same LAN network.
* You do not have to be connected to the internet to play in this mode. (Note that you won't see 
custom skins if you are not connected to the internet, however)

Click "connect" to start the client.

2) Minecraft.net
### Note: Mojang appears to have deleted all of the unpaid free accounts. Premium accounts should still
### work though. The public servers list on minecraft.net is also completely stuffed. There are servers
### that appear on the list that are either fake, or don't actually exist anymore. 
### You are probably better off just using ClassiCube.net.

You will need to provide the username and password for your minecraft.net account. Then click sign in.

If sign-in was successful, two new tabs should appear beside 'Sign in'.
You can then either: 
A) Browse the public servers list and double-click on a server (Which will then take you to the 'minecraft.net server' tab with the hash filled in for you) 
B) Directly enter in a hash in the 'minecraft.net server' tab. 

Then click "connect". The launcher will then download the necessary information about the server from minecraft.net and start the client.

3) ClassiCube.net
The process is the exact same as described under minecraft.net. (except that signing in, downloading skins, and retrieving server information is done through classicube.net)


=== Default key combinations ===
These can be reassigned in the pause menu (hit escape, then click on the key mapping you want to change)
W     | Move forward
S     | Move back
A     | Move left
D     | Move right
Space | Jump
R     | Respawn (if the server has not forbidden respawning)
Y     | Set respawn point to current position
T     | Open the chat bar
Enter | Send the chat currently in the chat bar
Escape| Pause the game
B     | Opens the inventory window, allowing you to change which blocks are in your hotbar.
F12   | Takes a screenshot of the game, and saves it to the screenshots folder.
F11   | Toggles whether the game is running in full screen.
F5    | Toggles 3rd person camera. (if the server has not forbidden this)
F7    | Toggles whether VSync is used. This limits the framerate to minimise CPU usage, but you can toggle
         this off to increase chunk loading speed.
F6    | Changes render distance. This can be used to improve performance, as chunks outside the view
          distance aren't loaded or rendered.
Z     | Toggles flying mode. (if the server has not forbidden this)
LShift| When held down, movement speed is increased. (if the server has not forbidden this)
X     | Toggles no clip mode, allowing you to fly through all blocks. (if the server has not forbidden this)
Q     | If flying is enabled, when held down causes you to move vertically up.
E     | If flying is enabled, when held down causes you to move vertically down.
Tab   | Displays the list of players in the same map as you.