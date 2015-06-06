using System;
using System.Collections.Generic;
using System.Drawing;
using System.Diagnostics;
using System.Text;

namespace OpenTK.Platform.MacOS
{
    using Input;

    class CarbonInput : IInputDriver
    {
        List<KeyboardDevice> dummy_keyboard_list = new List<KeyboardDevice>(1);
        List<MouseDevice> dummy_mice_list = new List<MouseDevice>(1);
        List<JoystickDevice> dummy_joystick_list = new List<JoystickDevice>(1);

        internal CarbonInput()
        {
            dummy_mice_list.Add(new MouseDevice());
            dummy_keyboard_list.Add(new KeyboardDevice());
            dummy_joystick_list.Add(new JoystickDevice<object>(0, 0, 0));
        }

        #region IInputDriver Members

        public void Poll()
        {
        }

        #endregion

        #region IKeyboardDriver Members

        public IList<KeyboardDevice> Keyboard
        {
            get { return dummy_keyboard_list; }
        }

        #endregion

        #region IMouseDriver Members

        public IList<MouseDevice> Mouse
        {
            get { return dummy_mice_list; }
        }
        
        // TODO: Implement using native API, rather than through Mono.
        // http://webnnel.googlecode.com/svn/trunk/lib/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/CarbonEventsCore.h
        // GetPos --> GetGlobalMouse (no 64 bit support?), and HIGetMousePosition()
        // https://developer.apple.com/library/mac/documentation/GraphicsImaging/Reference/Quartz_Services_Ref/index.html#//apple_ref/c/func/CGWarpMouseCursorPosition
        // SetPos --> CGWarpMouseCursorPosition
        // Note that: CGPoint uses float on 32 bit systems, double on 64 bit systems
        // The rest of the MacOS OpenTK API will probably need to be fixed for this too...
        public Point DesktopCursorPos {
        	get { return System.Windows.Forms.Cursor.Position; }
        	set { System.Windows.Forms.Cursor.Position = value; }
        }

        #endregion

        #region IJoystickDriver Members

        public IList<JoystickDevice> Joysticks
        {
            get { return dummy_joystick_list; }
        }

        #endregion

        #region IDisposable Members

        public void Dispose()
        {
        }

        #endregion
    }
}
