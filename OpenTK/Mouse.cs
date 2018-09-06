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

using System;

namespace OpenTK.Input {
	
	public enum MouseButton {
        Left, Right, Middle, 
        Count,
    }	
	public delegate void MouseEventFunc(MouseButton btn);
	public delegate void MouseWheelEventFunc(float delta);
	public delegate void MouseMoveEventFunc(int xDelta, int yDelta);
	
	public static class Mouse {
		static readonly bool[] states = new bool[(int)MouseButton.Count];
		public static float Wheel;
		public static int X, Y;

		public static bool Get(MouseButton btn) { return states[(int)btn]; }
		internal static void Set(MouseButton btn, bool value) {
			if (value == states[(int)btn]) return;
			states[(int)btn] = value;
			
			if (value && ButtonDown != null) {
				ButtonDown(btn);
			} else if (!value && ButtonUp != null) {
				ButtonUp(btn);
			}
		}
		
		internal static void SetWheel(float value) {
			float delta = value - Wheel;
			Wheel = value;
			if (WheelChanged != null) WheelChanged(delta);
		}

		internal static void SetPos(int x, int y) {
			int xDelta = x - X, yDelta = y - Y;
			X = x; Y = y;			
			if (Move != null) Move(xDelta, yDelta);
		}

		public static event MouseMoveEventFunc Move;
		public static event MouseEventFunc ButtonDown;
		public static event MouseEventFunc ButtonUp;
		public static event MouseWheelEventFunc WheelChanged;
	}
}
