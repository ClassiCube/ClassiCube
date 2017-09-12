#ifndef CS_QUEUE_H
#define CS_QUEUE_H
#include "Typedefs.h"
/* Implements a simple resizable queue, for liquid physics tick entries.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

/* Data for a resizable queue. */
typedef struct TickQueue_ {
	/* Buffer holding the items in the tick queue. */
	UInt32* Buffer;
	/* Max number of elements in the buffer.*/
	Int32 BufferSize;
	/* BufferSize - 1, as BufferSize is always a power of two. */
	Int32 BufferMask;
	/* Number of used elements. */
	Int32 Size;
	/* Head index into the buffer. */
	Int32 Head;
	/* Tail index into the buffer. */
	Int32 Tail;
} TickQueue;

/* Initalises the queue. */
void TickQueue_Init(TickQueue* queue);
/* Clears and deallocates buffer of the queue. */
void TickQueue_Clear(TickQueue* queue);
/* Adds an item to the end of the queue. */
void TickQueue_Enqueue(TickQueue* queue, UInt32 item);
/* Retrieves the item at the front of the queue.
NOTE: You must check size is > 0 before calling this! */
UInt32 TickQueue_Dequeue(TickQueue* queue);
#endif