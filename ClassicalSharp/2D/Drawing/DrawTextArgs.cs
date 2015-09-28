using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public struct DrawTextArgs {
		
		public Color TextColour;
		public string Text;
		
		public bool UseShadow;
		public Color ShadowColour;
		internal bool SkipPartsCheck;
		
		public DrawTextArgs( string text, Color col, bool useShadow ) : 
			this( text, col, useShadow, Color.Black ) { }
		
		public DrawTextArgs( string text, bool useShadow ) : 
			this( text, Color.White, useShadow, Color.Black ) { }
		
		public DrawTextArgs( string text, Color col, bool useShadow, Color shadowCol ) {
			Text = text;
			TextColour = col;
			UseShadow = useShadow;
			ShadowColour = shadowCol;
			SkipPartsCheck = false;
		}		
	}
}
