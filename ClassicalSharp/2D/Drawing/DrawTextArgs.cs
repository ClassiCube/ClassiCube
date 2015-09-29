using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public struct DrawTextArgs {
		
		public Color TextColour;
		public string Text;
		
		public bool UseShadow;
		internal bool SkipPartsCheck;
		
		public DrawTextArgs( string text, bool useShadow ) : 
			this( text, Color.White, useShadow ) { }
		
		public DrawTextArgs( string text, Color col, bool useShadow ) {
			Text = text;
			TextColour = col;
			UseShadow = useShadow;
			SkipPartsCheck = false;
		}		
	}
}
