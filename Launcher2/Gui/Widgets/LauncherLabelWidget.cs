// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	/// <summary> Represents text that cannot be modified by the user. </summary>
	public sealed class LauncherLabelWidget : LauncherWidget {
		
		Font font;
		public LauncherLabelWidget( LauncherWindow window, Font font ) : base( window ) {
			this.font = font;
		}
		
		public void SetDrawData( IDrawer2D drawer, string text ) {
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			Size size = drawer.MeasureSize( ref args );
			Width = size.Width; Height = size.Height;
			
			CalculatePosition();
			Text = text;
		}
		
		public override void Redraw( IDrawer2D drawer ) {
			if( Window.Minimised || !Visible ) return;
			DrawTextArgs args = new DrawTextArgs( Text, font, true );
			drawer.DrawText( ref args, X, Y );
		}
	}
}
