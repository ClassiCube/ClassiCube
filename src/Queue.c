#include "Core.h"
#include "Constants.h"
#include "Chat.h"
#include "Platform.h"
#include "Queue.h"

void Queue_Init(struct Queue* queue, cc_uint32 structSize) {
	queue->entries = NULL;
	queue->structSize = structSize;
	queue->capacity = 0;
	queue->mask = 0;
	queue->count = 0;
	queue->head = 0;
	queue->tail = 0;
}
void Queue_Clear(struct Queue* queue) {
	if (!queue->entries) return;
	Mem_Free(queue->entries);
	Queue_Init(queue, queue->structSize);
}
static void Queue_Resize(struct Queue* queue) {
	cc_uint8* entries;
	int i, idx, capacity, headToEndSize, byteOffsetToHead;
	if (queue->capacity >= (Int32_MaxValue / 4)) {
		Chat_AddRaw("&cToo many generic queue entries, clearing");
		Queue_Clear(queue);
		return;
	}
	capacity = queue->capacity * 2;
	if (capacity < 32) capacity = 32;
	entries = (cc_uint8*)Mem_Alloc(capacity, queue->structSize, "Generic queue");

	/* Elements must be readjusted to avoid index wrapping issues */
	headToEndSize = (queue->capacity - queue->head) * queue->structSize;
	byteOffsetToHead = queue->head * queue->structSize;
	/* Copy from head to end */
	Mem_Copy(entries, queue->entries + byteOffsetToHead, headToEndSize);
	if (queue->head != 0) {
		/* If there's any leftover before the head, copy that bit too */
		Mem_Copy(entries + headToEndSize, queue->entries, byteOffsetToHead);
	}

	Mem_Free(queue->entries);

	queue->entries = entries;
	queue->capacity = capacity;
	queue->mask = capacity - 1; /* capacity is power of two */
	queue->head = 0;
	queue->tail = queue->count;
}
/* Appends an entry to the end of the queue, resizing if necessary. */
void Queue_Enqueue(struct Queue* queue, void* item) {
	if (queue->count == queue->capacity)
		Queue_Resize(queue);

	//queue->entries[queue->tail] = item;
	Mem_Copy(queue->entries + queue->tail * queue->structSize, item, queue->structSize);
	queue->tail = (queue->tail + 1) & queue->mask;
	queue->count++;
}
/* Retrieves the entry from the front of the queue. */
void* Queue_Dequeue(struct Queue* queue) {
	void* result = queue->entries + queue->head * queue->structSize;
	queue->head = (queue->head + 1) & queue->mask;
	queue->count--;
	return result;
}