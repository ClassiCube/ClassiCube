#include "EventHandler.h"
#include "ErrorHandler.h"

void EventHandler_Register(void** handlers, Int32* count, void* handler) {
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

void EventHandler_Unregister(void** handlers, Int32* count, void* handler) {
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

void EventHandler_Raise_Int32(Event_Int32* handlers, Int32 handlersCount, Int32 arg) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](arg);
	}
}

void EventHandler_Raise_Float32(Event_Float32* handlers, Int32 handlersCount, Real32 arg) {
	Int32 i;
	for (i = 0; i < handlersCount; i++) {
		handlers[i](arg);
	}
}