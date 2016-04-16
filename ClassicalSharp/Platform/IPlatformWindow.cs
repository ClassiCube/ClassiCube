// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Input;
using OpenTK.Platform;
using Clipboard = System.Windows.Forms.Clipboard;

namespace ClassicalSharp {
	
	/// <summary> Abstracts away a platform specific window, and input handler mechanism. </summary>
	public interface IPlatformWindow {
		
		int Width { get; }
		
		int Height { get; }
		
		Size ClientSize { get; }
		
		bool VSync { get; set; }
		
		bool Exists { get; }
		
		bool Focused { get; }
		
		bool CursorVisible { get; set; }
		
		Point DesktopCursorPos { get; set; }
		
		MouseDevice Mouse { get; }
		
		KeyboardDevice Keyboard { get; }
		
		Icon Icon { get; set; }
		
		Point PointToScreen( Point coords );
		
		WindowState WindowState { get; set; }
		
		IWindowInfo WindowInfo { get; }
		
		string ClipboardText { get; set; }
		
		void Run();
		
		void SwapBuffers();
		
		void Exit();
		
		event EventHandler<KeyPressEventArgs> KeyPress;
	}
	
	/// <summary> Implementation of a native window and native input handling mechanism on Windows, OSX, and Linux. </summary>
	public sealed class DesktopWindow : GameWindow, IPlatformWindow {
		
		Game game;
		public DesktopWindow( Game game, string username, bool nullContext, int width, int height ) :
			base( width, height, GraphicsMode.Default, Program.AppName + " (" + username + ")", nullContext, 0, DisplayDevice.Default ) {
			this.game = game;
		}
		
		protected override void OnLoad( EventArgs e ) {
			game.OnLoad();
			base.OnLoad( e );
		}
		
		public override void Dispose() {
			game.Dispose();
			base.Dispose();
		}
		
		protected override void OnRenderFrame( FrameEventArgs e ) {
			game.RenderFrame( e.Time );
			base.OnRenderFrame( e );
		}
		
		protected override void OnResize( object sender, EventArgs e ) {
			game.OnResize();
			base.OnResize( sender, e );
		}
		
		// TODO: retry when clipboard returns null.
		public string ClipboardText {
			get {
				if ( OpenTK.Configuration.RunningOnMacOS )
					return GetClipboardText();
				else
					return Clipboard.GetText();				
			}
			set {
				if ( OpenTK.Configuration.RunningOnMacOS )
					SetClipboardText( value );
				else
					Clipboard.SetText( value );
			}
		}
	}
}
