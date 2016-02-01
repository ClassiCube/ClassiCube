using System;
using System.Drawing;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed partial class MainScreen : LauncherInputScreen {
		
		public MainScreen( LauncherWindow game ) : base( game, true ) {
			buttonFont = new Font( game.FontName, 16, FontStyle.Bold );
			enterIndex = 2;
			widgets = new LauncherWidget[14];
			LoadResumeInfo();
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
			             (x, y) => Client.Start( "", ref game.ShouldExit ) );
			
			if( !game.ClassicMode ) {
				MakeButtonAt( "Colour scheme", 160, buttonHeight, buttonFont,
				             Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10,
				             (x, y) => game.SetScreen( new ColoursScreen( game ) ) );
			} else {
				widgets[widgetIndex++] = null;
			}
			
			MakeButtonAt( "Update check", 160, buttonHeight, buttonFont,
			             Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
			string mode = game.ClassicMode ? "Normal mode" : "Pure classic mode";
			MakeButtonAt( mode, 190, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.BottomOrRight, 0, -10, ModeClick );
			
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
		
		void ModeClick( int mouseX, int mouseY ) {
			game.ClassicMode = !game.ClassicMode;
			Options.Load();
			Options.Set( "mode-classic", game.ClassicMode );
			if( game.ClassicMode )
				Options.Set( "namesmode", "AllNamesAndHovered" );
			
			Options.Set( "nostalgia-customblocks", !game.ClassicMode );
			Options.Set( "nostalgia-usecpe", !game.ClassicMode );
			Options.Set( "nostalgia-servertextures", !game.ClassicMode );
			Options.Set( "nostalgia-classictablist", game.ClassicMode );
			Options.Set( "nostalgia-classicoptions", game.ClassicMode );
			Options.Set( "nostalgia-classicgui", true );
			Options.Set( "hacksenabled", !game.ClassicMode );
			Options.Set( "doublejump", false );
			Options.Save();
			
			game.MakeBackground();
			Resize();
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
			StoreFields();
			base.Dispose();
		}
	}
}
