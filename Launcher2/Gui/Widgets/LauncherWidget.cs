// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {

	/// <summary> Represents a graphical element/control. </summary>
	public abstract class LauncherWidget {
		
		public int X, Y, Width, Height;
		public LauncherWindow Window;
		public Action<int, int> OnClick;
		
		/// <summary> The text associated with this widget. </summary>
		public string Text;
		
		/// <summary> Specifies the horizontal reference point for when the widget is resized. </summary>
		public Anchor HorizontalAnchor;
		
		/// <summary> Specifies the vertical reference point for when the widget is resized. </summary>
		public Anchor VerticalAnchor;
		
		/// <summary> Horizontal offset from the reference point in pixels. </summary>
		public int XOffset = 0;
		
		/// <summary> Vertical offset from the reference point in pixels. </summary>
		public int YOffset = 0;
		
		
		public LauncherWidget( LauncherWindow window ) {
			Window = window;
		}
		
		/// <summary> Redraws the contents of this widget. </summary>
		public abstract void Redraw( IDrawer2D drawer );
		
		/// <summary> Sets the reference points for when this widget is resized. </summary>
		public LauncherWidget SetAnchors( Anchor horAnchor, Anchor verAnchor ) {
			HorizontalAnchor = horAnchor;
			VerticalAnchor = verAnchor;
			return this;
		}
		
		/// <summary> Sets the offsets from the reference points (anchors) in pixels. </summary>
		public LauncherWidget SetOffsets( int xOffset, int yOffset ) {
			XOffset = xOffset;
			YOffset = yOffset;
			return this;
		}
		
		/// <summary> Calculates the position of this widget in the window, 
		/// based on its anchor points and offset from the anchor points. </summary>
		public LauncherWidget CalculatePosition() {
			X = CalcPos( HorizontalAnchor, XOffset, Width, Window.Width );
			Y = CalcPos( VerticalAnchor, YOffset, Height, Window.Height );
			return this;
		}
		
		static int CalcPos( Anchor anchor, int offset, int size, int axisLen ) {
			if( anchor == Anchor.LeftOrTop ) 
				return offset;
			if( anchor == Anchor.Centre ) 
				return offset + axisLen / 2 - size / 2;
			if( anchor == Anchor.BottomOrRight ) 
				return offset + axisLen - size;
			return 0;
		}
	}
}
