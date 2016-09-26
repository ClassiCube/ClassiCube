// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	/// <summary> Represents a state that can be toggled by the user. </summary>
	public sealed class CheckboxWidget : Widget {
		
		public int BoxWidth, BoxHeight;
		public bool Value;
		Font font;
		
		public CheckboxWidget( LauncherWindow window, Font font, int width, int height ) : base( window ) {
			BoxWidth = width; BoxHeight = height;
			Width = width; Height = height;
			this.font = font;
		}

		public override void Redraw( IDrawer2D drawer ) {
			if( Window.Minimised || !Visible ) return;
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
