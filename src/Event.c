#include "Event.h"
#include "ErrorHandler.h"

static void Event_RegisterImpl(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
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

static void Event_UnregisterImpl(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	UInt32 i, j;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] != handler || handlers->Objs[i] != obj) continue;

		/* Remove this handler from the list, by shifting all following handlers left */
		for (j = i; j < handlers->Count - 1; j++) {
			handlers->Handlers[j] = handlers->Handlers[j + 1];
			handlers->Objs[j]     = handlers->Objs[j + 1];
		}
		
		handlers->Count--;
		handlers->Handlers[handlers->Count] = NULL;
		handlers->Objs[handlers->Count]     = NULL;
		return;
	}
	ErrorHandler_Fail("Attempt to unregister event handler that was not registered to begin with");
}

void Event_RaiseVoid(struct Event_Void* handlers) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i]);
	}
}
void Event_RegisterVoid(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	Event_RegisterImpl(handlers, obj, handler);
}
void Event_UnregisterVoid(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	Event_UnregisterImpl(handlers, obj, handler);
}

void Event_RaiseInt(struct Event_Int* handlers, int arg) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}
void Event_RegisterInt(struct Event_Int* handlers, void* obj, Event_Int_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterInt(struct Event_Int* handlers, void* obj, Event_Int_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseFloat(struct Event_Float* handlers, float arg) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}
void Event_RegisterFloat(struct Event_Float* handlers, void* obj, Event_Float_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_UnregisterFloat(struct Event_Float* handlers, void* obj, Event_Float_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseEntry(struct Event_Entry* handlers, struct Stream* stream, const String* name) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], stream, name);
	}
}
void Event_RegisterEntry(struct Event_Entry* handlers, void* obj, Event_Entry_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterEntry(struct Event_Entry* handlers, void* obj, Event_Entry_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseBlock(struct Event_Block* handlers, Vector3I coords, BlockID oldBlock, BlockID block) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], coords, oldBlock, block);
	}
}
void Event_RegisterBlock(struct Event_Block* handlers, void* obj, Event_Block_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterBlock(struct Event_Block* handlers, void* obj, Event_Block_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseMouseMove(struct Event_MouseMove* handlers, int xDelta, int yDelta) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], xDelta, yDelta);
	}
}
void Event_RegisterMouseMove(struct Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterMouseMove(struct Event_MouseMove* handlers, void* obj, Event_MouseMove_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}

void Event_RaiseChat(struct Event_Chat* handlers, const String* msg, int msgType) {
	UInt32 i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], msg, msgType);
	}
}
void Event_RegisterChat(struct Event_Chat* handlers, void* obj, Event_Chat_Callback handler) {
	Event_RegisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
void Event_UnregisterChat(struct Event_Chat* handlers, void* obj, Event_Chat_Callback handler) {
	Event_UnregisterImpl((struct Event_Void*)handlers, obj, (Event_Void_Callback)handler);
}
