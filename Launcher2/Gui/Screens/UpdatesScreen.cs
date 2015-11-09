using System;
using System.Drawing;
using System.Globalization;
using System.IO;
using ClassicalSharp;
using Launcher2.Updater;

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
			widgets = new LauncherWidget[16];
		}

		UpdateCheckTask checkTask;
		public override void Init() {
			checkTask = new UpdateCheckTask();
			checkTask.CheckForUpdatesAsync();
			Resize();
		}
		
		public override void Tick() {
			if( checkTask != null && !checkTask.Working ) {
				if( checkTask.Exception != null ) {
					updateCheckFailed = true;
				} else {
					lastStable = DateTime.Parse( checkTask.LatestStableDate,
					                            null, DateTimeStyles.AssumeUniversal );
					lastDev =  DateTime.Parse( checkTask.LatestDevDate,
					                          null, DateTimeStyles.AssumeUniversal );
					
					validStable = Int32.Parse( checkTask.LatestStableSize ) > 50000;
					validDev = Int32.Parse( checkTask.LatestDevSize ) > 50000;
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
			string yourBuild = File.GetLastWriteTimeUtc( "ClassicalSharp.exe" ).ToString( dateFormat );
			MakeLabelAt( yourBuild, infoFont, Anchor.Centre, Anchor.Centre, 100, -120 );
			
			MakeLabelAt( "Latest stable:", titleFont, Anchor.Centre, Anchor.Centre, -70, -80 );
			string latestStable = GetDateString( lastStable, validStable );
			MakeLabelAt( latestStable, infoFont, Anchor.Centre, Anchor.Centre, 100, -80 );
			MakeButtonAt( "Update to stable", 180, 30, titleFont, Anchor.Centre, 0, -40,
			             (x, y) => UpdateBuild( lastStable, validStable, "latest.Release.zip" ) );
			
			MakeLabelAt( "Latest dev:", titleFont, Anchor.Centre, Anchor.Centre, -60, 40 );
			string latestDev = GetDateString( lastDev, validDev );
			MakeLabelAt( latestDev, infoFont, Anchor.Centre, Anchor.Centre, 100, 40 );
			MakeButtonAt( "Update to OpenGL dev", 240, 30, titleFont, Anchor.Centre, 0, 80,
			             (x, y) => UpdateBuild( lastDev, validDev, "latest.zip" ) );
			MakeButtonAt( "Update to D3D9 dev", 240, 30, titleFont, Anchor.Centre, 0, 120,
			             (x, y) => UpdateBuild( lastDev, validDev, "latest.DirectX.zip" ) );
			
			MakeButtonAt( "Back", 80, 35, titleFont, Anchor.Centre,
			             0, 200, (x, y) => game.SetScreen( new MainScreen( game ) ) );
		}
		
		string GetDateString( DateTime last, bool valid ) {
			if( updateCheckFailed ) return "Update check failed";
			if( !valid ) return "Build corrupted";
			if( last == DateTime.MinValue ) return "Checking..";
			
			return last.ToUniversalTime().ToString( dateFormat );
		}
		
		void UpdateBuild( DateTime last, bool valid, string dir ) {
			if( last == DateTime.MinValue || !valid ) return;
			Patcher.Update( dir );
		}
		
		public override void Dispose() {
			game.Window.Mouse.Move -= MouseMove;
			game.Window.Mouse.ButtonDown -= MouseButtonDown;
			
			titleFont.Dispose();
			infoFont.Dispose();
		}
	}
}
