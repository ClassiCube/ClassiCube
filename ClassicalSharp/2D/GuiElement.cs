// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {	
	public enum Anchor { LeftOrTop, Centre, BottomOrRight, }	
}

namespace ClassicalSharp.Gui {
	public delegate void ClickHandler( Game g, Widget w, MouseButton btn, int mouseX, int mouseY );
	
	public abstract class GuiElement : IDisposable {
		
		protected Game game;
		protected IGraphicsApi gfx;
		
		/// <summary> Object that represents any form of metadata attached to this widget. </summary>
		public object Metadata = null;
		
		public GuiElement( Game game ) {
			this.game = game;
			gfx = game.Graphics;
		}
		
		public abstract void Init();
		
		public abstract void Render( double delta );
		
		public abstract void Dispose();
		
		/// <summary> Causes the gui element to recreate all of its sub-elements and/or textures. </summary>
		/// <remarks> Typically used when bitmap font changes. </remarks>
		public void Recreate() { Dispose(); Init(); }
		
		public virtual bool HandlesKeyDown( Key key ) { return false; }
		
		public virtual bool HandlesKeyPress( char key ) { return false; }
		
		public virtual bool HandlesKeyUp( Key key ) { return false; }
		
		public virtual bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) { return false; }
		
		public virtual bool HandlesMouseMove( int mouseX, int mouseY ) { return false; }
		
		public virtual bool HandlesMouseScroll( int delta ) { return false; }
		
		public virtual bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) { return false; }
		
		protected static int CalcPos( Anchor anchor, int offset, int size, int axisLen ) {
			if( anchor == Anchor.LeftOrTop ) return offset;
			if( anchor == Anchor.BottomOrRight ) return axisLen - size - offset;
			return (axisLen - size) / 2 + offset;
		}
		
		public static bool Contains( int recX, int recY, int width, int height, int x, int y ) {
			return x >= recX && y >= recY && x < recX + width && y < recY + height;
		}
	}
}
