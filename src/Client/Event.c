#include "Event.h"
#include "ErrorHandler.h"

void Event_RegisterImpl(Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] == handler && handlers->Objs[i] == obj) {
			ErrorHandler_Fail("Attempt to register event handler that was already registered");
		}
	}

	if (handlers->Count == EVENT_MAX_CALLBACKS) {
		ErrorHandler_Fail("Unable to register another event handler");
	} else {
		handlers->Handlers[handlers->Count] = handler;
		handlers->Objs[handlers->Count]     = obj;
		handlers->Count++;
	}
}

void Event_UnregisterImpl(Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	UInt32 i, j;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] != handler || handlers->Objs[i] != obj) continue;

		/* Remove this handler from the list, by shifting all following handlers left */
		for (j = i; j < handlers->Count - 1; j++) {
			handlers->Handlers[j] = handlers->Handlers[j + 1];
			handlers->Objs[j]     = handlers->Objs[j + 1];
		}
		
		handlers->Handlers[handlers->Count - 1] = NULL;
		handlers->Objs[handlers->Count - 1]     = NULL;
		handlers->Count--;
		return;
	}
	ErrorHandler_Fail("Attempt to unregister event handler that was not registered to begin with");
}

void Event_RaiseVoid(Event_Void* handlers) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i]);
	}
}
void Event_RegisterVoid(Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	Event_RegisterImpl(handlers, obj, handler);
}
void Event_UnregisterVoid(Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	Event_UnregisterImpl(handlers, obj, handler);
}

void Event_RaiseInt(Event_Int* handlers, Int32 arg) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}
void Event_RegisterInt(Event_Int* handlers, void* obj, Event_Int_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterInt(Event_Int* handlers, void* obj, Event_Int_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseReal(Event_Real* handlers, Real32 arg) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}
void Event_RegisterReal(Event_Real* handlers, void* obj, Event_Real_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_UnregisterReal(Event_Real* handlers, void* obj, Event_Real_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseStream(Event_Stream* handlers, Stream* stream) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], stream);
	}
}
void Event_RegisterStream(Event_Stream* handlers, void* obj, Event_Stream_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterStream(Event_Stream* handlers, void* obj, Event_Stream_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseBlock(Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], coords, oldBlock, block);
	}
}
void Event_RegisterBlock(Event_Block* handlers, void* obj, Event_Block_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterBlock(Event_Block* handlers, void* obj, Event_Block_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseMouseMove(Event_MouseMove* handlers, Int32 xDelta, Int32 yDelta) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], xDelta, yDelta);
	}
}
void Event_RegisterMouseMove(Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterMouseMove(Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseChat(Event_Chat* handlers, String* msg, Int32 msgType) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], msg, msgType);
	}
}
void Event_RegisterChat(Event_Chat* handlers, void* obj, Event_Chat_Callback handler) {
	Event_RegisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterChat(Event_Chat* handlers, void* obj, Event_Chat_Callback handler) {
	Event_UnregisterImpl((Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}