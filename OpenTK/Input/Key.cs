 #region License
 //
 // The Open Toolkit Library License
 //
 // Copyright (c) 2006 - 2009 the Open Toolkit library.
 //
 // Permission is hereby granted, free of charge, to any person obtaining a copy
 // of this software and associated documentation files (the "Software"), to deal
 // in the Software without restriction, including without limitation the rights to 
 // use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 // the Software, and to permit persons to whom the Software is furnished to do
 // so, subject to the following conditions:
 //
 // The above copyright notice and this permission notice shall be included in all
 // copies or substantial portions of the Software.
 //
 // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 // EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 // OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 // NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 // HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 // WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 // FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 // OTHER DEALINGS IN THE SOFTWARE.
 //
 #endregion

namespace OpenTK.Input {
    /// <summary> The available keyboard keys. </summary>
    public enum Key : int {
        // Key outside the known keys
        Unknown = 0,
    
        // Modifiers
        ShiftLeft, ShiftRight, ControlLeft, ControlRight,
        AltLeft, AltRight, WinLeft, WinRight, Menu,
    
        // Function keys (hopefully enough for most keyboards - mine has 26)
        // <keysymdef.h> on X11 reports up to 35 function keys.
        F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10,
        F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
        F21, F22, F23, F24, F25, F26, F27, F28, F29, F30,
        F31, F32, F33, F34, F35,
    
        // Direction arrows
        Up, Down, Left, Right,
    
        // Action keys
        Enter, Escape, Space, Tab, BackSpace, Insert,
        Delete, PageUp, PageDown, Home, End, CapsLock,
        ScrollLock, PrintScreen, Pause, NumLock,
    
        // Keypad keys
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
        Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDivide, KeypadMultiply, KeypadSubtract,
        KeypadAdd, KeypadDecimal, KeypadEnter,
    
        // Letters
        A, B, C, D, E, F, G, H, I, J, 
        K, L, M, N, O, P, Q, R, S, T, 
        U, V, W, X, Y, Z,
    
        // Numbers
        Number0, Number1, Number2, Number3, Number4,
        Number5, Number6, Number7, Number8, Number9,
    
        // Symbols
        Tilde, Minus, Plus, BracketLeft, BracketRight,
        Semicolon, Quote, Comma, Period, Slash, BackSlash,
        
        // Last available keyboard key
        LastKey
    }
}