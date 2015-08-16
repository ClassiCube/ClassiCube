#region --- License ---
/* Copyright (c) 2007 Stefanos Apostolopoulos
 * See license.txt for license info
 */
#endregion

using System;

namespace OpenTK.Input
{
    /// <summary>
    /// Represents a keyboard device and provides methods to query its status. 
    /// </summary>
    public sealed class KeyboardDevice : IInputDevice
    {
        private bool[] keys = new bool[(int)Key.LastKey];
        private bool repeat;
        private KeyboardKeyEventArgs args = new KeyboardKeyEventArgs();

        #region --- Constructors ---

        internal KeyboardDevice() { }

        #endregion

        #region --- IKeyboard members ---

        /// <summary>
        /// Gets a value indicating the status of the specified Key.
        /// </summary>
        /// <param name="key">The Key to check.</param>
        /// <returns>True if the Key is pressed, false otherwise.</returns>
        public bool this[Key key]
        {
            get { return keys[(int)key]; }
            internal set
            {
                if (keys[(int)key] != value || KeyRepeat)
                {
                    keys[(int)key] = value;

                    if (value && KeyDown != null)
                    {
                        args.Key = key;
                        KeyDown(this, args);
                    }
                    else if (!value && KeyUp != null)
                    {
                        args.Key = key;
                        KeyUp(this, args);
                    }
                }
            }
        }

        #region public bool KeyRepeat

        /// <summary>
        /// Gets or sets a System.Boolean indicating key repeat status.
        /// </summary>
        /// <remarks>
        /// If KeyRepeat is true, multiple KeyDown events will be generated while a key is being held.
        /// Otherwise only one KeyDown event will be reported.
        /// <para>
        /// The rate of the generated KeyDown events is controlled by the Operating System. Usually,
        /// one KeyDown event will be reported, followed by a small (250-1000ms) pause and several
        /// more KeyDown events (6-30 events per second).
        /// </para>
        /// <para>
        /// Set to true to handle text input (where keyboard repeat is desirable), but set to false
        /// for game input.
        /// </para>
        /// </remarks>
        public bool KeyRepeat
        {
            get { return repeat; }
            set { repeat = value; }
        }

        #endregion

        #region KeyDown

        /// <summary>
        /// Occurs when a key is pressed.
        /// </summary>
        public event EventHandler<KeyboardKeyEventArgs> KeyDown;

        #endregion

        #region KeyUp

        /// <summary>
        /// Occurs when a key is released.
        /// </summary>
        public event EventHandler<KeyboardKeyEventArgs> KeyUp;

        #endregion

        #endregion
        
        public InputDeviceType DeviceType {
            get { return InputDeviceType.Keyboard; }
        }

        #region --- Internal Methods ---

        #region internal void ClearKeys()

        internal void ClearKeys()
        {
            for (int i = 0; i < keys.Length; i++)
                if (this[(Key)i])       // Make sure KeyUp events are *not* raised for keys that are up, even if key repeat is on.
                    this[(Key)i] = false;
        }

        #endregion

        #endregion
    }
    
    public class KeyboardKeyEventArgs : EventArgs {

        /// <summary> Gets the <see cref="Key"/> that generated this event. </summary>
        public Key Key;
    }
}