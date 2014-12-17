using System;
using System.Drawing;
using ClassicalSharp.GraphicsAPI;

namespace ClassicalSharp {

	public class DrawTextArgs {
		
		public float XOffset, YOffset;
		
		public Color TextColour;
		public string Text;
		
		public bool UseShadow;
		public Color ShadowColour;
		public IGraphicsApi Graphics;
		
		public DrawTextArgs( IGraphicsApi graphics, string text, Color col, bool useShadow ) : 
			this( graphics, text, col, useShadow, Color.Black, 0f, 0f ) {
		}
		
		public DrawTextArgs( IGraphicsApi graphics, string text, Color col, bool useShadow, Color shadowCol ) : 
			this( graphics, text, col, useShadow, shadowCol, 0f, 0f ) {
		}
		
		public DrawTextArgs( IGraphicsApi graphics, string text, Color col, 
		                    bool useShadow, Color shadowCol, float x, float y ) {
			Graphics = graphics;
			Text = text;
			TextColour = col;
			UseShadow = useShadow;
			ShadowColour = shadowCol;
			XOffset = x;
			YOffset = y;
		}		
	}
}