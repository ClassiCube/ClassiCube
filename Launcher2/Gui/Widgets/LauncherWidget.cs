// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher {

	public abstract class LauncherWidget {
		
		public int X, Y, Width, Height;
		public LauncherWindow Window;
		public Action<int, int> OnClick;
		
		public string Text;
		
		public LauncherWidget( LauncherWindow window ) {
			Window = window;
		}
		
		protected void CalculateOffset( int x, int y, Anchor horAnchor, Anchor verAnchor ) {
			if( horAnchor == Anchor.LeftOrTop ) X = x;
			else if( horAnchor == Anchor.Centre ) X = x + Window.Width / 2 - Width / 2;
			else if( horAnchor == Anchor.BottomOrRight ) X = x + Window.Width - Width;
			
			if( verAnchor == Anchor.LeftOrTop ) Y = y;
			else if( verAnchor == Anchor.Centre ) Y = y + Window.Height / 2 - Height / 2;
			else if( verAnchor == Anchor.BottomOrRight ) Y = y + Window.Height - Height;
		}
		
		public abstract void Redraw( IDrawer2D drawer );
	}
}
