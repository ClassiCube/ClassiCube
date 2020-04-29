#ifndef CC_STRING_H
#define CC_STRING_H
#include "Core.h"
/* Implements operations for a string.
   Also implements conversions betweens strings and numbers.
   Also implements converting code page 437 indices to/from unicode.
   Also implements wrapping a single line of text into multiple lines.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

#define STRING_INT_CHARS 24
/* Indicates that a reference to the buffer in a string argument is persisted after the function has completed.
Thus it is **NOT SAFE** to allocate a string on the stack. */
#define STRING_REF
/* Converts a character from A-Z to a-z, otherwise is left untouched. */
#define Char_MakeLower(c) if ((c) >= 'A' && (c) <= 'Z') { (c) += ' '; }

typedef struct String_ {	
	char* buffer;      /* Pointer to characters, NOT NULL TERMINATED */
	cc_uint16 length;   /* Number of characters used */
	cc_uint16 capacity; /* Max number of characters  */
} String;

/* Constant string that points to NULL and has 0 length. */
/* NOTE: Do NOT modify the contents of this string! */
extern const String String_Empty;
/* Constructs a string from the given arguments. */
static CC_INLINE String String_Init(STRING_REF char* buffer, int length, int capacity) {
	String s; s.buffer = buffer; s.length = length; s.capacity = capacity; return s;
}

/* Counts number of characters until a '\0' is found. */
CC_API int String_CalcLen(const char* raw, int capacity);
/* Constructs a string from a (maybe null terminated) buffer. */
CC_NOINLINE String String_FromRaw(STRING_REF char* buffer, int capacity);
/* Constructs a string from a null-terminated constant readonly buffer. */
CC_NOINLINE String String_FromReadonly(STRING_REF const char* buffer);

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

/* Sets length of dst to 0, then appends all characters in src. */
CC_API void String_Copy(String* dst, const String* src);
/* Copies up to capacity characters from src into dst. Appends \0 after if src->length is < capacity. */
/* NOTE: This means the buffer MAY NOT be null-terminated. Only use with String_FromRawArray. */
CC_API void String_CopyToRaw(char* dst, int capacity, const String* src);
/* Calls String_CopyToRaw on a compile time array. */
#define String_CopyToRawArray(buffer, src) String_CopyToRaw(buffer, sizeof(buffer), src)

/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
CC_API String String_UNSAFE_Substring(STRING_REF const String* str, int offset, int length);
/* UNSAFE: Returns a substring of the given string. (sub.buffer is within str.buffer + str.length) */
/* The substring returned is { str.buffer + offset, str.length - offset } */
CC_API String String_UNSAFE_SubstringAt(STRING_REF const String* str, int offset);
/* UNSAFE: Splits a string of the form [str1][c][str2][c][str3].. into substrings. */
/* e.g., "abc:id:xyz" becomes "abc","id","xyz" */
/* Returns the number of substrings found. (always <= maxSubs) */
CC_API int String_UNSAFE_Split(STRING_REF const String* str, char c, String* subs, int maxSubs);
/* UNSAFE: Splits a string of the form [part][c][rest], returning whether [c] was found or not. */
/* NOTE: This is intended to be repeatedly called until str->length is 0. (unbounded String_UNSAFE_Split) */
CC_API void String_UNSAFE_SplitBy(STRING_REF String* str, char c, String* part);
/* UNSAFE: Splits a string of the form [key][c][value] into two substrings. */
/* e.g., "allowed =true" becomes "allowed" and "true", and excludes the space. */
/* If c is not found, sets key to str and value to String_Empty, returns false. */
/* Otherwise if c is found, a non-zero value is returned. */
CC_API int String_UNSAFE_Separate(STRING_REF const String* str, char c, String* key, String* value);

/* Returns non-zero if all characters of the strings are equal. */
CC_API int String_Equals(const String* a, const String* b);
/* Returns non-zero if all characters of the strings are case-insensitively equal. */
CC_API int String_CaselessEquals(const String* a, const String* b);
/* Returns non-zero if all characters of the strings are case-insensitively equal. */
/* NOTE: Faster than String_CaselessEquals(a, String_FromReadonly(b)) */
CC_API int String_CaselessEqualsConst(const String* a, const char* b);
/* Breaks down an integer into an array of digits, and returns number of digits. */
/* NOTE: Digits are in reverse order, so e.g. '200' becomes '0','0','2' */
int String_MakeUInt32(cc_uint32 num, char* digits);

/* Attempts to append a character. */
/* Does nothing if str->length == str->capcity. */
CC_API void String_Append(String* str, char c);
/* Attempts to append "true" if value is non-zero, "false" otherwise. */
CC_API void String_AppendBool(String* str, cc_bool value);
/* Attempts to append the digits of an integer (and -sign if negative). */
CC_API void String_AppendInt(String* str, int num);
/* Attempts to append the digits of an unsigned 32 bit integer. */
CC_API void String_AppendUInt32(String* str, cc_uint32 num);
/* Attempts to append the digits of an integer, padding left with 0. */
CC_API void String_AppendPaddedInt(String* str, int num, int minDigits);

/* Attempts to append the digits of a float as a decimal. */
/* NOTE: If the number is an integer, no decimal point is added. */
/* Otherwise, fracDigits digits are added after a decimal point. */
/*  e.g. 1.0f produces "1", 2.6745f produces "2.67" when fracDigits is 2 */
CC_API void String_AppendFloat(String* str, float num, int fracDigits); /* TODO: Need to account for , or . for decimal */
/* Attempts to append characters. src MUST be null-terminated. */
CC_API void String_AppendConst(String* str, const char* src);
/* Attempts to append characters of a string. */
CC_API void String_AppendString(String* str, const String* src);
/* Attempts to append characters of a string, skipping any colour codes. */
CC_API void String_AppendColorless(String* str, const String* src);
/* Attempts to append the two hex digits of a byte. */
CC_API void String_AppendHex(String* str, cc_uint8 value);

/* Returns first index of the given character in the given string, -1 if not found. */
#define String_IndexOf(str, c) String_IndexOfAt(str, 0, c)
/* Returns first index of the given character in the given string, -1 if not found. */
CC_API int String_IndexOfAt(const String* str, int offset, char c);
/* Returns last index of the given character in the given string, -1 if not found. */
#define String_LastIndexOf(str, c) String_LastIndexOfAt(str, 0, c)
/* Returns last index of the given character in the given string, -1 if not found. */
CC_API int String_LastIndexOfAt(const String* str, int offset, char c);
/* Inserts the given character into the given string. Exits process if this fails. */
/* e.g. inserting 'd' at offset '1' into "abc" produces "adbc" */
CC_API void String_InsertAt(String* str, int offset, char c);
/* Deletes a character from the given string. Exits process if this fails. */
/* e.g. deleting at offset '1' from "adbc" produces "abc" */
CC_API void String_DeleteAt(String* str, int offset);
/* Trims leading spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
CC_API void String_UNSAFE_TrimStart(String* str);
/* Trims trailing spaces from the given string. */
/* NOTE: Works by adjusting buffer/length, the characters in memory are not shifted. */
CC_API void String_UNSAFE_TrimEnd(String* str);

/* Returns first index of the given substring in the given string, -1 if not found. */
/* e.g. index of "ab" within "cbabd" is 2 */
CC_API int String_IndexOfConst(const String* str, const char* sub);
/* Returns non-zero if given substring is inside the given string. */
#define String_ContainsConst(str, sub) (String_IndexOfConst(str, sub) >= 0)
/* Returns non-zero if given substring is case-insensitively inside the given string. */
CC_API int String_CaselessContains(const String* str, const String* sub);
/* Returns non-zero if given substring is case-insensitively equal to the beginning of the given string. */
CC_API int String_CaselessStarts(const String* str, const String* sub);
/* Returns non-zero if given substring is case-insensitively equal to the ending of the given string. */
CC_API int String_CaselessEnds(const String* str, const String* sub);
/* Compares the length of the given strings, then compares the characters if same length. Returns: */
/* -X if a.length < b.length, X if a.length > b.length */
/* -X if a.buffer[i] < b.buffer[i], X if a.buffer[i] > b.buffer[i] */
/* else returns 0. NOTE: The return value is not just in -1,0,1! */
CC_API int String_Compare(const String* a, const String* b);

/* See String_Format4 */
CC_API void String_Format1(String* str, const char* format, const void* a1);
/* See String_Format4 */
CC_API void String_Format2(String* str, const char* format, const void* a1, const void* a2);
/* See String_Format4 */
CC_API void String_Format3(String* str, const char* format, const void* a1, const void* a2, const void* a3);
/* Formats the arguments in a string, similiar to printf or C# String.Format
NOTE: This is a low level API. Argument count and types are not checked at all. */
CC_API void String_Format4(String* str, const char* format, const void* a1, const void* a2, const void* a3, const void* a4);

/* Converts a code page 437 character to its unicode equivalent. */
Codepoint Convert_CP437ToUnicode(char c);
/* Converts a unicode character to its code page 437 equivalent, or '?' if no match. */
char Convert_UnicodeToCP437(Codepoint cp);
/* Attempts to convert a unicode character to its code page 437 equivalent. */
cc_bool Convert_TryUnicodeToCP437(Codepoint cp, char* c);
/* Decodes a unicode character from UTF8, returning number of bytes read. */
/* Returns 0 if not enough input data to read the character. */
int Convert_Utf8ToUnicode(Codepoint* cp, const cc_uint8* data, cc_uint32 len);
/* Encodes a unicode character in UTF8, returning number of bytes written. */
/* The number of bytes written is always either 1,2 or 3. */
int Convert_UnicodeToUtf8(Codepoint cp, cc_uint8* data);
/* Encodes a code page 437 character in UTF8, returning number of bytes written. */
/* The number of bytes written is always either 1,2 or 3. */
int Convert_CP437ToUtf8(char c, cc_uint8* data);

/* Attempts to append all characters from UTF16 encoded data to the given string. */
/* Characters not in code page 437 are omitted. */
void String_AppendUtf16(String* str, const Codepoint* chars, int numBytes);
/* Attempts to append all characters from UTF8 encoded data to the given string. */
/* Characters not in code page 437 are omitted. */
void String_AppendUtf8(String* str, const cc_uint8* chars, int numBytes);
/* Attempts to append all characters from CP-1252 encoded data to the given string. */
/* Characters not in code page 437 are omitted. */
void String_DecodeCP1252(String* str, const cc_uint8* chars, int numBytes);

/* Attempts to convert the given string into an unsigned 8 bit integer. */
CC_API cc_bool Convert_ParseUInt8(const String*  str, cc_uint8* value);
/* Attempts to convert the given string into an unsigned 16 bit integer. */
CC_API cc_bool Convert_ParseUInt16(const String* str, cc_uint16* value);
/* Attempts to convert the given string into an integer. */
CC_API cc_bool Convert_ParseInt(const String*    str, int* value);
/* Attempts to convert the given string into an unsigned 64 bit integer. */
CC_API cc_bool Convert_ParseUInt64(const String* str, cc_uint64* value);
/* Attempts to convert the given string into a floating point number. */
CC_API cc_bool Convert_ParseFloat(const String*  str, float* value);
/* Attempts to convert the given string into a bool. */
/* NOTE: String must case-insensitively equal "true" or "false" */
CC_API cc_bool Convert_ParseBool(const String*   str, cc_bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
#define STRINGSBUFFER_LEN_SHIFT 9
#define STRINGSBUFFER_LEN_MASK  0x1FFUL

typedef struct StringsBuffer_ {
	char*      textBuffer;  /* Raw characters of all entries */
	cc_uint32*  flagsBuffer; /* Private flags for each entry */
	int count, totalLength;
	/* internal state */
	int      _textCapacity, _flagsCapacity;
	char     _defaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	cc_uint32 _defaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

/* Resets counts to 0, and frees any allocated memory. */
CC_API void StringsBuffer_Clear(StringsBuffer* buffer);
/* UNSAFE: Returns a direct pointer to the i'th string in the given buffer. */
/* You MUST NOT change the characters of this string. Copy to another string if necessary.*/
CC_API STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, int i);
/* Adds a given string to the end of the given buffer. */
CC_API void StringsBuffer_Add(StringsBuffer* buffer, const String* str);
/* Removes the i'th string from the given buffer, shifting following strings downwards. */
CC_API void StringsBuffer_Remove(StringsBuffer* buffer, int index);

/* Performs line wrapping on the given string. */
/* e.g. "some random tex|t* (| is lineLen) becomes "some random" "text" */
void WordWrap_Do(STRING_REF String* text, String* lines, int numLines, int lineLen);
/* Calculates the position of a raw index in the wrapped lines. */
void WordWrap_GetCoords(int index, const String* lines, int numLines, int* coordX, int* coordY);
/* Returns number of characters from current character to end of previous word. */
int  WordWrap_GetBackLength(const String* text, int index);
/* Returns number of characters from current character to start of next word. */
int  WordWrap_GetForwardLength(const String* text, int index);
#endif
