#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.InteropServices;
using OpenTK.Input;

namespace OpenTK.Platform.X11
{
    /// \internal
    /// <summary>
    /// Drives the InputDriver on X11.
    /// This class supports OpenTK, and is not intended for users of OpenTK.
    /// </summary>
    internal sealed class X11Input : IInputDriver
    {
        KeyboardDevice keyboard = new KeyboardDevice();
        MouseDevice mouse = new MouseDevice();

        X11KeyMap keymap = new X11KeyMap();
        int firstKeyCode, lastKeyCode; // The smallest and largest KeyCode supported by the X server.
        int keysyms_per_keycode;    // The number of KeySyms for each KeyCode.
        IntPtr[] keysyms;

        #region --- Constructors ---

        /// <summary>
        /// Constructs a new X11Input driver. Creates a hidden InputOnly window, child to
        /// the main application window, which selects input events and routes them to 
        /// the device specific drivers (Keyboard, Mouse, Hid).
        /// </summary>
        /// <param name="attach">The window which the InputDriver will attach itself on.</param>
        public X11Input(IWindowInfo attach)
        {
            Debug.WriteLine("Initalizing X11 input driver.");
            Debug.Indent();

            if (attach == null)
                throw new ArgumentException("A valid parent window must be defined, in order to create an X11Input driver.");

            //window = new X11WindowInfo(attach);
            X11WindowInfo window = (X11WindowInfo)attach;
            
                // Init keyboard
                API.XDisplayKeycodes(window.Display, ref firstKeyCode, ref lastKeyCode);
                Debug.Print("First keycode: {0}, last {1}", firstKeyCode, lastKeyCode);
    
                IntPtr keysym_ptr = API.XGetKeyboardMapping(window.Display, (byte)firstKeyCode,
                    lastKeyCode - firstKeyCode + 1, ref keysyms_per_keycode);
                Debug.Print("{0} keysyms per keycode.", keysyms_per_keycode);
    
                keysyms = new IntPtr[(lastKeyCode - firstKeyCode + 1) * keysyms_per_keycode];
                Marshal.PtrToStructure(keysym_ptr, keysyms);
                Functions.XFree(keysym_ptr);
    
                // Request that auto-repeat is only set on devices that support it physically.
                // This typically means that it's turned off for keyboards (which is what we want).
                // We prefer this method over XAutoRepeatOff/On, because the latter needs to
                // be reset before the program exits.
                bool supported;
                Functions.XkbSetDetectableAutoRepeat(window.Display, true, out supported);
            Debug.Unindent();
        }

        #endregion

        #region internal void ProcessEvent(ref XEvent e)

        internal void ProcessEvent(ref XEvent e)
        {
            switch (e.type)
            {
                case XEventName.KeyPress:
                case XEventName.KeyRelease:
                    bool pressed = e.type == XEventName.KeyPress;

                    IntPtr keysym = API.XLookupKeysym(ref e.KeyEvent, 0);
                    IntPtr keysym2 = API.XLookupKeysym(ref e.KeyEvent, 1);

                    if (keymap.ContainsKey((XKey)keysym))
                        keyboard[keymap[(XKey)keysym]] = pressed;
                    else if (keymap.ContainsKey((XKey)keysym2))
                        keyboard[keymap[(XKey)keysym2]] = pressed;
                    else
                        Debug.Print("KeyCode {0} (Keysym: {1}, {2}) not mapped.", e.KeyEvent.keycode, (XKey)keysym, (XKey)keysym2);
                    break;

                case XEventName.ButtonPress:
                    if      (e.ButtonEvent.button == 1) mouse[OpenTK.Input.MouseButton.Left] = true;
                    else if (e.ButtonEvent.button == 2) mouse[OpenTK.Input.MouseButton.Middle] = true;
                    else if (e.ButtonEvent.button == 3) mouse[OpenTK.Input.MouseButton.Right] = true;
                    else if (e.ButtonEvent.button == 4) mouse.Wheel++;
                    else if (e.ButtonEvent.button == 5) mouse.Wheel--;
                    else if (e.ButtonEvent.button == 6) mouse[OpenTK.Input.MouseButton.Button1] = true;
                    else if (e.ButtonEvent.button == 7) mouse[OpenTK.Input.MouseButton.Button2] = true;
                    else if (e.ButtonEvent.button == 8) mouse[OpenTK.Input.MouseButton.Button3] = true;
                    else if (e.ButtonEvent.button == 9) mouse[OpenTK.Input.MouseButton.Button4] = true;
                    else if (e.ButtonEvent.button == 10) mouse[OpenTK.Input.MouseButton.Button5] = true;
                    else if (e.ButtonEvent.button == 11) mouse[OpenTK.Input.MouseButton.Button6] = true;
                    else if (e.ButtonEvent.button == 12) mouse[OpenTK.Input.MouseButton.Button7] = true;
                    else if (e.ButtonEvent.button == 13) mouse[OpenTK.Input.MouseButton.Button8] = true;
                    else if (e.ButtonEvent.button == 14) mouse[OpenTK.Input.MouseButton.Button9] = true;
                    //if ((e.state & (int)X11.MouseMask.Button4Mask) != 0) m.Wheel++;
                    //if ((e.state & (int)X11.MouseMask.Button5Mask) != 0) m.Wheel--;
                    //Debug.Print("Button pressed: {0}", e.ButtonEvent.button);
                    break;

                case XEventName.ButtonRelease:
                    if      (e.ButtonEvent.button == 1) mouse[OpenTK.Input.MouseButton.Left] = false;
                    else if (e.ButtonEvent.button == 2) mouse[OpenTK.Input.MouseButton.Middle] = false;
                    else if (e.ButtonEvent.button == 3) mouse[OpenTK.Input.MouseButton.Right] = false;
                    else if (e.ButtonEvent.button == 6) mouse[OpenTK.Input.MouseButton.Button1] = false;
                    else if (e.ButtonEvent.button == 7) mouse[OpenTK.Input.MouseButton.Button2] = false;
                    else if (e.ButtonEvent.button == 8) mouse[OpenTK.Input.MouseButton.Button3] = false;
                    else if (e.ButtonEvent.button == 9) mouse[OpenTK.Input.MouseButton.Button4] = false;
                    else if (e.ButtonEvent.button == 10) mouse[OpenTK.Input.MouseButton.Button5] = false;
                    else if (e.ButtonEvent.button == 11) mouse[OpenTK.Input.MouseButton.Button6] = false;
                    else if (e.ButtonEvent.button == 12) mouse[OpenTK.Input.MouseButton.Button7] = false;
                    else if (e.ButtonEvent.button == 13) mouse[OpenTK.Input.MouseButton.Button8] = false;
                    else if (e.ButtonEvent.button == 14) mouse[OpenTK.Input.MouseButton.Button9] = false;
                    break;

                case XEventName.MotionNotify:
                    mouse.Position = new Point(e.MotionEvent.x, e.MotionEvent.y);
                    break;
            }
        }

        #endregion

        #region --- IInputDriver Members ---

        public KeyboardDevice Keyboard {
        	get { return keyboard; }
        }

        public MouseDevice Mouse {
        	get { return mouse; }
        }
        
        // TODO: Implement using native API, rather than through Mono.
        public Point DesktopCursorPos {
        	get { return System.Windows.Forms.Cursor.Position; }
        	set { System.Windows.Forms.Cursor.Position = value; }
        }

        #endregion

        public void Dispose() {
        }
    }
}
