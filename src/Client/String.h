#ifndef CC_STRING_H
#define CC_STRING_H
#include "Typedefs.h"
#include "Compiler.h"
/* Implements operations for a string.
   Also implements conversions betweens strings and numbers.
   Also implements converting code page 437 indices to/from unicode.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define String_BufferSize(n) (n + 1)
#define STRING_INT32CHARS 20

/* Indicates that a string argument is discarded after the function has completed, and is not modified. */
#define STRING_PURE
/* Indicates that a string argument is discarded after the function has completed, but **MAY BE MODIFIED**. */
#define STRING_TRANSIENT
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF

typedef struct String_ {	
	UInt8* buffer;   /* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */	
	UInt16 length;   /* Number of characters used. */
	UInt16 capacity; /* Max number of characters that can be in buffer. */
} String;

/* Constructs a new string, pointing a buffer consisting purely of NULL characters. */
String String_FromEmptyBuffer(STRING_REF UInt8* buffer, UInt16 capacity);
/* Constructs a new string, pointing a buffer consisting of arbitary data.
NOTE: This method sets the bytes occupied by the string to NULL. */
String String_FromRawBuffer(STRING_REF UInt8* buffer, UInt16 capacity);
/* Constructs a new string from a constant readonly buffer. */
String String_FromReadonly(STRING_REF const UInt8* buffer);
/* Makes an empty string that points to nowhere. */
String String_MakeNull(void);
/* Constructs a new string from a compile time string constant. */
#define String_FromConst(text) { text, (UInt16)(sizeof(text) - 1), (UInt16)(sizeof(text) - 1)};
/* Constructs a new string from a compile time empty string buffer. */
#define String_EmptyConstArray(buffer) { buffer, 0, (UInt16)(sizeof(buffer) - 1)};

/* Sets all characters in the given string to lowercase. */
void String_MakeLowercase(STRING_TRANSIENT String* str);
/* Sets all characters in the given string to NULL, then sets length to 0. */
void String_Clear(STRING_TRANSIENT String* str);
/* Returns a string that points directly to a substring of the given string.
NOTE: THIS IS UNSAFE - IT MAINTAINS A REFERENCE TO THE ORIGINAL BUFFER, AND THE SUBSTRING IS NOT NULL TERMINATED */
String String_UNSAFE_Substring(STRING_REF String* str, Int32 offset, Int32 length);
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))

bool String_Equals(STRING_PURE String* a, STRING_PURE String* b);
bool String_CaselessEquals(STRING_PURE String* a, STRING_PURE String* b);

/* Attempts to append a character to the end of a string. */
bool String_Append(STRING_TRANSIENT String* str, UInt8 c);
/* Attempts to append an integer value to the end of a string. */
bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num);
/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt32(STRING_TRANSIENT String* str, Int32 num, Int32 minDigits);
/* Attempts to append a constant raw null-terminated string. */
bool String_AppendConst(STRING_TRANSIENT String* str, const UInt8* toAppend);
/* Attempts to append a string. */
bool String_AppendString(STRING_TRANSIENT String* str, STRING_PURE String* toAppend);

/* Finds the first index of c in given string, -1 if not found. */
Int32 String_IndexOf(STRING_PURE String* str, UInt8 c, Int32 offset);
/* Finds the last index of c in given string, -1 if not found. */
Int32 String_LastIndexOf(STRING_PURE String* str, UInt8 c);
/* Gets the character at the given index in the string. */
UInt8 String_CharAt(STRING_PURE String* str, Int32 offset);
/* Inserts a character at the given index in the string. */
void String_InsertAt(STRING_TRANSIENT String* str, Int32 offset, UInt8 c);
/* Deletes a character at the given index in the string. */
void String_DeleteAt(STRING_TRANSIENT String* str, Int32 offset);

/* Find the first index of sub in given string, -1 if not found. */
Int32 String_IndexOfString(STRING_PURE String* str, STRING_PURE String* sub);
/* Returns whether sub is contained within string. */
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
/* Returns whether given string starts with sub. */
#define String_StartsWith(str, sub) (String_IndexOfString(str, sub) == 0)

UInt16 Convert_CP437ToUnicode(UInt8 c);
UInt8 Convert_UnicodeToCP437(UInt16 c);
bool Convert_TryParseInt32(STRING_PURE String* str, Int32* value);
bool Convert_TryParseUInt8(STRING_PURE String* str, UInt8* value);
bool Convert_TryParseUInt16(STRING_PURE String* str, UInt16* value);
bool Convert_TryParseReal32(STRING_PURE String* str, Real32* value);
bool Convert_TryParseBool(STRING_PURE String* str, bool* value);

/* todo use a single byte array for all strings, each 'string' is 22 bits offsrt, 10 bits length into this array. */
/* means resizing is expensive tho*/
typedef struct StringsBuffer_ {
	UInt8* TextBuffer;
	UInt32 TextBufferSize;
	UInt32* FlagsBuffer;
	UInt32 FlagsBufferSize;
	UInt32 Count;
} StringsBuffer;

void StringBuffers_Init(StringsBuffer* buffer);
void StringsBuffer_Free(StringsBuffer* buffer);
void StringsBuffer_Get(StringsBuffer* buffer, UInt32 index, STRING_TRANSIENT String* text);
STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, UInt32 index);
void StringsBuffer_Add(StringsBuffer* buffer, STRING_PURE String* text);
void StringsBuffer_Remove(StringsBuffer* buffer, UInt32 index);
#endif