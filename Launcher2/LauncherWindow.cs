using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Threading;
using ClassicalSharp;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Platform.Windows;

namespace Launcher2 {

	public sealed class LauncherWindow {
		
		public NativeWindow Window;
		public IDrawer2D Drawer;
		public LauncherScreen screen;
		public bool Dirty;
		
		public int Width { get { return Window.Width; } }
		
		public int Height { get { return Window.Height; } }
		
		public Bitmap Framebuffer;
		Font logoFont, logoItalicFont;
		public void Init() {
			Window.Resize += Resize;
			Window.FocusedChanged += FocusedChanged;
			logoFont = new Font( "Times New Roman", 28, FontStyle.Bold );
			logoItalicFont = new Font( "Times New Roman", 28, FontStyle.Italic );
		}

		void FocusedChanged( object sender, EventArgs e ) {
			MakeBackground();
			screen.Resize();
		}

		void Resize( object sender, EventArgs e ) {
			MakeBackground();
			screen.Resize();
		}
		
		public void SetScreen( LauncherScreen screen ) {
			if( this.screen != null )
				this.screen.Dispose();
			
			MakeBackground();
			this.screen = screen;
			screen.Init();
		}
		
		public void Run() {
			Window = new NativeWindow( 480, 480, Program.AppName, 0,
			                          GraphicsMode.Default, DisplayDevice.Default );
			Window.Visible = true;
			screen = new MainScreen( this );
			Drawer = new GdiPlusDrawer2D( null );
			Init();
			SetScreen( new MainScreen( this ) );
			
			while( true ) {
				Window.ProcessEvents();
				if( !Window.Exists ) break;
				
				if( Dirty || (screen != null && screen.Dirty) )
					Display();
				Thread.Sleep( 1 );
			}
		}
		
		void Display() {
			Dirty = false;
			if( screen != null ) 
				screen.Dirty = false;
			
			WinWindowInfo info = (WinWindowInfo)Window.WindowInfo;
			IntPtr dc = info.DeviceContext;
			
			using( Graphics g = Graphics.FromHdc( dc ) )
				g.DrawImage( Framebuffer, 0, 0, Framebuffer.Width, Framebuffer.Height );
		}
		
		internal static FastColour clearColour = new FastColour( 30, 30, 30 );
		public void MakeBackground() {
			if( Framebuffer != null )
				Framebuffer.Dispose();
			
			Framebuffer = new Bitmap( Width, Height );
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );
				drawer.Clear( clearColour );
				
				Size size1 = drawer.MeasureSize( "&eClassical", logoItalicFont, true );
				Size size2 = drawer.MeasureSize( "&eSharp", logoFont, true );
				int xStart = Width / 2 - (size1.Width + size2.Width ) / 2;
				
				DrawTextArgs args = new DrawTextArgs( "&eClassical", true );
				drawer.DrawText( logoItalicFont, ref args, xStart, 20 );
				args.Text = "&eSharp";
				drawer.DrawText( logoFont, ref args, xStart + size1.Width, 20 );
			}
			Dirty = true;
		}
		
		public void Dispose() {
			logoFont.Dispose();
			logoItalicFont.Dispose();
		}
	}
}
