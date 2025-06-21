#ifndef CC_FUNCS_H
#define CC_FUNCS_H
#include "Core.h"
CC_BEGIN_HEADER

/* 
Simple function implementations
  NOTE: doing min(x++, y) etc will increment x twice!
Copyright 2014-2025 ClassiCube | Licensed under BSD-3
*/

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define Array_Elems(arr) (sizeof(arr) / sizeof(arr[0]))
union IntAndFloat { float f; cc_int32 i; cc_uint32 u; };

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

#define LinkedList_Append(item, head, tail)\
if (!head) { head = item; } else { tail->next = item; }\
tail       = item;\
item->next = NULL;

#define LinkedList_Remove(item, cur, head, tail)\
cur = head; \
if (head == item) head = item->next;\
\
while (cur) {\
	if (cur->next == item) cur->next = item->next; \
	\
	tail = cur;\
	cur  = cur->next;\
}

CC_END_HEADER
#endif
