using System;
using System.Drawing;
using System.Threading;
using ClassicalSharp;
using ClassicalSharp.Network;
using OpenTK;
using OpenTK.Graphics;
using System.Net;

namespace Launcher2 {

	public sealed class LauncherWindow {
		
		public NativeWindow Window;
		public IDrawer2D Drawer;
		public LauncherScreen screen;
		public bool Dirty;
		public ClassicubeSession Session = new ClassicubeSession();
		public AsyncDownloader Downloader;
		
		public int Width { get { return Window.Width; } }
		
		public int Height { get { return Window.Height; } }
		
		public Bitmap Framebuffer;
		Font logoFont, logoItalicFont;
		PlatformDrawer platformDrawer;
		public void Init() {
			Window.Resize += Resize;
			Window.FocusedChanged += FocusedChanged;
			Window.WindowStateChanged += Resize;
			logoFont = new Font( "Times New Roman", 28, FontStyle.Bold );
			logoItalicFont = new Font( "Times New Roman", 28, FontStyle.Italic );
			
			if( Configuration.RunningOnWindows )
				platformDrawer = new WinPlatformDrawer();
			else if( Configuration.RunningOnX11 )
				platformDrawer = new X11PlatformDrawer();
			else
				platformDrawer = new WinPlatformDrawer(); // TODO: mac osx support
		}

		void FocusedChanged( object sender, EventArgs e ) {
			MakeBackground();
			screen.Resize();
		}

		void Resize( object sender, EventArgs e ) {
			platformDrawer.Resize( Window.WindowInfo );
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
		
		public bool ConnectToServer( string hash ) {
			ClientStartData data = null;
			try {
				data = Session.GetConnectInfo( hash );
			} catch( WebException ex ) {
				ErrorHandler.LogError( "retrieving server information", ex );
				return false;
			}
			Client.Start( data, true );
			return true;
		}
		
		public void Run() {
			Window = new NativeWindow( 480, 480, Program.AppName, 0,
			                          GraphicsMode.Default, DisplayDevice.Default );
			Window.Visible = true;
			Drawer = new GdiPlusDrawerFont( null );
			Init();
			platformDrawer.Init( Window.WindowInfo );
			
			if( !ResourceFetcher.CheckAllResourcesExist() ) {
				SetScreen( new ResourcesScreen( this ) );
			} else {
				SetScreen( new MainScreen( this ) );
			}		
			
			while( true ) {
				Window.ProcessEvents();
				if( !Window.Exists ) break;
				
				screen.Tick();
				if( Dirty || screen.Dirty )
					Display();
				Thread.Sleep( 1 );
			}
		}
		
		void Display() {
			Dirty = false;
			screen.Dirty = false;
			platformDrawer.Draw( Window.WindowInfo, Framebuffer );
		}
		
		internal FastColour clearColour = new FastColour( 30, 30, 30 );
		public void MakeBackground() {
			if( Framebuffer != null )
				Framebuffer.Dispose();
			
			Framebuffer = new Bitmap( Width, Height );
			using( IDrawer2D drawer = Drawer ) {
				drawer.SetBitmap( Framebuffer );
				drawer.Clear( clearColour );
				
				DrawTextArgs args1 = new DrawTextArgs( "&eClassical", logoItalicFont, true );
				Size size1 = drawer.MeasureSize( ref args1 );
				DrawTextArgs args2 = new DrawTextArgs( "&eSharp", logoFont, true );
				Size size2 = drawer.MeasureSize( ref args2 );
					
				int xStart = Width / 2 - (size1.Width + size2.Width ) / 2;				
				drawer.DrawText( ref args1, xStart, 20 );
				drawer.DrawText( ref args2, xStart + size1.Width, 20 );
			}
			Dirty = true;
		}
		
		public void Dispose() {
			logoFont.Dispose();
			logoItalicFont.Dispose();
		}
	}
}
