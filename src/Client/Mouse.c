#include "Mouse.h"
#include "Events.h"

bool MouseButton_States[MouseButton_Count];
bool Mouse_GetPressed(MouseButton btn) { return MouseButton_States[btn]; }

void Mouse_SetPressed(MouseButton btn, bool pressed) {
	if (MouseButton_States[btn] != pressed) {
		MouseButton_States[btn] = pressed;

		if (pressed) {
			Event_RaiseInt32(&KeyEvents_KeyDown, key);
		} else {
			Event_RaiseInt32(&KeyEvents_KeyUp, key);
		}
	}
}

void Mouse_SetWheel(Real32 wheel) {
	Mouse_Wheel = wheel;
	Event_RaiseInt32(&KeyEvents_KeyUp, key);
}

void Mouse_SetPosition(Int32 x, Int32 y) {
	Mouse_X = x; Mouse_Y = y;
	Event_RaiseInt32(&KeyEvents_KeyUp, key);
}