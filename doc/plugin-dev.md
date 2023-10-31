This document details how to compile a basic plugin in Visual Studio, MinGW, or GCC/Clang.

To find the functions and variables available for use in plugins, look for `CC_API`/`CC_VAR` in the .h files.

[Source code of some actual plugins](https://github.com/UnknownShadow200/ClassiCube-Plugins/)

### Setup

You need to download and install either Visual Studio, MinGW, or GCC/Clang.

*Note: MinGW/GCC/Clang are relatively small, while Visual Studio is gigabytes in size.  
If you're just trying to compile a plugin on Windows you might want to use MinGW. See main readme.*

I assume your directory is structured like this:
```
src/...
TestPlugin.c
```
Or in other words, in a directory somewhere, you have a file named `TestPlugin.c`, and then a sub-directory named `src` which contains the game's source code.

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
#ifdef _WIN32
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
When including headers from ClassiCube, they **must** be surrounded with `extern "C"`, i.e.
```C
extern "C" {
#include "src/Chat.h"
#include "src/Game.h"
#include "src/String.h"
}
```
Otherwise you will get obscure `Undefined reference` errors when compiling.

Exported plugin functions **must** be surrounded with `extern "C"`, i.e.
```C
extern "C" {
EXPORT int Plugin_ApiVersion = 1;
EXPORT struct IGameComponent Plugin_Component = { TestPlugin_Init };
}
```
Otherwise your plugin will not load. (you'll see `error getting plugin version` in-game)

---

## Compiling

Plugin compilation instructions differs depending on the compiler and operating system

### Linux

[Compiling using gcc or clang](plugin-dev.md#using-gcc-or-clang)

[Cross compiling for Windows 32-bit](plugin-dev.md#cross-compiling-for-windows-32-bit-using-mingw-w64)

[Cross compiling for Windows 64-bit](plugin-dev.md#cross-compiling-for-windows-64-bit-using-mingw-w64)

### macOS

[Compiling using gcc or clang](plugin-dev.md#using-gcc-or-clang-1)

### Windows

[Compiling using Visual Studio](plugin-dev.md#using-visual-studio)

[Compiling using mingw-w64](plugin-dev.md#using-mingw-w64)

---

## Compiling - Linux

### Using gcc or clang

#### Compiling

`cc TestPlugin.c -o TestPlugin.so -shared -fPIC`

Then put `TestPlugin.so` into your game's `plugins` folder. Done.

### Cross compiling for Windows 32 bit using mingw-w64

#### Setup

1) Create `ClassiCube.exe` by either:
    1) Compiling the game, see `Cross compiling for windows (32 bit)` in [main readme](/readme.md#cross-compiling-for-windows-32-bit)
    2) Downloading 32 bit ClassiCube from https://www.classicube.net/download/#dl-win
2) Install the `mingw-w64-tools` package (if it isn't already)
3) Generate the list of exported symbols from `ClassiCube.exe` by running:
    * `gendef ClassiCube.exe`
4) Create a linkable library from the exported symbols list by running: 
    * `i686-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe`

TODO: also document alternate method of compiling the game using --out-implib

#### Compiling

`i686-w64-mingw32-gcc TestPlugin.c -o TestPlugin.dll -s -shared -L . -lClassiCube`

Then put `TestPlugin.dll` into your game's `plugins` folder. Done.

### Cross compiling for Windows 64 bit using mingw-w64

#### Setup

1) Create `ClassiCube.exe` by either:
    1) Compiling the game, see `Cross compiling for windows (64 bit)` in [main readme](/readme.md#cross-compiling-for-windows-64-bit)
    2) Downloading 64 bit ClassiCube from https://www.classicube.net/download/#dl-win
2) Install the `mingw-w64-tools` package (if it isn't already)
3) Generate the list of exported symbols from `ClassiCube.exe` by running:
    * `gendef ClassiCube.exe`
4) Create a linkable library from the exported symbols list by running: 
    * `x86_64-w64-mingw32-dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe`

TODO: also document alternate method of compiling the game using --out-implib

#### Compiling

`x86_64-w64-mingw32-gcc TestPlugin.c -o TestPlugin.dll -s -shared -L . -lClassiCube`

Then put `TestPlugin.dll` into your game's `plugins` folder. Done.

## Compiling - macOS

### Using gcc or clang

#### Compiling

`cc TestPlugin.c -o TestPlugin.dylib -undefined dynamic_lookup`

Then put `TestPlugin.dylib` into your game's `plugins` folder. Done.

## Compiling - Windows

### Using Visual Studio
TODO more detailed when I have some more time...

#### Setup

1) Compile the game, see `Compiling - Windows > using Visual Studio` in main readme
2) Find the `ClassiCube.lib` that was generated when compiling the game. Usually it is in either `src\x64\Debug` or `src\x86\Debug`.
3) Add a new `Empty Project` to the ClassiCube solution, then add the plugin .c files to it

Note: If the plugin provides a .vcxproj file, you can skip step 2 and just open that project file instead.

#### Configuration - alternative #1

The simplest way of linking to the `.lib` file is simply adding the following code to one of the plugin's `.c` files

```C
#ifdef _MSC_VER
  #ifdef _WIN64
    #pragma comment(lib, "[GAME SRC FOLDER]/x64/Debug/ClassiCube.lib")
  #else
    #pragma comment(lib, "[GAME SRC FOLDER]/x86/Debug/ClassiCube.lib")
  #endif
#endif
```

replacing `[GAME SRC FOLDER]` with the full path of `src` folder (e.g. `C:/Dev/ClassiCube/src`)

#### Configuration - alternative #2

The more complicated way of linking to the `.lib` file is to add it to the plugin's project configuration file

Right click the plugin project in the `Solution Explorer` pane, then click `Properties`

TODO: add screenshots here

TODO: may need to configure include directories

1) In `Configuration properties` -> `General`, make sure `Configuration type` is set to `Dynamic library (.DLL)`

2) In `Configuration properties` -> `Linker` -> `Input`, click the dropdown button for `Additional Dependencies`, then click `Edit`. Add the full path to `ClassiCube.lib`, then click `OK`

#### Compiling

Build the project. There should be a line in the build output that tells you where you can find the .dll file like this:
`
Project1.vcxproj -> C:\classicube-dev\testplugin\src\x64\Debug\TestPlugin.dll
` 

Then put `TestPlugin.dll` into your game's `plugins` folder. Done.

### Using mingw-w64

#### Setup

1) Create `ClassiCube.exe` by either:
    1) Compiling the game, see `Compiling for windows (MinGW-w64)` in [main readme](/readme.md#using-mingw-w64)
    2) Downloading ClassiCube from https://www.classicube.net/download/#dl-win
2) Generate the list of exported symbols in `ClassiCube.exe` by running:
    * `gendef ClassiCube.exe`
3) Create a linkable library from the exported symbols list by running: 
    * `dlltool -d ClassiCube.def -l libClassiCube.a -D ClassiCube.exe`

#### Compiling

`gcc TestPlugin.c -o TestPlugin.dll -s -shared -L . -lClassiCube`

Then put `TestPlugin.dll` into your game's `plugins` folder. Done.

## Notes for compiling for Windows

### Ensuring your plugin works when the ClassiCube exe isn't named ClassiCube.exe

If you follow the prior compilation instructions, the compiled DLL will have a runtime dependancy on `ClassiCube.exe`

However, this means that if the executable is e.g. named `ClassiCube (2).exe` instead. the plugin DLL will fail to load

To avoid this problem, you must 
1) Stop linking to `ClassiCube` (e.g. for `MinGW`, remove the ` -L . -lClassiCube`)
2) Load all functions and variables exported from ClassiCube via `GetProcAddress` instead

This is somewhat tedious to do - see [here](https://github.com/UnknownShadow200/ClassiCube-Plugins/) for some examples of plugins which do this

#### Compiling ultra small plugin DLLs - MinGW
If you **ONLY** use code from the game (no external libraries and no C standard library functions):
* You can add `-nostartfiles -Wl,--entry=0` to the compile flags to reduce the DLL size (e.g from 11 to 4 kb)

This isn't necessary to do though, and plugin DLLs work completely fine without doing this.