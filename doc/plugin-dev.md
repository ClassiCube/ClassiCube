This document details how to compile a basic plugin in Visual Studio, MinGW, or GCC.

To find the functions and variables available for use in plugins, look for CC_API/CC_VAR in the .h files.

[Source code of some actual plugins](https://github.com/UnknownShadow200/ClassiCube-Plugins/tree/master/src)

### Setup

You need to download and install either Visual Studio, MinGW, or GCC.

*Note: MinGW/GCC are relatively small, while Visual Studio is gigabytes in size.  
If you're just trying to compile a plugin on Windows you might want to use MinGW. See main readme.*

I assume your directory is structured like this:
```
src/...
TestPlugin.c
```
Or in other words, in a directory somewhere, you have a file named ```TestPlugin.c```, and then a sub-directory named ```src``` which contains the game's source code.

### Basic plugin
```C
#include "src/Chat.h"
#include "src/Game.h"
#include "src/String.h"

static void TestPlugin_Init(void) {
        cc_string msg = String_FromConst("Hello world!");
        Chat_Add(&msg);
}

int Plugin_ApiVersion = 1;
struct IGameComponent Plugin_Component = { TestPlugin_Init };
```
Here's the idea for a basic plugin that shows "Hello world" in chat when the game starts. Alas, this won't compile...

### Basic plugin boilerplate
```C
#ifdef CC_BUILD_WIN
    #define CC_API __declspec(dllimport)
    #define CC_VAR __declspec(dllimport)
    #define EXPORT __declspec(dllexport)
#else
    #define CC_API
    #define CC_VAR
    #define EXPORT __attribute__((visibility("default")))
#endif

#include "src/Chat.h"
#include "src/Game.h"
#include "src/String.h"

static void TestPlugin_Init(void) {
        cc_string msg = String_FromConst("Hello world!");
        Chat_Add(&msg);
}

EXPORT int Plugin_ApiVersion = 1;
EXPORT struct IGameComponent Plugin_Component = { TestPlugin_Init };
```
With this boilerplate, we're ready to compile the plugin.
All plugins require this boilerplate, so feel free to copy and paste it.

---

### Writing plugins in C++
When including game headers, they must be surrounded with `extern "C"`, i.e.
```C
extern "C" {
#include "src/Chat.h"
#include "src/Game.h"
#include "src/String.h"
}
```
Otherwise you will get obscure `Undefined reference` errors when compiling.

Exported plugin functions must be surrounded with `extern "C"`, i.e.
```C
extern "C" {
EXPORT int Plugin_ApiVersion = 1;
EXPORT struct IGameComponent Plugin_Component = { TestPlugin_Init };
}
```
Otherwise your plugin will not load. (you'll see `error getting plugin version` in-game)

---

### Linux
#### For Linux
**Compiling**

```gcc TestPlugin.c -o TestPlugin.so -shared -fPIC```

Then put ```TestPlugin.so``` into your game's ```plugins``` folder. Done.

#### Cross compile for Windows 32 bit
**Prerequisites:**

1) Compile the game, see ```Cross compiling for windows``` in main readme
2) Rename compiled executable to ClassiCube.exe
3) Install the ```mingw-w64-tools``` package

TODO: this also works for mingw. clean this up.

TODO: also document alternate method of compiling the game using --out-implib

**Compiling**

First, generate list of exported symbols: 

```gendef src/ClassiCube.exe```

Next create a linkable library: 

```i686-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe```

Finally compile the plugin:

```i686-w64-mingw32-gcc TestPlugin.c -o TestPlugin.dll -s -shared -L . -lClassiCube```

Then put ```TestPlugin.dll``` into your game's ```plugins``` folder. Done.

##### Ultra small dlls
If you **ONLY** use code from the game (no external libraries and no C standard library functions), add ```-nostartfiles -Wl,--entry=0``` to the compile flags

This step isn't necessary, the dll works fine without it. But it does reduce the size of the dll from 11 to 4 kb.

#### Cross compile for Windows 64 bit
Repeat the steps of *Cross compile for Windows 32 bit*, but use ```x86_64-w64-mingw32``` instead of ```i686-w64-mingw32```


### Windows
TODO when I have some more time...

**Prerequisites:**

1) Compile the game, see ```Cross compiling for windows``` in main readme
2) Find the `ClassiCube.lib` that was generated when compiling the game. Usually it is in either `src\x64\Debug` or `src\x86\Debug`.
3) Add a new `Empty Project` to the ClassiCube solution, then add the plugin .c files to it

Note: If the plugin provides a .vcxproj file, you can skip step 2 and just open that project file instead.

**Configuration**

Right click the plugin project in the `Solution Explorer` pane, then click `Properties`

TODO: add screenshots here

TODO: may need to configure include directories

1) In `Configuration properties` -> `General`, make sure `Configuration type` is set to `Dynamic library (.DLL)`

2) In `Configuration properties` -> `Linker` -> `Input`, click the dropdown button for `Additional Dependencies`, then click `Edit`. Add the full path to `ClassiCube.lib`, then click `OK`

**Compiling**

Build the project. There should be a line in the build output that tells you where you can find the .dll file like this:
```
Project1.vcxproj -> C:\classicube-dev\testplugin\src\x64\Debug\Project1.dll
``` 
