using System;
using System.Drawing;
using System.IO;
using ClassicalSharp;
using Launcher2.Updater;
using OpenTK.Input;

namespace Launcher2 {
	
	// TODO: Download asynchronously
	public sealed class UpdatesScreen : LauncherScreen {
		
		Font titleFont, infoFont;
		public UpdatesScreen( LauncherWindow game ) : base( game ) {
			game.Window.Mouse.Move += MouseMove;
			game.Window.Mouse.ButtonDown += MouseButtonDown;
			
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			infoFont = new Font( "Arial", 14, FontStyle.Regular );
			buttonFont = titleFont;
			widgets = new LauncherWidget[20];
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			checkTask = new UpdateCheckTask();
			checkTask.CheckForUpdatesAsync();
			game.Window.Keyboard.KeyDown += KeyDown;
			game.Window.Keyboard.KeyUp += KeyUp;
			Resize();
		}

		void KeyDown( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab ) {
				HandleTab();
			} else if( e.Key == Key.Enter ) {
				LauncherWidget widget = selectedWidget;
				if( widget != null && widget.OnClick != null )
					widget.OnClick( 0, 0 );
			}
		}
		
		void KeyUp( object sender, KeyboardKeyEventArgs e ) {
			if( e.Key == Key.Tab )
				tabDown = false;
		}

		Build dev, stable;
		public override void Tick() {
			if( checkTask != null && !checkTask.Working ) {
				if( checkTask.Exception != null ) {
					updateCheckFailed = true;
				} else {
					dev = checkTask.LatestDev;
					lastDev = dev.TimeBuilt;
					validDev = dev.DirectXSize > 50000 && dev.OpenGLSize > 50000;
					
					stable = checkTask.LatestStable;
					lastStable = stable.TimeBuilt;
					validStable = stable.DirectXSize > 50000 && stable.OpenGLSize > 50000;
				}
				checkTask = null;
				game.MakeBackground();
				Resize();
			}
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		const string dateFormat = "dd-MM-yyyy HH:mm";
		DateTime lastStable, lastDev;
		bool validStable = true, validDev = true;
		bool updateCheckFailed;
		
		void Draw() {
			widgetIndex = 0;
			string exePath = Path.Combine( Program.AppDirectory, "ClassicalSharp.exe" );
			
			MakeLabelAt( "Your build:", infoFont, Anchor.Centre, Anchor.Centre, -100, -120 );
			string yourBuild = File.GetLastWriteTime( exePath ).ToString( dateFormat );
			MakeLabelAt( yourBuild, infoFont, Anchor.Centre, Anchor.Centre, 30, -120 );
			
			MakeLabelAt( "Update to stable build", titleFont, Anchor.Centre, Anchor.Centre, -130, -80 );
			MakeLabelAt( "Latest stable:", infoFont, Anchor.Centre, Anchor.Centre, -90, -50 );
			string latestStable = GetDateString( lastStable, validStable );
			MakeLabelAt( latestStable, infoFont, Anchor.Centre, Anchor.Centre, 50, -50 );
			MakeButtonAt( "Direct3D 9", 130, 30, titleFont, Anchor.Centre, -80, -15,
			             (x, y) => UpdateBuild( lastStable, validStable, true, true ) );
			MakeButtonAt( "OpenGL", 130, 30, titleFont, Anchor.Centre, 80, -15,
			             (x, y) => UpdateBuild( lastStable, validStable, true, false ) );
			
			MakeLabelAt( "Update to dev build", titleFont, Anchor.Centre, Anchor.Centre, -120, 30 );
			MakeLabelAt( "Latest dev:", infoFont, Anchor.Centre, Anchor.Centre, -100, 60 );
			string latestDev = GetDateString( lastDev, validDev );
			MakeLabelAt( latestDev, infoFont, Anchor.Centre, Anchor.Centre, 30, 60 );
			MakeButtonAt( "Direct3D 9", 130, 30, titleFont, Anchor.Centre, -80, 95,
			             (x, y) => UpdateBuild( lastDev, validDev, false, true ) );
			MakeButtonAt( "OpenGL", 130, 30, titleFont, Anchor.Centre, 80, 95,
			             (x, y) => UpdateBuild( lastDev, validDev, false, false ) );
			
			MakeLabelAt( "&eThe Direct3D 9 builds are the recommended builds for Windows.", 
			            infoFont, Anchor.Centre, Anchor.Centre, 0, 130 );
			
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 170, (x, y) => game.SetScreen( new MainScreen( game ) ) );
		}
		
		string GetDateString( DateTime last, bool valid ) {
			if( updateCheckFailed ) return "Update check failed";
			if( !valid ) return "Build corrupted";
			if( last == DateTime.MinValue ) return "Checking..";
			
			return last.ToString( dateFormat );
		}
		
		void UpdateBuild( DateTime last, bool valid, bool release, bool dx ) {
			if( last == DateTime.MinValue || !valid ) return;
			
			Build build = release ? stable : dev;
			string dir = dx ? build.DirectXPath : build.OpenGLPath;
			Console.WriteLine( "FETCH! " + dir );
			Patcher.Update( dir );
		}
		
		public override void Dispose() {
			game.Window.Keyboard.KeyDown -= KeyDown;
			game.Window.Keyboard.KeyUp -= KeyUp;
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			titleFont.Dispose();
			infoFont.Dispose();
		}
	}
}
