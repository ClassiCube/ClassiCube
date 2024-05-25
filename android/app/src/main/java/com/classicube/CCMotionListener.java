package com.classicube;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

public class CCMotionListener implements View.OnGenericMotionListener {
    MainActivity activity;

    public CCMotionListener(MainActivity activity) {
        this.activity = activity;
    }

    // https://developer.android.com/develop/ui/views/touch-and-input/game-controllers/controller-input#java
    @Override
    public boolean onGenericMotion(View view, MotionEvent event) {
        if (event.getAction() != MotionEvent.ACTION_MOVE) return false;
        boolean source_joystick = (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
        boolean source_gamepad  = (event.getSource() & InputDevice.SOURCE_GAMEPAD)  == InputDevice.SOURCE_GAMEPAD;

        if (source_joystick || source_gamepad) {
            int historySize = event.getHistorySize();
            for (int i = 0; i < historySize; i++) {
                processJoystickInput(event, i);
            }

            processJoystickInput(event, -1);
            return true;
        }
        return false;
    }

    void processJoystickInput(MotionEvent event, int historyPos) {
        float x1 = getAxisValue(event, MotionEvent.AXIS_X,  historyPos);
        float y1 = getAxisValue(event, MotionEvent.AXIS_Y,  historyPos);

        float x2 = getAxisValue(event, MotionEvent.AXIS_Z,  historyPos);
        float y2 = getAxisValue(event, MotionEvent.AXIS_RZ, historyPos);

        if (x1 != 0 || y1 != 0)
            pushAxisMovement(MainActivity.CMD_GPAD_AXISL, x1, y1);
        if (x2 != 0 || y2 != 0)
            pushAxisMovement(MainActivity.CMD_GPAD_AXISR, x2, y2);
    }

    float getAxisValue(MotionEvent event, int axis, int historyPos) {
        float value = historyPos < 0 ? event.getAxisValue(axis) :
                        event.getHistoricalAxisValue(axis, historyPos);

        // Deadzone detection
        if (value >= -0.25f && value <= 0.25f) value = 0;
        return value;
    }

    void pushAxisMovement(int axis, float x, float y) {
        activity.pushCmd(axis, (int)(x * 4096), (int)(y * 4096));
    }
}
