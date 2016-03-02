using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	/// <summary> Represents an individual 2D gui component. </summary>
	public abstract class Widget : GuiElement {
		
		public Widget( Game game ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.LeftOrTop;
		}
		
		/// <summary> Whether this widget is currently being moused over. </summary>
		public bool Active;
		
		/// <summary> Whether this widget is prevented from being interacted with. </summary>
		public bool Disabled;
		
		/// <summary> Invoked when this widget is clicked on. Can be null. </summary>
		public ClickHandler OnClick;
		
		/// <summary> Horizontal coordinate of top left corner in window space. </summary>
		public int X;
		
		/// <summary> Vertical coordinate of top left corner in window space. </summary>
		public int Y;
		
		/// <summary> Horizontal length of widget's bounds in window space. </summary>
		public int Width;
		
		/// <summary> Vertical length of widget's bounds in window space. </summary>
		public int Height;
		
		/// <summary> Specifies the horizontal reference point for when the widget is resized. </summary>
		public Anchor HorizontalAnchor;
		
		/// <summary> Specifies the vertical reference point for when the widget is resized. </summary>
		public Anchor VerticalAnchor;
		
		/// <summary> Width and height of widget in window space. </summary>
		public Size Size { get { return new Size( Width, Height ); } }
		
		/// <summary> Coordinate of top left corner of widget's bounds in window space. </summary>
		public Point TopLeft { get { return new Point( X, Y ); } }
		
		/// <summary> Coordinate of bottom right corner of widget's bounds in window space. </summary>
		public Point BottomRight { get { return new Point( X + Width, Y + Height ); } }
		
		/// <summary> Specifies the boundaries of the widget in window space. </summary>
		public Rectangle Bounds { get { return new Rectangle( X, Y, Width, Height ); } }
		
		/// <summary> Moves the widget to the specified window space coordinates. </summary>
		public virtual void MoveTo( int newX, int newY ) {
			X = newX; Y = newY;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			int deltaX = CalcDelta( width, oldWidth, HorizontalAnchor );
			int deltaY = CalcDelta( height, oldHeight, VerticalAnchor );
			MoveTo( X + deltaX, Y + deltaY );
		}
	}
}
