#region --- License ---
/* Copyright (c) 2006, 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;
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
    }
    
    /// <summary> Defines the interface for an input driver. </summary>
    public interface IInputDriver : IDisposable {

    	/// <summary> Gets the available KeyboardDevice. </summary>
    	KeyboardDevice Keyboard { get; }
        
        /// <summary> Gets the available MouseDevice. </summary>
        MouseDevice Mouse { get; }
        
        Point DesktopCursorPos { get; set; }
    }
}
