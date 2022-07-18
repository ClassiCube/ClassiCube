#include "Event.h"
#include "Logger.h"

int EventAPIVersion = 3;
struct _EntityEventsList        EntityEvents;
struct _TabListEventsList       TabListEvents;
struct _TextureEventsList       TextureEvents;
struct _GfxEventsList           GfxEvents;
struct _UserEventsList          UserEvents;
struct _BlockEventsList         BlockEvents;
struct _WorldEventsList         WorldEvents;
struct _ChatEventsList          ChatEvents;
struct _WindowEventsList        WindowEvents;
struct _InputEventsList         InputEvents;
struct _PointerEventsList       PointerEvents;
struct _NetEventsList           NetEvents;

void Event_Register(struct Event_Void* handlers, void* obj, Event_Void_Callback handler) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		/* Attempting to register the same handler twice is usually caused by a bug */
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
}

void Event_UnregisterAll(void) {
	/* NOTE: This MUST be kept in sync with Event.h list of events */
	EntityEvents.Added.Count   = 0;
	EntityEvents.Removed.Count = 0;

	TabListEvents.Added.Count   = 0;
	TabListEvents.Changed.Count = 0;
	TabListEvents.Removed.Count = 0;

	TextureEvents.AtlasChanged.Count = 0;
	TextureEvents.PackChanged.Count  = 0;
	TextureEvents.FileChanged.Count  = 0;

	GfxEvents.ViewDistanceChanged.Count = 0;
	GfxEvents.LowVRAMDetected.Count     = 0;
	GfxEvents.ProjectionChanged.Count   = 0;
	GfxEvents.ContextLost.Count         = 0;
	GfxEvents.ContextRecreated.Count    = 0;

	UserEvents.BlockChanged.Count     = 0;
	UserEvents.HackPermsChanged.Count = 0;
	UserEvents.HeldBlockChanged.Count = 0;

	BlockEvents.PermissionsChanged.Count = 0;
	BlockEvents.BlockDefChanged.Count    = 0;

	WorldEvents.NewMap.Count    = 0;
	WorldEvents.Loading.Count   = 0;
	WorldEvents.MapLoaded.Count = 0;
	WorldEvents.EnvVarChanged.Count = 0;

	ChatEvents.FontChanged.Count    = 0;
	ChatEvents.ChatReceived.Count   = 0;
	ChatEvents.ChatSending.Count    = 0;
	ChatEvents.ColCodeChanged.Count = 0;

	WindowEvents.RedrawNeeded.Count = 0;
	WindowEvents.Resized.Count = 0;
	WindowEvents.Closing.Count = 0;
	WindowEvents.FocusChanged.Count = 0;
	WindowEvents.StateChanged.Count = 0;
	WindowEvents.Created.Count      = 0;
	WindowEvents.InactiveChanged.Count = 0;
	WindowEvents.Redrawing.Count    = 0;

	InputEvents.Press.Count = 0;
	InputEvents.Down.Count  = 0;
	InputEvents.Up.Count    = 0;
	InputEvents.Wheel.Count = 0;
	InputEvents.TextChanged.Count = 0;

	PointerEvents.Moved.Count = 0;
	PointerEvents.Down.Count  = 0;
	PointerEvents.Up.Count    = 0;
	PointerEvents.RawMoved.Count = 0;

	NetEvents.Connected.Count    = 0;
	NetEvents.Disconnected.Count = 0;
	NetEvents.PluginMessageReceived.Count = 0;
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

void Event_RaiseEntry(struct Event_Entry* handlers, struct Stream* stream, const cc_string* name) {
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

void Event_RaiseChat(struct Event_Chat* handlers, const cc_string* msg, int msgType) {
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

void Event_RaiseString(struct Event_String* handlers, const cc_string* str) {
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

void Event_RaisePluginMessage(struct Event_PluginMessage* handlers, cc_uint8 channel, cc_uint8* data) {
	int i;
	for (i = 0; i < handlers->Count; i++) {
		handlers->Handlers[i](handlers->Objs[i], channel, data);
	}
}
