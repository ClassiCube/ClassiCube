#include "Key.h"
#include "Events.h"

bool Key_States[Key_Count];
bool Key_GetPressed(Key key) { return Key_States[key]; }

void Key_SetPressed(Key key, bool pressed) {
	if (Key_States[key] != pressed || Key_KeyRepeat) {
		Key_States[key] = pressed;

		if (pressed) {
			Event_RaiseInt32(&KeyEvents_KeyDown, key);
		} else {
			Event_RaiseInt32(&KeyEvents_KeyUp, key);
		}
	}
}

void Key_Clear(void) {
	Int32 i;
	for (i = 0; i < Key_Count; i++) {
		if (Key_States[i]) Key_SetPressed((Key)i, false);
	}
}