#include "EventHandler.h"
#include "ErrorHandler.h"

void EventHandler_RegisterImpl(void** handlers, Int32* count, void* handler) {
	Int32 i;
	for (i = 0; i < *count; i++) {
		if (handlers[i] == handler) return;
	}

	if (*count == EventHandler_Size) {
		ErrorHandler_Fail("Unable to add another event handler");
	}
	handlers[*count] = handler;
	(*count)++;
}

void EventHandler_UnregisterImpl(void** handlers, Int32* count, void* handler) {
	Int32 i, j;
	for (i = 0; i < *count; i++) {
		if (handlers[i] != handler) continue;

		/* Remove this handler from the list, by shifting all following handlers left */
		for (j = i; j < *count - 1; j++) {
			handlers[j] = handlers[j + 1];
		}
		handlers[*count - 1] = NULL;

		(*count)--;
		return;
	}
}


void EventHandler_Raise_Void(Event_Void* handlers, Int32 handlersCount) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i]();
	}
}

void EventHandler_RegisterVoidImpl(Event_Void* handlers, Int32* count, Event_Void handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterVoidImpl(Event_Void* handlers, Int32* count, Event_Void handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}


void EventHandler_Raise_Int32(Event_Int32* handlers, Int32 handlersCount, Int32 arg) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](arg);
	}
}

void EventHandler_RegisterInt32Impl(Event_Int32* handlers, Int32* count, Event_Int32 handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterInt32Impl(Event_Int32* handlers, Int32* count, Event_Int32 handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}


void EventHandler_Raise_Real32(Event_Real32* handlers, Int32 handlersCount, Real32 arg) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](arg);
	}
}

void EventHandler_RegisterReal32Impl(Event_Real32* handlers, Int32* count, Event_Real32 handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterReal32Impl(Event_Real32* handlers, Int32* count, Event_Real32 handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}


void EventHandler_Raise_EntityID(Event_EntityID* handlers, Int32 handlersCount, EntityID arg) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](arg);
	}
}

void EventHandler_RegisterEntityIDImpl(Event_EntityID* handlers, Int32* count, Event_EntityID handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterEntityIDImpl(Event_EntityID* handlers, Int32* count, Event_EntityID handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}


void EventHandler_Raise_Stream(Event_Stream* handlers, Int32 handlersCount, Stream* stream) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](stream);
	}
}

void EventHandler_RegisterStreamImpl(Event_Stream* handlers, Int32* count, Event_Stream handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterStreamImpl(Event_Stream* handlers, Int32* count, Event_Stream handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}


void EventHandler_Raise_Block(Event_Block* handlers, Int32 handlersCount, Vector3I coords, BlockID oldBlock, BlockID block) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](coords, oldBlock, block);
	}
}

void EventHandler_RegisterBlockImpl(Event_Block* handlers, Int32* count, Event_Block handler) {
	EventHandler_RegisterImpl((void**)handlers, count, (void*)handler);
}

void EventHandler_UnregisterBlockImpl(Event_Block* handlers, Int32* count, Event_Block handler) {
	EventHandler_UnregisterImpl((void**)handlers, count, (void*)handler);
}