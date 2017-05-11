#ifndef CS_FUNCS_H
#define CS_FUNCS_H
#include "Typedefs.h"
/* Simple function implementations
   NOTE: doing min(x++, y) etc will increment x twice!
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* returns a bit mask for the nth bit in an integer */
#define bit(x) (1 << x)

/* returns smallest of two numbers */
#define min(x, y) ((x) < (y) ? (x) : (y))

/* returns largest of two numbers */
#define max(x, y) ((x) > (y) ? (x) : (y))

/* returns whether character is uppercase letter */
bool Char_IsUpper(UInt8 c);

/* Converts uppercase letter to lowercase */
UInt8 Char_ToLower(UInt8 c);
#endif