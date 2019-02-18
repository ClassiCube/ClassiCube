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
* Integers generally use plain ```int```, floating point numbers use ```float``` or ```double```. 
* Explicit integer types are also provided in ```Core.h```.
* There are a few typedefs of integer types to provide better context in some functions
* A few common simple structs are typedef-ed, but are rarely otherwise.
* ```bool``` is an alias for 8 bit unsigned integer
* ```GfxResourceID``` is not constant type - can be pointer or integer, depending on underlying 3D graphics API
* ```PackedCol``` field order differs depending on the underlying 3D graphics API

As such, your compiler is required to support:
- explicit 8/16/32/64 bit signed and unsigned integers
- 32 and 64 bit floating point numbers

I may not have defined the appropriate types for your compiler, so you may need to modify ```Core.h```

### Strings
Strings are one of the most troublesome aspects of C. In this software, strings consist of:
- Pointer to 8 bit characters (unsigned code page 437 indices)
- Number of characters currently used (length)
- Maximum number of characters / buffer size (capacity)

Although this makes substrings / concatenating very fast, it also means 
**STRINGS ARE NOT NULL TERMINATED** (and are not in most cases).

Thus, when using or implementing a per-platform API, you must null-terminate and convert characters to native encoding. You should implement the ```Platform_ConvertString``` function and use that.

*Note: Several functions will take raw ```char*``` for performance, but this is not encouraged*

#### String arguments
String arguments are annotated to indicate storage and readonly requirements. These are:
- ```const String*``` - String is not modified at all
- ```String*``` - Characters in string may be modified
- ```STRING_REF``` - Macro annotation indicating a **reference is kept to characters**

To make it extra clear, functions with ```STRING_REF``` arguments usually also have ```_UNSAFE_``` as part of their name.

For example, consider the function ```String Substring_UNSAFE(STRING_REF const String* str, length)```

The *input string* is not modified at all. However, the characters of the *returned string* points to the characters of the *input string*, so modifying the characters in the *input string* also modifies the *returned string*.

In general, use of ```const String*``` is preferred when possible, and ```STRING_REF``` as little as possible.

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
| Specifier | Type | Example |
| ------------- |-------------| -----|
| ```%b```      | uint8_t | ```%b``` of ```46``` = ```46``` |
| ```%i```      | int | ```%i``` of ```-5``` = ```-5``` |
| ```%f[0-9]``` | float |  ```%f2``` of ```321.3519``` = ```321.35``` |
| ```%p[0-9]``` | int | ```%p3``` of ```5``` = ```005``` |
| ```%t```      | Boolean | ```%t``` of ```1``` = ```true``` |
| ```%c```      | char* | ```%c``` of ```"ABCD"``` = ```ABCD``` |
| ```%s```      | String |  ```%s``` of ```{"ABCD", 2, 4}``` = ```AB``` |
| ```%r```      | char | ```%r``` of ```47``` = ```\``` |
| ```%x```      | uintptr_t | ```%x``` of ```31``` = ```2F``` |
| ```%h```      | uint32_t | ```%h``` of ```11``` = ```B``` | 
