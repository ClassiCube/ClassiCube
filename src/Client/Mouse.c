#include "Mouse.h"
#include "Events.h"

bool MouseButton_States[MouseButton_Count];
bool Mouse_GetPressed(MouseButton btn) { return MouseButton_States[btn]; }

void Mouse_SetPressed(MouseButton btn, bool pressed) {
	if (MouseButton_States[btn] != pressed) {
		MouseButton_States[btn] = pressed;

		if (pressed) {
			Event_RaiseInt32(&MouseEvents_ButtonDown, btn);
		} else {
			Event_RaiseInt32(&MouseEvents_ButtonUp, btn);
		}
	}
}

void Mouse_SetWheel(Real32 wheel) {
	Real32 delta = wheel - Mouse_Wheel;
	Mouse_Wheel = wheel;
	Event_RaiseReal32(&MouseEvents_WheelChanged, delta);
}

void Mouse_SetPosition(Int32 x, Int32 y) {
	Int32 deltaX = x - Mouse_X, deltaY = y - Mouse_Y;
	Mouse_X = x; Mouse_Y = y;
	Event_RaiseMouseMove(&MouseEvents_Move, deltaX, deltaY);
}