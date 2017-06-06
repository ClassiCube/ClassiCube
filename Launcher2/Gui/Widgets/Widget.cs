// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp;

namespace Launcher.Gui.Widgets {
	
	public delegate void WidgetClickHandler(int mouseX, int mouseY);
	
	/// <summary> Represents a graphical element/control. </summary>
	public abstract class Widget {
		
		public int X, Y, Width, Height;
		public LauncherWindow Window;
		
		/// <summary> The text associated with this widget. </summary>
		public string Text;
		
		/// <summary> Specifies the horizontal reference point for when the widget is resized. </summary>
		public Anchor HorizontalAnchor;
		
		/// <summary> Specifies the vertical reference point for when the widget is resized. </summary>
		public Anchor VerticalAnchor;
		
		/// <summary> Horizontal offset from the reference point in pixels. </summary>
		public int XOffset;
		
		/// <summary> Vertical offset from the reference point in pixels. </summary>
		public int YOffset;
		
		/// <summary> Whether this widget should be rendered and interactable with. </summary>
		public bool Visible = true;
		
		/// <summary> Whether this widget is the active widget selected by the user. </summary>
		public bool Active;
		
		/// <summary>Whether this widget can be selected via pressing tab. </summary>
		public bool TabSelectable;
		
		public WidgetClickHandler OnClick;
		
		public Widget(LauncherWindow window) {
			Window = window;
		}
		
		/// <summary> Redraws the contents of this widget. </summary>
		public abstract void Redraw(IDrawer2D drawer);
		

		/// <summary> Sets the reference points for when this widget is resized,
		/// and the offsets from the reference points (anchors) in pixels. </summary>
		/// <remarks> Updates the position of the widget. </remarks>
		public void SetLocation(Anchor horAnchor, Anchor verAnchor, int xOffset, int yOffset) {
			HorizontalAnchor = horAnchor; VerticalAnchor = verAnchor;
			XOffset = xOffset; YOffset = yOffset;
			CalculatePosition();
		}
		
		/// <summary> Calculates the position of this widget in the window,
		/// based on its anchor points and offset from the anchor points. </summary>
		public void CalculatePosition() {
			X = CalcPos(HorizontalAnchor, XOffset, Width, Window.Width);
			Y = CalcPos(VerticalAnchor, YOffset, Height, Window.Height);
		}
		
		static int CalcPos(Anchor anchor, int offset, int size, int axisLen) {
			if (anchor == Anchor.LeftOrTop) return offset;
			if (anchor == Anchor.BottomOrRight) return offset + axisLen - size;
			return offset + axisLen / 2 - size / 2;
		}
	}
}
