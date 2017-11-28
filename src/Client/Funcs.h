#ifndef CC_FUNCS_H
#define CC_FUNCS_H
#include "Typedefs.h"
/* Simple function implementations
   NOTE: doing min(x++, y) etc will increment x twice!
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* returns a bit mask for the nth bit in an integer */
#define bit(x) (1 << (x))
/* returns smallest of two numbers */
#define min(x, y) ((x) < (y) ? (x) : (y))
/* returns largest of two numbers */
#define max(x, y) ((x) > (y) ? (x) : (y))
/* returns number of elements in given array. */
#define Array_NumElements(arr) (sizeof(arr) / sizeof(arr[0]))

#define QuickSort_Swap_Maybe()\
if (i <= j) {\
	key = keys[i]; keys[i] = keys[j]; keys[j] = key;\
	i++; j--;\
}

#define QuickSort_Swap_KV_Maybe()\
if (i <= j) {\
	key = keys[i]; keys[i] = keys[j]; keys[j] = key;\
	value = values[i]; values[i] = values[j]; values[j] = value;\
	i++; j--;\
}

#define QuickSort_Recurse(quickSort)\
if (j - left <= right - i) {\
	if (left < j) { quickSort(left, j); }\
	left = i;\
} else {\
	if (i < right) { quickSort(i, right); }\
	right = j;\
}
#endif