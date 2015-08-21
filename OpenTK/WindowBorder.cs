using System;

namespace OpenTK {
	
    /// <summary> Enumerates available window borders. </summary>
    public enum WindowBorder {
    	
        /// <summary> The window has a resizable border. A window with a resizable border can be resized by the user or programmatically. </summary>
        Resizable = 0,
        /// <summary> The window does not have a border. A window with a hidden border can only be resized programmatically. </summary>
        Hidden
    }
}
