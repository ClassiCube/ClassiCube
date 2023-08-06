## Introduction

ClassiCube uses a custom string type rather than the standard C `char*` string in most places

ClassiCube strings (`cc_string`) are a struct with the following fields:
- `buffer` -> Pointer to 8 bit characters (unsigned code page 437 indices)
- `length` -> Number of characters currently used
- `capacity` -> Maximum number of characters (i.e buffer size)

Note: This means **STRINGS MAY NOT BE NULL TERMINATED** (and are not in most cases)

You should also read the **Strings** section in the [style guide](/doc/style.md)

## Memory management
Some general guidelines to keep in mind when it comes to `cc_string` strings:
- String buffers can be allocated on either the stack or heap<br>
(i.e. make sure you don't return strings that are using stack allocated buffers)
- Strings are fixed capacity (strings do not grow when length reaches capcity)<br>
(i.e. make sure you allocate a large enough buffer upfront)
- Strings are not garbage collected or reference counted<br>
(i.e. you are responsible for managing the lifetime of strings)

## C String conversion

### C string -> cc_string

Creating a `cc_string` string from a C string is straightforward:

#### From a constant C string
```C
void Example(void) {
    cc_string str = String_FromConst("test");
}
```

#### From a C string
```C
void Example(const char* c_str) {
    cc_string str = String_FromReadonly(c_str);
}
```
Note: `String_FromReadonly` can also be used with constant C strings, it's just a bit slower

#### From a C fixed size string
```C
struct Something { int value; char name[50]; };

void Example(struct Something* some) {
    cc_string str = String_FromRawArray(some->name);
}
```

### cc_string -> C string

The `buffer` field **should not** be treated as a C string, because `cc_string` strings **MAY NOT BE NULL TERMINATED** 

The general way to achieve this is to
1. Initialise `capacity` with 1 less than actual buffer size (e.g. use `String_InitArray_NT` instead of `String_InitArray`)
2. Perform various operations on the `cc_string` string
3. Add null terminator to end (i.e. `buffer[length]` = '\0';
4. Use `buffer` as a C string now

For example:
```C
void PrintInt(int value) {
    cc_string str; char strBuffer[128];
    String_InitArray_NT(str, strBuffer);
    String_AppendInt(&str, value);
    str.buffer[str.length] = '\0';
    puts(str.buffer);
}
```

## OS String conversion

`cc_string` strings cannot be directly used as arguments for operating system functions and must be converted first.

The following functions are provided to convert `cc_string` strings into operating system specific encoded strings:

### cc_string -> Windows string

`Platform_EncodeString` converts a `cc_string` into a null terminated `WCHAR` and `CHAR` string

#### Example
```C
void SetWorkingDir(cc_string* title) {
    cc_winstring str;
    Platform_EncodeUtf16(&str, title);
    SetCurrentDirectoryW(str.uni);
	
	// it's recommended that you DON'T use the ansi format whenever possible
    //SetCurrentDirectoryA(str.ansi); 
}
```

### cc_string -> UTF8 string

`String_EncodeUtf8` converts a `cc_string` into a null terminated UTF8-encoded `char*` string

#### Example
```C
void SetWorkingDir(cc_string* title) {
    char buffer[NATIVE_STR_LEN];
    String_EncodeUtf8(buffer, title);
    chdir(buffer);
}
```

## API

I'm lazy so I will just link to [String.h](/src/String.h)

If you'd rather I provided a more detailed reference here, please let me know.

# Extra details

## C comparison

A rough mapping of C string API to ClassiCube's string API:
```
atof    -> Convert_ParseFloat
strtof  -> Convert_ParseFloat
atoi    -> Convert_ParseInt
strtoi  -> Convert_ParseInt

strcat  -> String_AppendConst/String_AppendString
strcpy  -> String_Copy
strtok  -> String_UNSAFE_Split

strlen  -> str.length
strcmp  -> String_Equals/String_Compare
strchr  -> String_IndexOf
strrchr -> String_LastIndexOf
strstr  -> String_IndexOfConst

sprintf -> String_Format1/2/3/4
    %d  -> %i
  %04d -> %p4
    %i  -> %i
    %c  -> %r
  %.4f  -> %f4
    %s  -> %s (cc_string)
    %s  -> %c (char*)
    %x  -> %h
```

## C# comparison

A rough mapping of C# string API to ClassiCube's string API:
```
byte.Parse   -> Convert_ParseUInt8
ushort.Parse -> Convert_ParseUInt16
float.Parse  -> Convert_ParseFloat
int.Parse    -> Convert_ParseInt
ulong.Parse  -> Convert_ParseUInt64
bool.Parse   -> Convert_ParseBool

a += "X";     -> String_AppendString
b = a;        -> String_Copy
string.Insert -> String_InsertAt
string.Remove -> String_DeleteAt

string.Substring -> String_UNSAFE_Substring/String_UNSAFE_SubstringAt
string.Split     -> String_UNSAFE_Split/String_UNSAFE_SplitBy
string.TrimStart -> String_UNSAFE_TrimStart
string.TrimEnd   -> String_UNSAFE_TrimEnd

a.Length           -> str.length
a == b             -> String_Equals
string.Equals      -> String_CaslessEquals (StringComparison.OrdinalIgnoreCase)
string.IndexOf     -> String_IndexOf/String_IndexOfConst
string.LastIndexOf -> String_LastIndexOf
string.StartsWith  -> String_CaselessStarts (StringComparison.OrdinalIgnoreCase)
string.EndsWith    -> String_CaselessEnds   (StringComparison.OrdinalIgnoreCase)
string.CompareTo   -> String_Compare

string.Format -> String_Format1/2/3/4
```
*Note: I modelled cc_string after C# strings, hence the similar function names*

## C++ comparison

A rough mapping of C++ std::string API to ClassiCube's string API:
```
std::stof  -> Convert_ParseFloat
std::stoi  -> Convert_ParseInt
std::stoul -> Convert_ParseUInt64

string::append -> String_AppendString/String_AppendConst
b = a;         -> String_Copy
string::insert -> String_InsertAt
string::erase  -> String_DeleteAt

string::substr -> String_UNSAFE_Substring/String_UNSAFE_SubstringAt

string::length  -> str.length
a == b          -> String_Equals
string::find    -> String_IndexOf/String_IndexOfConst
string::rfind   -> String_LastIndexOf
string::compare -> String_Compare

std::sprintf -> String_Format1/2/3/4
```


## lifetime examples

Stack allocated returning example

Mem_Alloc/Mem_Free and function example

UNSAFE and mutating characters example