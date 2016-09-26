// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	/// <summary> Represents text that cannot be modified by the user. </summary>
	public sealed class LabelWidget : Widget {
		
		/// <summary> Whether text should be drawn in a darker manner when this widget is not active. </summary>
		public bool DarkenWhenInactive;
		
		Font font;
		public LabelWidget( LauncherWindow window, Font font ) : base( window ) {
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
			string text = Text;
			if( DarkenWhenInactive && !Active )
				text = "&7" + text;
			
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			drawer.DrawText( ref args, X, Y );
		}
	}
}
