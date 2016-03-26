// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher {

	/// <summary> Widget that represents text can have modified by the user. </summary>
	public sealed class LauncherBooleanWidget : LauncherWidget {
		
		public int BoxWidth, BoxHeight;
		public bool Value;
		Font font;
		
		public LauncherBooleanWidget( LauncherWindow window, Font font, int width, int height ) : base( window ) {
			BoxWidth = width; BoxHeight = height;
			Width = width; Height = height;
			this.font = font;
		}

		public void SetDrawData( IDrawer2D drawer, Anchor horAnchor, Anchor verAnchor, int x, int y ) {		
			CalculateOffset( x, y, horAnchor, verAnchor );
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			if( Window.Minimised ) return;
			drawer.DrawRect( FastColour.Black, X, Y, Width, Height );
			if( Value ) {
				DrawTextArgs args = new DrawTextArgs( "X", font, false );
				Size size = drawer.MeasureSize( ref args );
				args.SkipPartsCheck = true;
				drawer.DrawText( ref args, X + (Width + 2 - size.Width) / 2, // account for border
				                Y + (Height - size.Height) / 2 );
			}
			drawer.DrawRectBounds( FastColour.White, 2, X, Y, Width, Height );
		}
	}
}
