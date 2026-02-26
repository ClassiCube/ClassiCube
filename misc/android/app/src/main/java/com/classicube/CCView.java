package com.classicube;
import android.text.Editable;
import android.text.Selection;
import android.text.SpannableStringBuilder;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

public class CCView extends SurfaceView {
    SpannableStringBuilder kbText;
    MainActivity activity;

    public CCView(MainActivity activity) {
        // setFocusable, setFocusableInTouchMode - API level 1
        super(activity);
        this.activity = activity;
        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        return activity.handleTouchEvent(ev) || super.dispatchTouchEvent(ev);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo attrs) {
        // BaseInputConnection, IME_ACTION_GO, IME_FLAG_NO_EXTRACT_UI - API level 3
        attrs.actionLabel = null;
        attrs.inputType   = MainActivity.calcKeyboardType(activity.keyboardType);
        attrs.imeOptions  = MainActivity.calcKeyboardOptions(activity.keyboardType);

        kbText = new SpannableStringBuilder(activity.keyboardText);

        InputConnection ic = new BaseInputConnection(this, true) {
            boolean inited;

            void updateText() {
                activity.pushCmd(MainActivity.CMD_KEY_TEXT, kbText.toString());
            }

            @Override
            public Editable getEditable() {
                if (!inited) {
                    // needed to set selection, otherwise random crashes later with backspacing
                    // set selection to end, so backspacing after opening keyboard with text still works
                    Selection.setSelection(kbText, kbText.toString().length());
                    inited = true;
                }
                return kbText;
            }

            @Override
            public boolean setComposingText(CharSequence text, int newCursorPosition) {
                boolean success = super.setComposingText(text, newCursorPosition);
                updateText();
                return success;
            }

            @Override
            public boolean deleteSurroundingText(int beforeLength, int afterLength) {
                boolean success = super.deleteSurroundingText(beforeLength, afterLength);
                updateText();
                return success;
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                boolean success = super.commitText(text, newCursorPosition);
                updateText();
                return success;
            }

            @Override
            public boolean sendKeyEvent(KeyEvent ev) {
                // getSelectionStart - API level 1
                if (ev.getAction() != KeyEvent.ACTION_DOWN) return super.sendKeyEvent(ev);
                int code = ev.getKeyCode();
                int uni = ev.getUnicodeChar();

                // start is -1 sometimes, and trying to insert/delete there crashes
                int start = Selection.getSelectionStart(kbText);
                if (start == -1) start = kbText.toString().length();

                if (code == KeyEvent.KEYCODE_ENTER) {
                    // enter maps to \n but that should not be intercepted
                } else if (code == KeyEvent.KEYCODE_DEL) {
                    if (start <= 0) return false;
                    kbText.delete(start - 1, start);
                    updateText();
                    return false;
                } else if (uni != 0) {
                    kbText.insert(start, String.valueOf((char) uni));
                    updateText();
                    return false;
                }
                return super.sendKeyEvent(ev);
            }

        };
        //String text = MainActivity.this.keyboardText;
        //if (text != null) ic.setComposingText(text, 0);
        return ic;
    }
}
