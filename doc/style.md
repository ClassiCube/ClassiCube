### Guidelines
* Code should be C89 compatible and compilable as C++.
* Each .c file should represent a module. (see architecture.md for more details)
* Public functions and variables should be prefixed by module to avoid name collisions. (e.g. `Game_Reset`)
* Private functions should be named using pascal case. Prefixing module is optional - do it when it makes sense.
* Private variables don't really have a consistent style.

### Types
* Explicit integer typedefs are provided in ```Core.h``` for when needed. Otherwise just use int.
* A few common simple structs are typedef-ed, but are rarely otherwise.
* ```cc_bool``` is an alias for 8 bit unsigned integer
* ```PackedCol``` field order differs depending on the underlying 3D graphics API

I may not have defined the appropriate types for your compiler, so you may need to modify ```Core.h```

### Strings

A custom string type (`cc_string`) is used rather than `char*` strings in most places (see [strings](doc/strings.md) page for more details)

*Note: Several functions will take raw ```char*``` for performance, but this is not encouraged*

#### String arguments
String arguments are annotated to indicate storage and readonly requirements. These are:
- ```const cc_string*``` - String is not modified at all
- ```cc_string*``` - Characters in string may be modified
- ```STRING_REF``` - Macro annotation indicating a **reference is kept to the characters**

To make it extra clear, functions with ```STRING_REF``` arguments usually also have ```_UNSAFE_``` as part of their name.

For example, consider the function ```cc_string Substring_UNSAFE(STRING_REF const cc_string* str, length)```

The *input string* is not modified at all. However, the characters of the *returned string* points to the characters of the *input string*, so modifying the characters in the *input string* also modifies the *returned string*.

In general, use of ```const String*``` is preferred when possible, and ```STRING_REF``` as little as possible.
