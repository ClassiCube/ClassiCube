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
	public sealed class MouseDevice {
		
		readonly bool[] button_state = new bool[(int)MouseButton.LastButton];
		float wheel, last_wheel;
		Point pos, last_pos;
		MouseMoveEventArgs move_args = new MouseMoveEventArgs();
		MouseButtonEventArgs button_args = new MouseButtonEventArgs();
		MouseWheelEventArgs wheel_args = new MouseWheelEventArgs();

		/// <summary> Gets the absolute wheel position in integer units.
		/// To support high-precision mice, it is recommended to use <see cref="WheelPrecise"/> instead. </summary>
		public int Wheel {
			get { return (int)Math.Round(wheel, MidpointRounding.AwayFromZero); }
			internal set { WheelPrecise = value; }
		}

		/// <summary> Gets the absolute wheel position in floating-point units. </summary>
		public float WheelPrecise {
			get { return wheel; }
			internal set {
				wheel = value;

				wheel_args.X = pos.X;
				wheel_args.Y = pos.Y;
				wheel_args.ValuePrecise = wheel;
				wheel_args.DeltaPrecise = wheel - last_wheel;

				WheelChanged(this, wheel_args);

				last_wheel = wheel;
			}
		}

		/// <summary> Gets an integer representing the absolute x position of the pointer, in window pixel coordinates. </summary>
		public int X {
			get { return pos.X; }
		}

		/// <summary> Gets an integer representing the absolute y position of the pointer, in window pixel coordinates. </summary>
		public int Y {
			get { return pos.Y; }
		}

		/// <summary> Gets a System.Boolean indicating the state of the specified MouseButton. </summary>
		/// <param name="button">The MouseButton to check.</param>
		/// <returns>True if the MouseButton is pressed, false otherwise.</returns>
		public bool this[MouseButton button] {
			get { return button_state[(int)button]; }
			internal set {
				bool previous_state = button_state[(int)button];
				button_state[(int)button] = value;

				button_args.X = pos.X;
				button_args.Y = pos.Y;
				button_args.Button = button;
				button_args.IsPressed = value;
				if (value && !previous_state)
					ButtonDown(this, button_args);
				else if (!value && previous_state)
					ButtonUp(this, button_args);
			}
		}

		/// <summary> Sets a System.Drawing.Point representing the absolute position of the pointer, in window pixel coordinates. </summary>
		internal Point Position {
			set {
				pos = value;
				move_args.X = pos.X;
				move_args.Y = pos.Y;
				move_args.XDelta = pos.X - last_pos.X;
				move_args.YDelta = pos.Y - last_pos.Y;
				Move(this, move_args);
				last_pos = pos;
			}
		}

		/// <summary> Occurs when the mouse's position is moved. </summary>
		public event EventHandler<MouseMoveEventArgs> Move = delegate { };

		/// <summary> Occurs when a button is pressed. </summary>
		public event EventHandler<MouseButtonEventArgs> ButtonDown = delegate { };

		/// <summary> Occurs when a button is released. </summary>
		public event EventHandler<MouseButtonEventArgs> ButtonUp = delegate { };

		/// <summary> Occurs when one of the mouse wheels is moved. </summary>
		public event EventHandler<MouseWheelEventArgs> WheelChanged = delegate { };
	}

	public class MouseEventArgs : EventArgs {

		/// <summary> Gets the X position of the mouse for the event. </summary>
		public int X;

		/// <summary>Gets the Y position of the mouse for the event. </summary>
		public int Y;
	}

	public class MouseMoveEventArgs : MouseEventArgs {

		/// <summary> Gets the change in X position produced by this event. </summary>
		public int XDelta;

		/// <summary> Gets the change in Y position produced by this event. </summary>
		public int YDelta;
	}

	public class MouseButtonEventArgs : MouseEventArgs {
		
		/// <summary> The mouse button for the event. </summary>
		public MouseButton Button;

		/// <summary> Gets a System.Boolean representing the state of the mouse button for the event. </summary>
		public bool IsPressed;
	}

	public class MouseWheelEventArgs : MouseEventArgs {

		/// <summary> Gets the value of the wheel in integer units.
		/// To support high-precision mice, it is recommended to use <see cref="ValuePrecise"/> instead. </summary>
		public int Value { get { return (int)Math.Round(ValuePrecise, MidpointRounding.AwayFromZero); } }

		/// <summary> Gets the change in value of the wheel for this event in integer units.
		/// To support high-precision mice, it is recommended to use <see cref="DeltaPrecise"/> instead. </summary>
		public int Delta { get { return (int)Math.Round(DeltaPrecise, MidpointRounding.AwayFromZero); } }

		/// <summary> Gets the precise value of the wheel in floating-point units. </summary>
		public float ValuePrecise;

		/// <summary> Gets the precise change in value of the wheel for this event in floating-point units. </summary>
		public float DeltaPrecise;
	}
}
