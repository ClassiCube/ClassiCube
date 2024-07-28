#ifndef CC_QUEUE_H
#define CC_QUEUE_H
#include "Core.h"
CC_BEGIN_HEADER

struct Queue {
	cc_uint8* entries;    /* Buffer holding the bytes of the queue */
	int structSize; /* Size in bytes of the type of structure this queue holds */
	int capacity;        /* Max number of elements in the buffer */
	int mask;            /* capacity - 1, as capacity is always a power of two */
	int count;           /* Number of used elements */
	int head;            /* Head index into the buffer */
	int tail;            /* Tail index into the buffer */
};
void Queue_Init(struct Queue* queue, cc_uint32 structSize);
/* Appends an entry to the end of the queue, resizing if necessary. */
void Queue_Enqueue(struct Queue* queue, void* item);
/* Retrieves the entry from the front of the queue. */
void* Queue_Dequeue(struct Queue* queue);
/* Frees the memory of the queue and resets the members to 0. */
void Queue_Clear(struct Queue* queue);

CC_END_HEADER
#endif
