#ifndef CS_STRING_H
#define CS_STRING_H
#include "Typedefs.h"
#include "Compiler.h"
/* Implements operations for a string.
   Also implements conversions betweens strings and numbers.
   Also implements converting code page 437 indices to/from unicode.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define String_BufferSize(n) (n + 1)
#define STRING_INT32CHARS 20

typedef struct String_ {
	/* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */
	UInt8* buffer;
	/* Number of characters used. */
	UInt16 length;
	/* Max number of characters that can be in buffer. */
	UInt16 capacity;
} String;

/* Constructs a new string, pointing a buffer consisting purely of NULL characters. */
String String_FromEmptyBuffer(UInt8* buffer, UInt16 capacity);
/* Constructs a new string, pointing a buffer consisting of arbitary data.
NOTE: This method sets the bytes occupied by the string to NUL. */
String String_FromRawBuffer(UInt8* buffer, UInt16 capacity);
/* Constructs a new string from a constant readonly buffer. */
String String_FromReadonly(const UInt8* buffer);
/* Makes an empty string that points to nowhere. */
String String_MakeNull(void);
/* Constructs a new string from a compile time string constant. */
#define String_FromConstant(text) { text, (UInt16)(sizeof(text) - 1), (UInt16)(sizeof(text) - 1)};

/* Sets all characters in the given string to lowercase. */
void String_MakeLowercase(STRING_TRANSIENT String* str);
/* Sets all characters in the given string to NULL, then sets length t0 0. */
void String_Clear(STRING_TRANSIENT String* str);
/* Returns a string that points directly to a substring of the given string.
NOTE: THIS IS UNSAFE - IT MAINTAINS A REFERENCE TO THE ORIGINAL BUFFER, AND THE SUBSTRING IS NOT NULL TERMINATED */
String String_UNSAFE_Substring(STRING_REF String* str, Int32 offset, Int32 length);
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))

/* Returns whether two strings have same contents. */
bool String_Equals(STRING_TRANSIENT String* a, STRING_TRANSIENT String* b);
/* Returns whether two strings have same case-insensitive contents. */
bool String_CaselessEquals(STRING_TRANSIENT String* a, STRING_TRANSIENT String* b);

/* Attempts to append a character to the end of a string. */
bool String_Append(STRING_TRANSIENT String* str, UInt8 c);
/* Attempts to append an integer value to the end of a string. */
bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num);
/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt32(STRING_TRANSIENT String* str, Int32 num, Int32 minDigits);
/* Attempts to append a constant raw null-terminated string. */
bool String_AppendConstant(STRING_TRANSIENT String* str, const UInt8* buffer);
/* Attempts to append a string. */
bool String_AppendString(STRING_TRANSIENT String* str, String* buffer);

/* Finds the first index of c in given string, -1 if not found. */
Int32 String_IndexOf(STRING_TRANSIENT String* str, UInt8 c, Int32 offset);
/* Finds the last index of c in given string, -1 if not found. */
Int32 String_LastIndexOf(STRING_TRANSIENT String* str, UInt8 c);
/* Gets the character at the given index in the string. */
UInt8 String_CharAt(STRING_TRANSIENT String* str, Int32 offset);

/* Find the first index of sub in given string, -1 if not found. */
Int32 String_IndexOfString(STRING_TRANSIENT String* str, STRING_TRANSIENT String* sub);
/* Returns whether sub is contained within string. */
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
/* Returns whether given string starts with sub. */
#define String_StartsWith(str, sub) (String_IndexOfString(str, sub) == 0)

/* Converts a code page 437 index into a unicode character. */
UInt16 Convert_CP437ToUnicode(UInt8 c);
/* Converts a unicode character into a code page 437 index. */
UInt8 Convert_UnicodeToCP437(UInt16 c);

/* Attempts to parse the given string as a signed 32 bit integer.*/
bool Convert_TryParseInt32(STRING_TRANSIENT String* str, Int32* value);
/* Attempts to parse the given string as an unsigned 8 bit integer.*/
bool Convert_TryParseUInt8(STRING_TRANSIENT String* str, UInt8* value);
/* Attempts to parse the given string as an unsigned 8 bit integer.*/
bool Convert_TryParseUInt16(STRING_TRANSIENT String* str, UInt16* value);
/* Attempts to parse the given string as a 32 bit floating point number. */
bool Convert_TryParseReal32(STRING_TRANSIENT String* str, Real32* value);
/* Attempts to parse the given string as a boolean. */
bool Convert_TryParseBool(STRING_TRANSIENT String* str, bool* value);
#endif