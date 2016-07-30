// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Net;
using System.Reflection;
using System.Threading;
using ClassicalSharp;
using ClassicalSharp.Network;
using Launcher.Gui.Screens;
using Launcher.Patcher;
using Launcher.Web;
using OpenTK;
using OpenTK.Graphics;
using OpenTK.Input;

namespace Launcher {

	public sealed partial class LauncherWindow {
		
		/// <summary> Underlying native window instance. </summary>
		public NativeWindow Window;
		
		/// <summary> Platform specific class used to draw 2D elements,
		/// such as text, rounded rectangles and lines. </summary>
		public IDrawer2D Drawer;
		
		/// <summary> Currently active screen. </summary>
		public LauncherScreen Screen;
		
		/// <summary> Whether the client drawing area needs to be redrawn/presented to the screen. </summary>
		public bool Dirty;
		
		/// <summary> The specific area/region of the window that needs to be redrawn. </summary>
		public Rectangle DirtyArea;
		
		/// <summary> Currently active logged in session with classicube.net. </summary>
		public ClassicubeSession Session = new ClassicubeSession();
		
		/// <summary> Queue used to download resources asynchronously. </summary>
		public AsyncDownloader Downloader;
		
		/// <summary> Returns the width of the client drawing area. </summary>
		public int Width { get { return Window.Width; } }
		
		/// <summary> Returns the height of the client drawing area. </summary>
		public int Height { get { return Window.Height; } }
		
		/// <summary> Bitmap that contains the entire array of pixels that describe the client drawing area. </summary>
		public Bitmap Framebuffer;
		
		/// <summary> Whether at the next tick, the launcher window should proceed to stop displaying frames and subsequently exit. </summary>
		public bool ShouldExit;
		public bool ShouldUpdate;
		
		public string FontName = "Arial";
		
		public bool Minimised {
			get { return Window.WindowState == WindowState.Minimized || (Width == 1 && Height == 1); }
		}
		
		/// <summary> Contains metadata attached for different screen instances,
		/// typically used to save 'last text entered' text when a screen is disposed. </summary>
		public Dictionary<string, Dictionary<string, object>> ScreenMetadata =
			new Dictionary<string, Dictionary<string, object>>();
		
		internal ResourceFetcher fetcher;
		internal UpdateCheckTask checkTask;
		bool fullRedraw;
		
		Font logoFont;
		PlatformDrawer platformDrawer;
		public void Init() {
			Window.Resize += Resize;
			Window.FocusedChanged += FocusedChanged;
			Window.WindowStateChanged += Resize;
			Window.Keyboard.KeyDown += KeyDown;
			LoadFont();
			logoFont = new Font( FontName, 32, FontStyle.Regular );
			string path = Assembly.GetExecutingAssembly().Location;
			Window.Icon = Icon.ExtractAssociatedIcon( path );
			//Minimised = Window.WindowState == WindowState.Minimized;
			
			PlatformID platform = Environment.OSVersion.Platform;
			if( platform == PlatformID.Win32Windows )
				platformDrawer = new WinOldPlatformDrawer();
			else if( Configuration.RunningOnWindows )
				platformDrawer = new WinPlatformDrawer();
			else if( Configuration.RunningOnX11 )
				platformDrawer = new X11PlatformDrawer();
			else if( Configuration.RunningOnMacOS )
				platformDrawer = new OSXPlatformDrawer();
		}
		
		void LoadFont() {
			Options.Load();
			FontName = Options.Get( "gui-fontname" ) ?? "Arial";
			try {
				using( Font f = new Font( FontName, 16 ) ) { }
			} catch( Exception ) {
				FontName = "Arial";
				Options.Set( "gui-fontname", "Arial" );
			}
		}

		void FocusedChanged( object sender, EventArgs e ) {
			if( Program.ShowingErrorDialog ) return;
			RedrawBackground();
			Screen.Resize();
		}

		void Resize( object sender, EventArgs e ) {
			platformDrawer.Resize();
			RedrawBackground();
			Screen.Resize();
			fullRedraw = true;
		}
		
		public void SetScreen( LauncherScreen screen ) {
			if( this.Screen != null )
				this.Screen.Dispose();
			
			RedrawBackground();
			this.Screen = screen;
			screen.Init();
		}
		
		public bool ConnectToServer( List<ServerListEntry> publicServers, string hash ) {
			if( String.IsNullOrEmpty( hash ) ) return false;
			
			ClientStartData data = null;
			foreach( ServerListEntry entry in publicServers ) {
				if( entry.Hash == hash ) {
					data = new ClientStartData( Session.Username, entry.Mppass,
					                           entry.IPAddress, entry.Port );
					Client.Start( data, true, ref ShouldExit );
					return true;
				}
			}
			
			// Fallback to private server handling
			try {
				data = Session.GetConnectInfo( hash );
			} catch( WebException ex ) {
				ErrorHandler.LogError( "retrieving server information", ex );
				return false;
			} catch( ArgumentOutOfRangeException ) {
				return false;
			}
			Client.Start( data, true, ref ShouldExit );
			return true;
		}
		
		public void Run() {
			Window = new NativeWindow( 640, 400, Program.AppName, 0,
			                          GraphicsMode.Default, DisplayDevice.Default );
			Window.Visible = true;
			Drawer = new GdiPlusDrawer2D( null );
			Init();
			TryLoadTexturePack();
			platformDrawer.info = Window.WindowInfo;
			platformDrawer.Init();
			
			string audioPath = Path.Combine( Program.AppDirectory, "audio" );
			BinUnpacker.Unpack( audioPath, "dig" );
			BinUnpacker.Unpack( audioPath, "step" );
			
			fetcher = new ResourceFetcher();
			fetcher.CheckResourceExistence();
			checkTask = new UpdateCheckTask();
			checkTask.CheckForUpdatesAsync();
			if( !fetcher.AllResourcesExist )
				SetScreen( new ResourcesScreen( this ) );
			else
				SetScreen( new MainScreen( this ) );
			
			while( true ) {
				Window.ProcessEvents();
				if( !Window.Exists ) break;
				if( ShouldExit ) {
					if( Screen != null )
						Screen.Dispose();
					break;
				}
				
				Screen.Tick();
				if( Dirty ) Display();
				Thread.Sleep( 10 );
			}
			
			if( Options.Load() ) {
				LauncherSkin.SaveToOptions();
				Options.Save();
			}
			
			if( ShouldUpdate )
				Updater.Applier.ApplyUpdate();
			if( Window.Exists )
				Window.Close();
		}
		
		void Display() {
			Screen.OnDisplay();
			Dirty = false;
			
			if( !fullRedraw && DirtyArea.Width > 0 )
				platformDrawer.Redraw( Framebuffer, DirtyArea );
			else
				platformDrawer.Redraw( Framebuffer );
			DirtyArea = Rectangle.Empty;
			fullRedraw = false;
		}
		
		Key lastKey;
		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( IsShutdown( e.Key ) )
				ShouldExit = true;
			lastKey = e.Key;
		}
		
		public void Dispose() {
			Window.Resize -= Resize;
			Window.FocusedChanged -= FocusedChanged;
			Window.WindowStateChanged -= Resize;
			Window.Keyboard.KeyDown -= KeyDown;
			logoFont.Dispose();
		}
		
		bool IsShutdown( Key key ) {
			if( key == Key.F4 && (lastKey == Key.AltLeft || lastKey == Key.AltRight) )
				return true;
			// On OSX, Cmd+Q should also terminate the process.
			if( !OpenTK.Configuration.RunningOnMacOS ) return false;
			return key == Key.Q && (lastKey == Key.WinLeft || lastKey == Key.WinRight);
		}
	}
}
