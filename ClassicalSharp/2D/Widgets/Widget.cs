// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Widgets {	
	/// <summary> Represents an individual 2D gui component. </summary>
	public abstract class Widget : GuiElement {
		
		public Widget(Game game) : base(game) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.LeftOrTop;
		}
		
		/// <summary> Whether this widget is currently being moused over. </summary>
		public bool Active;
		
		/// <summary> Whether this widget is prevented from being interacted with. </summary>
		public bool Disabled;
		
		/// <summary> Invoked when this widget is clicked on. Can be null. </summary>
		public ClickHandler OnClick;
		
		/// <summary> Horizontal coordinate of top left corner in pixels. </summary>
		public int X;
		
		/// <summary> Vertical coordinate of top left corner in pixels. </summary>
		public int Y;
		
		/// <summary> Horizontal length of widget's bounds in pixels. </summary>
		public int Width;
		
		/// <summary> Vertical length of widget's bounds in pixels. </summary>
		public int Height;
		
		/// <summary> Specifies the horizontal reference point for when the widget is resized. </summary>
		public Anchor HorizontalAnchor;
		
		/// <summary> Specifies the vertical reference point for when the widget is resized. </summary>
		public Anchor VerticalAnchor;
		
		/// <summary> Horizontal offset from the reference point in pixels. </summary>
		public int XOffset;
		
		/// <summary> Vertical offset from the reference point in pixels. </summary>
		public int YOffset;
		
		/// <summary> Specifies the boundaries of the widget in pixels. </summary>
		public Rectangle Bounds { get { return new Rectangle(X, Y, Width, Height); } }
		
		public virtual void Reposition() {
			X = CalcPos(HorizontalAnchor, XOffset, Width, game.Width);
			Y = CalcPos(VerticalAnchor, YOffset, Height, game.Height);
		}
	}
}
