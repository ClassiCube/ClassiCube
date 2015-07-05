using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public struct DrawTextArgs {
		
		public Color TextColour;
		public string Text;
		
		public bool UseShadow;
		public Color ShadowColour;
		public IGraphicsApi Graphics;
		
		public DrawTextArgs( IGraphicsApi graphics, string text, Color col, bool useShadow ) : 
			this( graphics, text, col, useShadow, Color.Black ) {
		}
		
		public DrawTextArgs( IGraphicsApi graphics, string text, bool useShadow ) : 
			this( graphics, text, Color.White, useShadow, Color.Black ) {
		}
		
		public DrawTextArgs( IGraphicsApi graphics, string text, Color col, bool useShadow, Color shadowCol ) {
			Graphics = graphics;
			Text = text;
			TextColour = col;
			UseShadow = useShadow;
			ShadowColour = shadowCol;
		}		
	}
}