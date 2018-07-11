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
using System.Drawing;

namespace OpenTK.Input {
	
	/// <summary> Represents a mouse device and provides methods to query its status. </summary>
	public static class Mouse {
		static readonly bool[] states = new bool[(int)MouseButton.LastButton];
		static MouseMoveEventArgs move_args = new MouseMoveEventArgs();
		static MouseButtonEventArgs button_args = new MouseButtonEventArgs();
		static MouseWheelEventArgs wheel_args = new MouseWheelEventArgs();

		/// <summary> Gets the absolute wheel position in floating-point units. </summary>
		// /// <summary> Gets the precise value of the wheel in floating-point units. </summary>
		public static float Wheel;
		
		internal static void SetWheel(float value) {
			wheel_args.Delta = value - Wheel;
			Wheel = value;
			if (WheelChanged != null) WheelChanged(null, wheel_args);
		}

		/// <summary> Gets an integer representing the absolute x position of the pointer, in window pixel coordinates. </summary>
		public static int X;
		/// <summary> Gets an integer representing the absolute y position of the pointer, in window pixel coordinates. </summary>
		public static int Y;

		public static bool Get(MouseButton btn) { return states[(int)btn]; }
		internal static void Set(MouseButton btn, bool value) {
			if (value == states[(int)btn]) return;
			states[(int)btn] = value;
			button_args.Button = btn;
			
			if (value && ButtonDown != null) {
				ButtonDown(null, button_args);
			} else if (!value && ButtonUp != null) {
				ButtonUp(null, button_args);
			}
		}

		internal static void SetPos(int x, int y) {
			move_args.XDelta = x - X;
			move_args.YDelta = y - Y;
			X = x; Y = y;
			
			if (Move != null) Move(null, move_args);
		}

		public static event EventHandler<MouseMoveEventArgs> Move;
		public static event EventHandler<MouseButtonEventArgs> ButtonDown;
		public static event EventHandler<MouseButtonEventArgs> ButtonUp;
		public static event EventHandler<MouseWheelEventArgs> WheelChanged;
	}

	public class MouseMoveEventArgs : EventArgs {
		/// <summary> Gets the change in X position produced by this event. </summary>
		public int XDelta;
		/// <summary> Gets the change in Y position produced by this event. </summary>
		public int YDelta;
	}

	public class MouseButtonEventArgs : EventArgs {
		/// <summary> The mouse button for the event. </summary>
		public MouseButton Button;
	}

	public class MouseWheelEventArgs : EventArgs {
		/// <summary> Gets the precise change in value of the wheel for this event in floating-point units. </summary>
		public float Delta;
	}
}
