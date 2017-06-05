#include "TickQueue.h"

void TickQueue_Init(TickQueue* queue) {
	queue->Buffer = NULL;
	queue->BufferSize = 0;
	queue->Head = 0;
	queue->Tail = 0;
	queue->Size = 0;
}

void TickQueue_Clear(TickQueue* queue) {
	if (queue->Buffer == NULL) return;
	Platform_MemFree(queue->Buffer);
	TickQueue_Init(queue);
}

void TickQueue_Enqueue(TickQueue* queue, UInt32 item) {
	if (queue->Size == queue->BufferSize)
		TickQueue_Resize(queue);

	queue->Buffer[queue->Tail] = item;
	queue->Tail = (queue->Tail + 1) % queue->BufferSize;
	queue->Size++;
}

UInt32 TickQueue_Dequeue(TickQueue* queue) {
	UInt32 result = queue->Buffer[queue->Head];
	queue->Head = (queue->Head + 1) % queue->BufferSize;
	queue->Size--;
	return result;
}

void TickQueue_Resize(TickQueue* queue) {
	if (queue->BufferSize >= (Int32_MaxValue / 4))
		ErrorHandler_Fail("Queue - too large to resize.");

	Int32 capacity = queue->BufferSize * 2;
	if (capacity < 32) capacity = 32;

	UInt32* newBuffer = Platform_MemAlloc(capacity * sizeof(UInt32));
	if (newBuffer == NULL)
		ErrorHandler_Fail("Queue - failed to allocate memory");
	
	Int32 i, idx;
	for (i = 0; i < queue->Size; i++) {
		idx = (queue->Head + i) % queue->BufferSize;
		newBuffer[i] = queue->Buffer[idx];
	}

	if (queue->Buffer != NULL)
		Platform_MemFree(queue->Buffer);

	queue->Buffer = newBuffer;
	queue->BufferSize = capacity;
	queue->Head = 0;
	queue->Tail = queue->Size;
}