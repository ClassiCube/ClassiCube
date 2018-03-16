#ifndef CC_STRING_H
#define CC_STRING_H
#include "Typedefs.h"
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

bool Char_IsUpper(UInt8 c);
UInt8 Char_ToLower(UInt8 c);

typedef struct String_ {	
	UInt8* buffer;   /* Pointer to raw characters. Size is capacity + 1, as buffer is null terminated. */	
	UInt16 length;   /* Number of characters used. */
	UInt16 capacity; /* Max number of characters that can be in buffer. */
} String;

String String_MakeNull(void);
String String_Init(STRING_REF UInt8* buffer, UInt16 length, UInt16 capacity);
String String_InitAndClear(STRING_REF UInt8* buffer, UInt16 capacity);
#define String_InitAndClearArray(buffer) String_InitAndClear(buffer, (UInt16)(sizeof(buffer) - 1))
/* Constructs a new string from a (maybe null terminated) buffer. */
String String_FromRaw(STRING_REF UInt8* buffer, UInt16 capacity);
/* Constructs a new string from a constant readonly buffer. */
String String_FromReadonly(STRING_REF const UInt8* buffer);
/* Constructs a new string from a compile time string constant. */
#define String_FromConst(text) { text, (UInt16)(sizeof(text) - 1), (UInt16)(sizeof(text) - 1)};
/* Constructs a new string from a compile time empty string buffer. */
#define String_FromEmptyArray(buffer) { buffer, 0, (UInt16)(sizeof(buffer) - 1)};
/* Constructs a new string from a compile time array, that may have arbitary actual length of data at runtime */
#define String_FromRawArray(buffer) String_FromRaw(buffer, (UInt16)(sizeof(buffer) - 1))

void String_MakeLowercase(STRING_TRANSIENT String* str);
void String_Clear(STRING_TRANSIENT String* str);
/* Returns a string that points directly to a substring of the given string.
NOTE: THIS IS UNSAFE - IT MAINTAINS A REFERENCE TO THE ORIGINAL BUFFER, AND THE SUBSTRING IS NOT NULL TERMINATED */
String String_UNSAFE_Substring(STRING_REF String* str, Int32 offset, Int32 length);
#define String_UNSAFE_SubstringAt(str, offset) (String_UNSAFE_Substring(str, offset, (str)->length - (offset)))
/* Splits a string into a sequence of substrings
NOTE: THIS IS UNSAFE - IT MAINTAINS A REFERENCE TO THE ORIGINAL BUFFER, AND THE SUBSTRING IS NOT NULL TERMINATED */
void String_UNSAFE_Split(STRING_REF String* str, UInt8 c, STRING_TRANSIENT String* subs, UInt32* subsCount);

bool String_Equals(STRING_PURE String* a, STRING_PURE String* b);
bool String_CaselessEquals(STRING_PURE String* a, STRING_PURE String* b);

bool String_Append(STRING_TRANSIENT String* str, UInt8 c);
bool String_AppendBool(STRING_TRANSIENT String* str, bool value);
bool String_AppendInt32(STRING_TRANSIENT String* str, Int32 num);
/* Attempts to append an integer value to the end of a string, padding left with 0. */
bool String_AppendPaddedInt32(STRING_TRANSIENT String* str, Int32 num, Int32 minDigits);
bool String_AppendConst(STRING_TRANSIENT String* str, const UInt8* toAppend);
bool String_AppendString(STRING_TRANSIENT String* str, STRING_PURE String* toAppend);
bool String_AppendColorless(STRING_TRANSIENT String* str, STRING_PURE String* toAppend);

Int32 String_IndexOf(STRING_PURE String* str, UInt8 c, Int32 offset);
Int32 String_LastIndexOf(STRING_PURE String* str, UInt8 c);
UInt8 String_CharAt(STRING_PURE String* str, Int32 offset);
void String_InsertAt(STRING_TRANSIENT String* str, Int32 offset, UInt8 c);
void String_DeleteAt(STRING_TRANSIENT String* str, Int32 offset);

Int32 String_IndexOfString(STRING_PURE String* str, STRING_PURE String* sub);
#define String_ContainsString(str, sub) (String_IndexOfString(str, sub) >= 0)
bool String_StartsWith(STRING_PURE String* str, STRING_PURE String* sub);
bool String_CaselessStarts(STRING_PURE String* str, STRING_PURE String* sub);
Int32 String_Compare(STRING_PURE String* a, STRING_PURE String* b);

UInt16 Convert_CP437ToUnicode(UInt8 c);
UInt8 Convert_UnicodeToCP437(UInt16 c);
bool Convert_TryParseUInt8(STRING_PURE String* str, UInt8* value);
bool Convert_TryParseInt16(STRING_PURE String* str, Int16* value);
bool Convert_TryParseUInt16(STRING_PURE String* str, UInt16* value);
bool Convert_TryParseInt32(STRING_PURE String* str, Int32* value);
bool Convert_TryParseReal32(STRING_PURE String* str, Real32* value);
bool Convert_TryParseBool(STRING_PURE String* str, bool* value);

#define STRINGSBUFFER_BUFFER_DEF_SIZE 4096
#define STRINGSBUFFER_BUFFER_EXPAND_SIZE 8192
#define STRINGSBUFFER_FLAGS_DEF_ELEMS 256
#define STRINGSBUFFER_FLAGS_EXPAND_ELEMS 512
typedef struct StringsBuffer_ {
	UInt8* TextBuffer; 
	UInt32* FlagsBuffer;
	UInt32 TextBufferElems, FlagsBufferElems;
	UInt32 Count, UsedElems;
	UInt8 DefaultBuffer[STRINGSBUFFER_BUFFER_DEF_SIZE];
	UInt32 DefaultFlags[STRINGSBUFFER_FLAGS_DEF_ELEMS];
} StringsBuffer;

void StringsBuffer_Init(StringsBuffer* buffer);
void StringsBuffer_Free(StringsBuffer* buffer);
void StringsBuffer_UNSAFE_Reset(StringsBuffer* buffer);
void StringsBuffer_Get(StringsBuffer* buffer, UInt32 index, STRING_TRANSIENT String* text);
STRING_REF String StringsBuffer_UNSAFE_Get(StringsBuffer* buffer, UInt32 index);
void StringsBuffer_Add(StringsBuffer* buffer, STRING_PURE String* text);
void StringsBuffer_Remove(StringsBuffer* buffer, UInt32 index);
#endif