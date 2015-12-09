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
			widgets = new LauncherWidget[17];
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
			
			MakeLabelAt( "Your build:", titleFont, Anchor.Centre, Anchor.Centre, -55, -120 );
			string yourBuild = File.GetLastWriteTime( "ClassicalSharp.exe" ).ToString( dateFormat );
			MakeLabelAt( yourBuild, infoFont, Anchor.Centre, Anchor.Centre, 100, -120 );
			
			MakeLabelAt( "Latest stable:", titleFont, Anchor.Centre, Anchor.Centre, -70, -80 );
			string latestStable = GetDateString( lastStable, validStable );
			MakeLabelAt( latestStable, infoFont, Anchor.Centre, Anchor.Centre, 100, -80 );
			MakeButtonAt( "Update to D3D9 stable", 260, 30, titleFont, Anchor.Centre, 0, -40,
			             (x, y) => UpdateBuild( lastStable, validStable, true, true ) );
			MakeButtonAt( "Update to OpenGL stable", 260, 30, titleFont, Anchor.Centre, 0, 0,
			             (x, y) => UpdateBuild( lastStable, validStable, true, false ) );
			
			MakeLabelAt( "Latest dev:", titleFont, Anchor.Centre, Anchor.Centre, -60, 40 );
			string latestDev = GetDateString( lastDev, validDev );
			MakeLabelAt( latestDev, infoFont, Anchor.Centre, Anchor.Centre, 100, 40 );
			MakeButtonAt( "Update to D3D9 dev", 240, 30, titleFont, Anchor.Centre, 0, 80,
			             (x, y) => UpdateBuild( lastDev, validDev, false, true ) );
			MakeButtonAt( "Update to OpenGL dev", 240, 30, titleFont, Anchor.Centre, 0, 120,
			             (x, y) => UpdateBuild( lastDev, validDev, false, false ) );
			
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 200, (x, y) => game.SetScreen( new MainScreen( game ) ) );
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
