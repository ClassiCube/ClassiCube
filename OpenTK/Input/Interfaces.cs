#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
using System.Collections.Generic;
using System.Drawing;

namespace OpenTK.Input {
	
    /// <summary> Defines a common interface for all input devices. </summary>
    public interface IInputDevice {

        /// <summary> Gets an OpenTK.Input.InputDeviceType value, representing the device type of this IInputDevice instance. </summary>
        InputDeviceType DeviceType { get; }
    }

    /// <summary> The type of the input device. </summary>
    public enum InputDeviceType {
        /// <summary> Device is a keyboard. </summary>
        Keyboard,
        /// <summary> Device is a mouse. </summary>
        Mouse,
        /// <summary> Device is a Human Interface Device. Joysticks, joypads, pens
        /// and some specific usb keyboards/mice fall into this category. </summary>
        Hid
    }
    
    /// <summary> Defines the interface for an input driver. </summary>
    public interface IInputDriver : IJoystickDriver, IDisposable {

    	/// <summary> Gets the list of available KeyboardDevices. </summary>
    	KeyboardDevice Keyboard { get; }
        
        /// <summary> Gets the list of available MouseDevices. </summary>
        MouseDevice Mouse { get; }
        
        /// <summary> Updates the state of the driver. </summary>
        void Poll();
        
        Point DesktopCursorPos { get; set; }
    }
    
    /// <summary> Defines the interface for JoystickDevice drivers. </summary>
    public interface IJoystickDriver {
    	
        /// <summary> Gets the list of available JoystickDevices. </summary>
        IList<JoystickDevice> Joysticks { get; }
    }
}
