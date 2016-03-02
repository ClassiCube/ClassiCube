using System;

namespace ClassicalSharp {
	
	/// <summary> Represents a container of widgets and other 2D elements. </summary>
	/// <remarks> May cover the entire game window. </remarks>
	public abstract class Screen : GuiElement {
		
		public Screen( Game game ) : base( game ) {
		}
		
		/// <summary> Whether this screen handles all mouse and keyboard input. </summary>
		/// <remarks> This prevents the client from interacting with the world. </remarks>
		public virtual bool HandlesAllInput { get; protected set; }
		
		/// <summary> Whether this screen completely and opaquely covers the game world behind it. </summary>
		public virtual bool BlocksWorld { get { return false; } }
		
		/// <summary> Whether this screen hides the normal in-game hud. </summary>
		public virtual bool HidesHud { get { return false; } }
		
		/// <summary> Whether the normal in-game hud should be drawn over the top of this screen. </summary>
		public virtual bool RenderHudAfter { get { return false; } }
		
		/// <summary> Causes the screen to recreate all of its contained widgets. </summary>
		/// <remarks> Typically used when bitmap font changes. </remarks>
		public virtual void Recreate() {
			Dispose();
			Init();
		}
	}
}
