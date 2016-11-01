// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	/// <summary> Represents a container of widgets and other 2D elements. </summary>
	/// <remarks> May cover the entire game window. </remarks>
	public abstract class Screen : GuiElement {
		
		public Screen( Game game ) : base( game ) {
		}
		
		bool handlesAll;
		/// <summary> Whether this screen handles all mouse and keyboard input. </summary>
		/// <remarks> This prevents the client from interacting with the world. </remarks>
		public virtual bool HandlesAllInput { get { return handlesAll; } protected set { handlesAll = value; } }
		
		/// <summary> Whether this screen completely and opaquely covers the game world behind it. </summary>
		public virtual bool BlocksWorld { get { return false; } }
		
		/// <summary> Whether this screen hides the normal in-game hud. </summary>
		public virtual bool HidesHud { get { return false; } }
		
		/// <summary> Whether the normal in-game hud should be drawn over the top of this screen. </summary>
		public virtual bool RenderHudAfter { get { return false; } }

		/// <summary> Called when the game window is resized. </summary>
		public abstract void OnResize( int width, int height );		
		
		protected ClickHandler LeftOnly( Action<Game, Widget> action ) {
			if( action == null ) return (g, w, btn, x, y) => {};
			return (g, w, btn, x, y) => {
				if( btn != MouseButton.Left ) return;
				action( g, w );
			};
		}
	}
}
