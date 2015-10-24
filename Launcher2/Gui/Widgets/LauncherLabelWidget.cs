using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher2 {

	/// <summary> Widget that represents text that cannot be modified by the user. </summary>
	public sealed class LauncherLabelWidget : LauncherWidget {
		
		public string Text;
		
		public LauncherLabelWidget( LauncherWindow window, string text ) : base( window ) {
			Text = text;
		}

		public void DrawAt( IDrawer2D drawer, string text, Font font,
		                   Anchor horAnchor, Anchor verAnchor, int x, int y ) {
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = size.Width; Height = size.Height;
			
			CalculateOffset( x, y, horAnchor, verAnchor );
			Redraw( drawer, text, font );
		}
		
		public void Redraw( IDrawer2D drawer, string text, Font font ) {
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = size.Width; Height = size.Height;
			
			args.SkipPartsCheck = true;
			drawer.DrawText( ref args, X, Y );
			Text = text;
		}
	}
}
