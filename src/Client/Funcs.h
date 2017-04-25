#ifndef CS_FUNCS_H
#define CS_FUNCS_H
/* Simple function implementations
   NOTE: doing min(x++, y) etc will increment x twice!
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

// returns a bit mask for the nth bit in an integer
#define bit(x) (1 << x)

// returns smallest of two numbers
#define min(x, y) ((x) < (y) ? (x) : (y))

// returns largest of two numbers
#define max(x, y) ((x) > (y) ? (x) : (y))

// returns absolute value of a number
#define abs(x) (x >= 0 ? x : -x)
#endif