#include "Event.h"
#include "Logger.h"

struct _EntityEventsList  EntityEvents;
struct _TabListEventsList TabListEvents;
struct _TextureEventsList TextureEvents;
struct _GfxEventsList     GfxEvents;
struct _UserEventsList    UserEvents;
struct _BlockEventsList   BlockEvents;
struct _WorldEventsList   WorldEvents;
struct _ChatEventsList    ChatEvents;
struct _WindowEventsList  WindowEvents;
struct _InputEventsList   InputEvents;
struct _PointerEventsList PointerEvents;
struct _NetEventsList     NetEvents;

void Event_Register(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		if (handlers->Handlers[i] == handler && handlers->Objs[i] == obj) {
			Logger_Abort("Attempt to register event handler that was already registered");
		}
	}

	if (handlers->Count == EVENT_MAX_CALLBACKS) {
		Logger_Abort("Unable to register another event handler");
	} else {
		handlers->Handlers[handlers->Count] = handler;
		handlers->Objs[handlers->Count]     = obj;
		handlers->Count++;
	}
}

void Event_Unregister(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	int i, j;
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
	Logger_Abort("Attempt to unregister event handler that was not registered to begin with");
}

void Event_RaiseVoid(struct Event_Void* handlers) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i]);
	}
}

void Event_RaiseInt(struct Event_Int* handlers, int arg) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}

void Event_RaiseFloat(struct Event_Float* handlers, float arg) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], arg);
	}
}

void Event_RaiseEntry(struct Event_Entry* handlers, struct Stream* stream, const String* name) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], stream, name);
	}
}

void Event_RaiseBlock(struct Event_Block* handlers, IVec3 coords, BlockID oldBlock, BlockID block) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], coords, oldBlock, block);
	}
}

void Event_RaiseMove(struct Event_PointerMove* handlers, int idx, int xDelta, int yDelta) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], idx, xDelta, yDelta);
	}
}

void Event_RaiseChat(struct Event_Chat* handlers, const String* msg, int msgType) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], msg, msgType);
	}
}

void Event_RaiseInput(struct Event_Input* handlers, int key, cc_bool repeating) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], key, repeating);
	}
}

void Event_RaiseString(struct Event_String* handlers, const String* str) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], str);
	}
}

void Event_RaiseRawMove(struct Event_RawMove* handlers, float xDelta, float yDelta) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], xDelta, yDelta);
	}
}
