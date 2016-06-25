// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using OpenTK.Input;

namespace ClassicalSharp {	
	public enum Anchor { LeftOrTop, Centre, BottomOrRight, }	
}

namespace ClassicalSharp.Gui {
	
	public delegate void ClickHandler( Game g, Widget w, MouseButton btn );
	                                  
	public abstract class GuiElement : IDisposable {
		
		protected Game game;
		protected IGraphicsApi api;
		
		/// <summary> Object that represents any form of metadata attached to this widget. </summary>
		public object Metadata = null;
		
		public GuiElement( Game game ) {
			this.game = game;
			api = game.Graphics;
		}
		
		public abstract void Init();
		
		public abstract void Render( double delta );
		
		public abstract void Dispose();
		
		/// <summary> Causes the gui element to recreate all of its sub-elements and/or textures. </summary>
		/// <remarks> Typically used when bitmap font changes. </remarks>
		public virtual void Recreate() { 
			Dispose(); Init();
		}
		
		/// <summary> Called when the game window is resized. </summary>
		public abstract void OnResize( int oldWidth, int oldHeight, int width, int height );
		
		public virtual bool HandlesKeyDown( Key key ) { return false; }
		
		public virtual bool HandlesKeyPress( char key ) { return false; }
		
		public virtual bool HandlesKeyUp( Key key ) { return false; }
		
		public virtual bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) { return false; }
		
		public virtual bool HandlesMouseMove( int mouseX, int mouseY ) { return false; }
		
		public virtual bool HandlesMouseScroll( int delta ) { return false; }
		
		public virtual bool HandlesMouseUp( int mouseX, int mouseY, MouseButton button ) { return false; }
		
		protected static int CalcOffset( int axisSize, int elemSize, int offset, Anchor mode ) {
			if( mode == Anchor.LeftOrTop ) return offset;
			if( mode == Anchor.BottomOrRight) return axisSize - elemSize - offset;
			return (axisSize - elemSize) / 2 + offset;
		}
		
		protected bool IsValidInputChar( char c ) {
			if( c >= ' ' && c <= '~' ) return true; // ascii
			
			bool isCP437 = Utils.ControlCharReplacements.IndexOf( c ) >= 0 ||
				Utils.ExtendedCharReplacements.IndexOf( c ) >= 0;
			bool supportsCP437 = game.Network.ServerSupportsFullCP437;
			return supportsCP437 && isCP437;
		}
		
		protected static bool Contains( int recX, int recY, int width, int height, int x, int y ) {
			return x >= recX && y >= recY && x < recX + width && y < recY + height;
		}
		
		protected ClickHandler LeftOnly( Action<Game, Widget> action ) {
			if( action == null ) return (g, w, btn) => {};
			return (g, w, btn) => {
				if( btn != MouseButton.Left ) return;
				action( g, w );
			};
		}
	}
}
