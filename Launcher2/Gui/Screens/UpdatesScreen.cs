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
			
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			infoFont = new Font( game.FontName, 14, FontStyle.Regular );
			buttonFont = titleFont;
			widgets = new LauncherWidget[13];
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			if( game.checkTask.Done )
				SuccessfulUpdateCheck( game.checkTask );
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
			if( checkTask.Done ) return;
			if( !checkTask.TaskTick( SuccessfulUpdateCheck, this ) )
				updateCheckFailed = true;
		}
		
		void SuccessfulUpdateCheck( UpdateCheckTask task ) {
			dev = task.LatestDev;
			lastDev = dev.TimeBuilt;
			validDev = dev.DirectXSize > 50000 && dev.OpenGLSize > 50000;
			
			stable = task.LatestStable;
			lastStable = stable.TimeBuilt;
			validStable = stable.DirectXSize > 50000 && stable.OpenGLSize > 50000;
			game.checkTask = checkTask;
		}
		
		public override void Resize() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
				FastColour col = LauncherSkin.ButtonBorderCol;
				int middle = game.Height / 2;
				game.Drawer.DrawRect( col, game.Width / 2 - 160, middle - 100, 320, 1 );
				game.Drawer.DrawRect( col, game.Width / 2 - 160, middle - 10, 320, 1 );
			}
			Dirty = true;
		}
		
		const string dateFormat = "dd-MM-yyyy HH:mm";
		DateTime lastStable, lastDev;
		bool validStable = true, validDev = true;
		bool updateCheckFailed;
		
		void MakeWidgets() {
			widgetIndex = 0;
			string exePath = Path.Combine( Program.AppDirectory, "ClassicalSharp.exe" );
			
			MakeLabelAt( "Your build:", infoFont, Anchor.Centre, Anchor.Centre, -60, -120 );
			string yourBuild = File.GetLastWriteTime( exePath ).ToString( dateFormat );
			MakeLabelAt( yourBuild, infoFont, Anchor.Centre, Anchor.Centre, 70, -120 );
			
			MakeLabelAt( "Latest stable:", infoFont, Anchor.Centre, Anchor.Centre, -70, -75 );
			string latestStable = GetDateString( lastStable, validStable );
			MakeLabelAt( latestStable, infoFont, Anchor.Centre, Anchor.Centre, 70, -75 );
			MakeButtonAt( "Direct3D 9", 130, 30, titleFont, Anchor.Centre, -80, -40,
			             (x, y) => UpdateBuild( lastStable, validStable, true, true ) );
			MakeButtonAt( "OpenGL", 130, 30, titleFont, Anchor.Centre, 80, -40,
			             (x, y) => UpdateBuild( lastStable, validStable, true, false ) );
			
			MakeLabelAt( "Latest dev build:", infoFont, Anchor.Centre, Anchor.Centre, -80, 15 );
			string latestDev = GetDateString( lastDev, validDev );
			MakeLabelAt( latestDev, infoFont, Anchor.Centre, Anchor.Centre, 70, 15 );
			MakeButtonAt( "Direct3D 9", 130, 30, titleFont, Anchor.Centre, -80, 50,
			             (x, y) => UpdateBuild( lastDev, validDev, false, true ) );
			MakeButtonAt( "OpenGL", 130, 30, titleFont, Anchor.Centre, 80, 50,
			             (x, y) => UpdateBuild( lastDev, validDev, false, false ) );
			
			MakeLabelAt( "&eDirect3D 9 is recommended for Windows.",
			            infoFont, Anchor.Centre, Anchor.Centre, 0, 105 );
			MakeLabelAt( "&eThe client must be closed before updating.",
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
