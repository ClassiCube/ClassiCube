#ifndef CS_STRINGCONVERT_H
#define CS_STRINGCONVERT_H
#include "Typedefs.h"
#include "String.h"
#include "Compiler.h"
/* Implements conversions between code page 437 indices and unicode characters.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/


#define Convert_ControlCharsCount 32
/* Unicode values for the 32 code page 437 control indices. */
UInt16 Convert_ControlChars[Convert_ControlCharsCount];

#define Convert_ExtendedCharsCount 129
/* Unicode values for the 129 extended code page 437 indices. (including DELETE character)*/
UInt16 Convert_ExtendedChars[Convert_ExtendedCharsCount];


/* Converts a code page 437 index into a unicode character. */
UInt16 Convert_CP437ToUnicode(UInt8 c);

/* Converts a unicode character into a code page 437 index. */
UInt8 Convert_UnicodeToCP437(UInt16 c);


/* Attempts to parse the given string as a byte.*/
bool Convert_TryParseByte(STRING_TRANSIENT String* str, UInt8* value);

#endif