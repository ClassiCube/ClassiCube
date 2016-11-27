// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;

namespace ClassicalSharp {
	
	/// <summary> Contains arguments for measuring or drawing text. </summary>
	public struct DrawTextArgs {
		
		public string Text;
		public Font Font;
		public bool UseShadow;
		public bool SkipPartsCheck;
		
		public DrawTextArgs(string text, Font font, bool useShadow) {
			Text = text;
			Font = font;
			UseShadow = useShadow;
			SkipPartsCheck = false;
		}		
	}
}
