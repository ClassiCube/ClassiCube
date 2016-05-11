// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp {
	
	/// <summary> Integer constants for the six tile sides of a block. </summary>
	public static class Side {
		/// <summary> Face X = 0. </summary>
		public const int Left = 0;
		/// <summary> Face X = 1. </summary>
		public const int Right = 1;
		/// <summary> Face Z = 0. </summary>
		public const int Front = 2;
		/// <summary> Face Z = 1. </summary>
		public const int Back = 3;
		/// <summary> Face Y = 0. </summary>
		public const int Bottom = 4;
		/// <summary> Face Y = 1. </summary>
		public const int Top = 5;
		/// <summary> Number of sides on a cube. </summary>
		public const int Sides = 6;
		
		public static int Opposite( int side ) {
			switch( side ) {
				case Left: return Right;
				case Right: return Left;
				case Front: return Back;
				case Back: return Front;
				case Bottom: return Top;
				case Top: return Bottom;
			}
			return side;
		}
	}
}