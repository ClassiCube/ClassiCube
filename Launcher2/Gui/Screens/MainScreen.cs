// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using System.IO;
using System.Net;
using ClassicalSharp;

namespace Launcher {
	
	public sealed partial class MainScreen : LauncherInputScreen {
		
		Font updateFont;
		
		public MainScreen( LauncherWindow game ) : base( game, true ) {
			buttonFont = new Font( game.FontName, 16, FontStyle.Bold );
			updateFont = new Font( game.FontName, 12, FontStyle.Italic );
			enterIndex = 2;
			widgets = new LauncherWidget[16];
			LoadResumeInfo();
		}
		
		public override void Init() {
			base.Init();
			Resize();
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				LoadSavedInfo( drawer );
			}
		}
		
		public override void Resize() {
			MakeWidgets();
			RedrawAllButtonBackgrounds();
			
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				RedrawAll();
			}
			Dirty = true;
		}
		
		string updateText = "&eChecking for updates..";
		bool updateDone;
		void SuccessfulUpdateCheck( UpdateCheckTask task ) {
			if( updateDone ) return;			
			string latestVer = game.checkTask.LatestStable.Version.Substring( 1 );
			int spaceIndex = Program.AppName.LastIndexOf( ' ' );
			string currentVer = Program.AppName.Substring( spaceIndex + 1 );		
			bool update = new Version( latestVer ) > new Version( currentVer );
			
			updateText = update ? "&aNew release available" : "&eUp to date     ";
			updateDone = true;
			game.MakeBackground();
			Resize();
			SelectWidget( selectedWidget );
		}
		
		void MakeWidgets() {
			widgetIndex = 0;
			DrawClassicube();
			
			MakeButtonAt( "Resume", 100, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 90, -20, ResumeClick );
			
			MakeButtonAt( "Direct connect", 200, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 60,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );
			
			MakeButtonAt( "Singleplayer", 200, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 110,
			             (x, y) => Client.Start( widgets[0].Text, ref game.ShouldExit ) );
			
			if( !game.ClassicBackground ) {
				MakeButtonAt( "Colours", 110, buttonHeight, buttonFont,
				             Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10,
				             (x, y) => game.SetScreen( new ColoursScreen( game ) ) );
			} else {
				widgets[widgetIndex++] = null;
			}
			
			MakeButtonAt( "Updates", 110, buttonHeight, buttonFont,
			             Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
			MakeButtonAt( "Choose mode", 200, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.BottomOrRight, 0, -10,
			             (x, y) => game.SetScreen( new ChooseModeNormalScreen( game ) ) );
			
			MakeLabelAt( updateText, updateFont, Anchor.BottomOrRight, 
			            Anchor.BottomOrRight, -10, -50 );
			
			if( widgets[widgetIndex] != null ) {
				MakeSSLSkipValidationBoolean();
				MakeSSLSkipValidationLabel();
			}
		}

		const int buttonWidth = 220, buttonHeight = 35, sideButtonWidth = 150;
		string resumeUser, resumeIp, resumePort, resumeMppass;
		bool resumeCCSkins, resumeValid;
		
		void LoadResumeInfo() {
			resumeUser = Options.Get( "launcher-username" );
			resumeIp = Options.Get( "launcher-ip" ) ?? "";
			resumePort = Options.Get( "launcher-port" ) ?? "";
			resumeCCSkins = Options.GetBool( "launcher-ccskins", true );
			
			IPAddress address;
			if( !IPAddress.TryParse( resumeIp, out address ) ) resumeIp = "";
			ushort portNum;
			if( !UInt16.TryParse( resumePort, out portNum ) ) resumePort = "";
			
			string mppass = Options.Get( "launcher-mppass" ) ?? null;
			resumeMppass = Secure.Decode( mppass, resumeUser );
			resumeValid = !String.IsNullOrEmpty( resumeUser ) && !String.IsNullOrEmpty( resumeIp )
				&& !String.IsNullOrEmpty( resumePort ) && !String.IsNullOrEmpty( resumeMppass );
		}
		
		void ResumeClick( int mouseX, int mouseY ) {
			if( !resumeValid ) return;
			ClientStartData data = new ClientStartData( resumeUser, resumeMppass, resumeIp, resumePort );
			Client.Start( data, resumeCCSkins, ref game.ShouldExit );
		}
		
		protected override void SelectWidget( LauncherWidget widget ) {
			base.SelectWidget( widget );
			if( signingIn || !resumeValid || widget != widgets[4] ) return;
			const string format = "&eResume to {0}:{1}, as {2}";
			SetStatus( String.Format( format, resumeIp, resumePort, resumeUser ) );
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			base.UnselectWidget( widget );
			if( signingIn || !resumeValid || widget != widgets[4] ) return;
			SetStatus( "" );
		}
		
		public override void Dispose() {
			buttonFont.Dispose();
			updateFont.Dispose();
			StoreFields();
			base.Dispose();
		}
	}
}
