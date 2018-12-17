#ifndef CC_STRING_H
#define CC_STRING_H
#include "Core.h"
/* Implements operations for a string.
   Also implements conversions betweens strings and numbers.
   Also implements converting code page 437 indices to/from unicode.
   Also implements wrapping a single line of text into multiple lines.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

#define STRING_INT_CHARS 24
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF
/* Converts a character from A-Z to a-z, otherwise is left untouched. */
#define Char_MakeLower(c) if ((c) >= 'A' && (c) <= 'Z') { (c) += ' '; }

typedef struct String_ {	
	char* buffer;      /* Pointer to characters, NOT NULL TERMINATED */
	uint16_t length;   /* Number of characters used */
	uint16_t capacity; /* Max number of characters  */
} String;

/* Counts number of characters until a '\0' is found. */
int String_CalcLen(const char* raw, int capacity);
/* Constant string that points to NULL and has 0 length. */
/* NOTE: Do NOT modify the contents of this string! */
extern const String String_Empty;
/* Constructs a string from the given arguments. */
String String_Init(STRING_REF char* buffer, int length, int capacity);
/* Constructs a string from the given arguments, then sets all characters to '\0'. */
String String_InitAndClear(STRING_REF char* buffer, int capacity);
/* Constructs a string from a (maybe null terminated) buffer. */
CC_NOINLINE String String_FromRaw(STRING_REF char* buffer, int capacity);
/* Constructs a string from a null-terminated constant readonly buffer. */
CC_NOINLINE String String_FromReadonly(STRING_REF const char* buffer);

/* Constructs a string from a compile time array, then sets all characters to '\0'. */
#define String_ClearedArray(buffer) String_InitAndClear(buffer, sizeof(buffer))
/* Constructs a string from a compile time string constant */
#define String_FromConst(text) { text, (sizeof(text) - 1), (sizeof(text) - 1)}
/* Constructs a string from a compile time array */
#define String_FromArray(buffer) { buffer, 0, sizeof(buffer)}
/* Constructs a string from a compile time array, that may have arbitary actual length of data at runtime */
#define String_FromRawArray(buffer) String_FromRaw(buffer, sizeof(buffer))
/* Constructs a string from a compile time array (leaving 1 byte of room for null terminator) */
#define String_NT_Array(buffer) { buffer, 0, (sizeof(buffer) - 1)}
/* Initialises a string from a compile time array. */
#define String_InitArray(str, buffr) str.buffer = buffr; str.length = 0; str.capacity = sizeof(buffr);
/* Initialises a string from a compile time array. (leaving 1 byte of room for null terminator) */
#define String_InitArray_NT(str, buffr) str.buffer = buffr; str.length = 0; str.capacity = sizeof(buffr) - 1;

/* Removes all colour codes from the given string. */
CC_NOINLINE void String_StripCols(String* str);
/* Sets length of dst to 0, then appends all characters in src. */
CC_NOINLINE void String_Copy(String* dst, const String* src);
/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
CC_NOINLINE String String_UNSAFE_Substring(STRING_REF const String* str, int offset, int length);
/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))
/* UNSAFE: Splits a string of the form [str1][c][str2][c][str3].. into substrings. */
/* e.g., "abc:id:xyz" becomes "abc","id","xyz" */
CC_NOINLINE int String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int maxSubs);
/* UNSAFE: Splits a string of the form [key][c][value] into two substrings. */
/* e.g., "allowed =true" becomes "allowed" and "true", and excludes the space. */
/* If c is not found, sets key to str and value to String_Empty, returns false. */
CC_NOINLINE bool String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value);

/* Whether all characters of the strings are equal. */
CC_NOINLINE bool String_Equals(const String* a, const String* b);
/* Whether all characters of the strings are case-insensitively equal. */
CC_NOINLINE bool String_CaselessEquals(const String* a, const String* b);
/* Whether all characters of the strings are case-insensitively equal. */
/* NOTE: Faster than String_CaselessEquals(a, String_FromReadonly(b)) */
CC_NOINLINE bool String_CaselessEqualsConst(const String* a, const char* b);
/* Breaks down an integer into an array of digits. */
/* NOTE: Digits are in reverse order, so e.g. '200' becomes '0','0','2' */
CC_NOINLINE int  String_MakeUInt32(uint32_t num, char* digits);

/* Attempts to append a character. */
/* Does nothing if str->length == str->capcity. */
void String_Append(String* str, char c);
/* Attempts to append "true" if value is non-zero, "false" otherwise. */
CC_NOINLINE void String_AppendBool(String* str, bool value);
/* Attempts to append the digits of an integer (and -sign if negative). */
CC_NOINLINE void String_AppendInt(String* str, int num);
/* Attempts to append the digits of an unsigned 32 bit integer. */
CC_NOINLINE void String_AppendUInt32(String* str, uint32_t num);
/* Attempts to append the digits of an integer, padding left with 0. */
CC_NOINLINE void String_AppendPaddedInt(String* str, int num, int minDigits);
/* Attempts to append the digits of an unsigned 64 bit integer. */
CC_NOINLINE void String_AppendUInt64(String* str, uint64_t num);

/* Attempts to append the digits of a float as a decimal. */
/* NOTE: If the number is an integer, no decimal point is added. */
/* Otherwise, fracDigits digits are added after a decimal point. */
/*  e.g. 1.0f produces "1", 2.6745f produces "2.67" when fracDigits is 2 */
CC_NOINLINE void String_AppendFloat(String* str, float num, int fracDigits); /* TODO: Need to account for , or . for decimal */
/* Attempts to append characters. src MUST be null-terminated. */
CC_NOINLINE void String_AppendConst(String* str, const char* src);
/* Attempts to append characters of a string. */
CC_NOINLINE void String_AppendString(String* str, const String* src);
/* Attempts to append characters of a string, skipping any colour codes. */
CC_NOINLINE void String_AppendColorless(String* str, const String* src);
/* Attempts to append the two hex digits of a byte. */
CC_NOINLINE void String_AppendHex(String* str, uint8_t value);

/* Returns first index of the given character in the given string, -1 if not found. */
CC_NOINLINE int String_IndexOf(const String* str, char c, int offset);
/* Returns last index of the given character in the given string, -1 if not found. */
CC_NOINLINE int String_LastIndexOf(const String* str, char c);
/* Inserts the given character into the given string. Exits process if this fails. */
/* e.g. inserting 'd' at offset '1' into "abc" produces "adbc" */
CC_NOINLINE void String_InsertAt(String* str, int offset, char c);
/* Deletes a character from the given string. Exits process if this fails. */
/* e.g. deleting at offset '1' from "adbc" produces "abc" */
CC_NOINLINE void String_DeleteAt(String* str, int offset);
/* Trims leading spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
CC_NOINLINE void String_UNSAFE_TrimStart(String* str);
/* Trims trailing spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
CC_NOINLINE void String_UNSAFE_TrimEnd(String* str);

/* Returns first index of the given substring in the given string, -1 if not found. */
/* e.g. index of "ab" within "cbabd" is 2 */
CC_NOINLINE int String_IndexOfString(const String* str, const String* sub);
/* Returns whether given substring is inside the given string. */
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
/* Returns whether given substring is case-insensitively inside the given string. */
CC_NOINLINE bool String_CaselessContains(const String* str, const String* sub);
/* Returns whether given substring is case-insensitively equal to the beginning of the given string. */
CC_NOINLINE bool String_CaselessStarts(const String* str, const String* sub);
/* Returns whether given substring is case-insensitively equal to the ending of the given string. */
CC_NOINLINE bool String_CaselessEnds(const String* str, const String* sub);
/* Compares the length of the given strings, then compares the characters if same length. Returns: */
/* -X if a.length < b.length, X if a.length > b.length */
/* -X if a.buffer[i] < b.buffer[i], X if a.buffer[i] > b.buffer[i] */
/* else returns 0. NOTE: The return value is not just in -1,0,1! */
CC_NOINLINE int  String_Compare(const String* a, const String* b);

/* See String_Format4 */
void String_Format1(String* str, const char* format, const void* a1);
/* See String_Format4 */
void String_Format2(String* str, const char* format, const void* a1, const void* a2);
/* See String_Format4 */
void String_Format3(String* str, const char* format, const void* a1, const void* a2, const void* a3);
/* Formats the arguments in a string, similiar to printf or C# String.Format
NOTE: This is a low level API. Argument count and types are not checked at all. */
void String_Format4(String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Converts a code page 437 character to its unicode equivalent. */
Codepoint Convert_CP437ToUnicode(char c);
/* Converts a unicode character to its code page 437 equivalent, or '?' if no match. */
char Convert_UnicodeToCP437(Codepoint cp);
/* Attempts to convert a unicode character to its code page 437 equivalent. */
bool Convert_TryUnicodeToCP437(Codepoint cp, char* c);
/* Appends all characters from UTF8 encoded data to the given string. */
void String_DecodeUtf8(String* str, uint8_t* data, uint32_t len);
/* Decodes a unicode character from UTF8, returning number of bytes read. */
/* Returns 0 if not enough input data to read the character. */
int Convert_Utf8ToUnicode(Codepoint* cp, const uint8_t* data, uint32_t len);
/* Encodes a unicode character in UTF8, returning number of bytes written. */
int Convert_UnicodeToUtf8(Codepoint cp, uint8_t* data);

/* Attempts to convert the given string into an unsigned 8 bit integer. */
CC_NOINLINE bool Convert_ParseUInt8(const String*  str, uint8_t* value);
/* Attempts to convert the given string into an signed 16 bit integer. */
CC_NOINLINE bool Convert_ParseInt16(const String*  str, int16_t* value);
/* Attempts to convert the given string into an unsigned 16 bit integer. */
CC_NOINLINE bool Convert_ParseUInt16(const String* str, uint16_t* value);
/* Attempts to convert the given string into an integer. */
CC_NOINLINE bool Convert_ParseInt(const String*    str, int* value);
/* Attempts to convert the given string into an unsigned 64 bit integer. */
CC_NOINLINE bool Convert_ParseUInt64(const String* str, uint64_t* value);
/* Attempts to convert the given string into a floating point number. */
CC_NOINLINE bool Convert_ParseFloat(const String*  str, float* value);
/* Attempts to convert the given string into a bool. */
/* NOTE: String must case-insensitively equal "true" or "false" */
CC_NOINLINE bool Convert_ParseBool(const String*   str, bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
typedef struct StringsBuffer_ {
	char*      TextBuffer;  /* Raw characters of all entries */
	uint32_t*  FlagsBuffer; /* Private flags for each entry */
	int Count, TotalLength;
	/* internal state */
	int      _TextBufferSize, _FlagsBufferSize;
	char     _DefaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	uint32_t _DefaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

/* Resets counts to 0, and frees any allocated memory. */
CC_NOINLINE void StringsBuffer_Clear(StringsBuffer* buffer);
/* Copies the characters from the i'th string in the given buffer into the given string. */
CC_NOINLINE void StringsBuffer_Get(StringsBuffer* buffer, int i, String* str);
/* UNSAFE: Returns a direct pointer to the i'th string in the given buffer. */
CC_NOINLINE STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i);
/* Adds a given string to the end of the given buffer. */
CC_NOINLINE void StringsBuffer_Add(StringsBuffer* buffer, const String* str);
/* Removes the i'th string from the given buffer, shifting following strings downwards. */
CC_NOINLINE void StringsBuffer_Remove(StringsBuffer* buffer, int index);

/* Performs line wrapping on the given string. */
/* e.g. "some random tex|t* (| is lineLen) becomes "some random" "text" */
CC_NOINLINE void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen);
/* Calculates the position of a raw index in the wrapped lines. */
CC_NOINLINE void WordWrap_GetCoords(int index, const String* lines, int numLines, int* coordX, int* coordY);
/* Returns number of characters from current character to end of previous word. */
CC_NOINLINE int  WordWrap_GetBackLength(const String* text, int index);
/* Returns number of characters from current character to start of next word. */
CC_NOINLINE int  WordWrap_GetForwardLength(const String* text, int index);
#endif
