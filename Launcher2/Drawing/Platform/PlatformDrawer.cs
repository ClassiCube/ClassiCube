// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Platform;

namespace Launcher.Drawing {	
	/// <summary> Per-platform class used to transfer a framebuffer directly to the native window. </summary>
	public abstract class PlatformDrawer {
		
		/// <summary> Data describing the native window. </summary>
		internal IWindowInfo info;
		
		/// <summary> Initialises the variables for this platform drawer. </summary>
		public abstract void Init();
		
		/// <summary> Creates a framebuffer bitmap of the given dimensions. </summary>
		public virtual Bitmap CreateFrameBuffer(int width, int height) { 
			return new Bitmap(width, height);
		}
		
		/// <summary> Updates the variables when the native window changes dimensions. </summary>
		public abstract void Resize();
		
		/// <summary> Redraws a portion of the framebuffer to the window. </summary>
		/// <remarks> r is only a hint, the entire framebuffer may still be
		/// redrawn on some platforms. </remarks>
		public abstract void Redraw(Bitmap framebuffer, Rectangle r);
	}
}
