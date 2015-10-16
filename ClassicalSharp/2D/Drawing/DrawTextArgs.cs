using System;
using System.Drawing;

namespace ClassicalSharp {
	
	public struct DrawTextArgs {
		
		public string Text;
		
		public bool UseShadow;
		public bool SkipPartsCheck;
		
		public DrawTextArgs( string text, bool useShadow ) {
			Text = text;
			UseShadow = useShadow;
			SkipPartsCheck = false;
		}		
	}
}
