#include "Event.h"
#include "ErrorHandler.h"

void Event_RegisterImpl(Event_Void* handlers, Event_Void_Callback handler) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] == handler) return;
	}

	if (handlers->Count == Event_MaxCallbacks) {
		ErrorHandler_Fail("Unable to register another event handler");
	} else {
		handlers->Handlers[handlers->Count] = handler;
		handlers->Count++;
	}
}

void Event_UnregisterImpl(Event_Void* handlers, Event_Void_Callback handler) {
	Int32 i, j;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] != handler) continue;

		/* Remove this handler from the list, by shifting all following handlers left */
		for (j = i; j < handlers->Count - 1; j++) {
			handlers->Handlers[j] = handlers->Handlers[j + 1];
		}
		
		handlers->Handlers[handlers->Count - 1] = NULL;
		handlers->Count--;
		return;
	}
	ErrorHandler_Fail("Unregistering event handler that was not registered to begin with");
}

void Event_RaiseVoid(Event_Void* handlers) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i]();
	}
}
void Event_RegisterVoid(Event_Void* handlers, Event_Void_Callback handler) {
	Event_RegisterImpl(handlers, handler);
}
void Event_UnregisterVoid(Event_Void* handlers, Event_Void_Callback handler) {
	Event_UnregisterImpl(handlers, handler);
}

void Event_RaiseInt32(Event_Int32* handlers, Int32 arg) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](arg);
	}
}
void Event_RegisterInt32(Event_Int32* handlers,Event_Int32_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}
void Event_UnregisterInt32(Event_Int32* handlers, Event_Int32_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_RaiseReal32(Event_Real32* handlers, Real32 arg) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](arg);
	}
}
void Event_RegisterReal32(Event_Real32* handlers, Event_Real32_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_UnregisterReal32(Event_Real32* handlers, Event_Real32_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_RaiseEntityID(Event_EntityID* handlers, EntityID arg) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](arg);
	}
}
void Event_RegisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}
void Event_UnregisterEntityID(Event_EntityID* handlers, Event_EntityID_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_RaiseStream(Event_Stream* handlers, Stream* stream) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](stream);
	}
}
void Event_RegisterStream(Event_Stream* handlers, Event_Stream_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}
void Event_UnregisterStream(Event_Stream* handlers, Event_Stream_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_RaiseBlock(Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](coords, oldBlock, block);
	}
}
void Event_RegisterBlock(Event_Block* handlers, Event_Block_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}
void Event_UnregisterBlock(Event_Block* handlers, Event_Block_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}

void Event_RaiseMouseMove(Event_MouseMove* handlers, Int32 xDelta, Int32 yDelta) {
	Int32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](xDelta, yDelta);
	}
}
void Event_RegisterMouseMove(Event_MouseMove* handlers, Event_MouseMove_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}
void Event_UnregisterMouseMove(Event_MouseMove* handlers, Event_MouseMove_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, (Event_Void_Callback)handler);
}