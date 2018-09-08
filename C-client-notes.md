### Compiling on linux

Install appropriate libs as required. Build steps are still WIP, but current way I'm using is:

Cross compiling for windows:

```i586-mingw32msvc-gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -lopengl32```

```i586-mingw32msvc-gcc *.c -o ClassiCube.exe -mwindows -lws2_32 -lwininet -lwinmm -ld3d9```

Compiling for linux: 

```gcc *.c -o Classicube -lX11 -lpthread -lGL -lm -lcurl -lopenal```

### Platform
Although the majority of the code is designed to be platform-independent, some per-platform functionality is required.

Some of the per-platform functionality you'll be required to implement is:
- 3D graphics API (shader support is not required)
- File I/O, Directory I/O, Socket I/O
- Error handling (including unhandled errors/exceptions)
- Application window, keyboard state, mouse state
- Text drawing, fonts
- Threading, signalable wait, mutex
- Stopwatch timer, current system time
- Http and https, getting command line args
- Memory allocation, copying, setting

### Types
* Integers and real numbers are always of a fixed size, see ```Typedefs.h```
* There are a few typedefs of integer types to provide better context in some functions
* structs are rarely typedef-ed
* ```bool``` is an alias for 8 bit unsigned integer
* ```GfxResourceID``` is not constant type - can be pointer or integer, depending on underlying 3D graphics API
* ```PackedCol``` field order differs depending on the underlying 3D graphics API

As such, your compiler is required to support:
- explicit 8/16/32/64 bit signed and unsigned integers
- 32 and 64 bit floating point numbers

I may not have defined the appropriate types for your compiler, so you may need to modify ```Typedefs.h```

### Strings
Strings are one of the most troublesome aspects of C. In this software, strings consist of:
- Pointer to 8 bit characters (unsigned code page 437 indices)
- Number of characters currently used (length)
- Maximum number of characters / buffer size (capacity)

Although this makes substrings / concatenating very fast, it also means 
**STRINGS ARE NOT NULL TERMINATED** (and are not in most cases).

Thus, when using or implementing a per-platform API, you must null-terminate and convert characters to native encoding

*Note: Several functions will take raw ```char*``` for performance, but this is not encouraged*

#### String arguments
A attribute macro is applied to string parameters and/or return types in functions. The three types are:
- ```STRING_PURE``` - String is not modified, no reference is kept (default)
- ```STRING_TRANSIENT``` - Characters in string may be modified, no reference is kept
- ```STRING_REF``` - Characters in string may be modified, **reference is kept to characters**

To make it extra clear, functions with ```STRING_REF``` arguments usually also have ```_UNSAFE_``` as part of their name.

For example, consider the function ```STRING_REF String StringsBuffer_UNSAFE_Get(buffer, index)```. 
The returned string has characters pointing to within *buffer*, so modifying them also changes the contents of *buffer*.

In general, use of ```STRING_PURE``` or ```STRING_TRANSIENT``` is preferred when possible.

#### String formatting
An API is provided for formatting strings similiar to printf in C or String.Format in C#.
The functions for formatting strings are in String.h:
```
void String_Format1(str, format, a1);
void String_Format2(str, format, a1, a2);
void String_Format3(str, format, a1, a2, a3);
void String_Format4(str, format, a1, a2, a3, a4);
```
#### Formatting specifiers
| Specifier | Meaning | Example |
| ------------- |-------------| -----|
| ```%b```      | UInt8 | ```%b``` of ```46``` = ```46``` |
| ```%i```      | Int32 | ```%i``` of ```-5``` = ```-5``` |
| ```%f[0-9]``` | Real32 |  ```%f2``` of ```321.3519``` = ```321.35``` |
| ```%p[0-9]``` | Int32, padded | ```%p3``` of ```5``` = ```005``` |
| ```%t```      | Boolean | ```%t``` of ```1``` = ```true``` |
| ```%c```      | UInt8* | ```%c``` of ```"ABCD"``` = ```ABCD``` |
| ```%s```      | String |  ```%s``` of ```{"ABCD", 2, 4}``` = ```AB``` |
| ```%r```      | UInt8, raw | ```%r``` of ```47``` = ```\``` |
| ```%x```      | UInt64, hex | ```%x``` of ```31``` = ```2F``` |
| ```%y```      | UInt32, hex | ```%y``` of ```11``` = ```B``` |
