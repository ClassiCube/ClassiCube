using System;
using System.Drawing;
using System.Net;
using ClassicalSharp;

namespace Launcher2 {
	
	public sealed partial class MainScreen : LauncherInputScreen {
		
		public MainScreen( LauncherWindow game ) : base( game, true ) {
			buttonFont = new Font( "Arial", 16, FontStyle.Bold );
			enterIndex = 4;
			widgets = new LauncherWidget[11];
			LoadResumeInfo();
		}
		
		public override void Resize() {
			using( drawer ) {
				drawer.SetBitmap( game.Framebuffer );
				Draw();
			}
			Dirty = true;
		}
		
		void Draw() {
			widgetIndex = 0;
			DrawClassicube();
			
			MakeButtonAt( "Resume", 100, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, -90, -50, ResumeClick );
			
			MakeButtonAt( "Direct connect", 200, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 80,
			             (x, y) => game.SetScreen( new DirectConnectScreen( game ) ) );
			
			MakeButtonAt( "Singleplayer", 200, buttonHeight, buttonFont,
			             Anchor.Centre, Anchor.Centre, 0, 135,
			             (x, y) => Client.Start( "", ref game.ShouldExit ) );
			
			MakeButtonAt( "Colour scheme", 160, buttonHeight, buttonFont,
			             Anchor.LeftOrTop, Anchor.BottomOrRight, 10, -10,
			             (x, y) => game.SetScreen( new ColoursScreen( game ) ) );
			
			MakeButtonAt( "Update check", 160, buttonHeight, buttonFont,
			             Anchor.BottomOrRight, Anchor.BottomOrRight, -10, -10,
			             (x, y) => game.SetScreen( new UpdatesScreen( game ) ) );
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
			const string format = "&eResume to {0}:{1}, with the name {2}";
			SetStatusNoLock( String.Format( format, resumeIp, resumePort, resumeUser ) );
		}
		
		protected override void UnselectWidget( LauncherWidget widget ) {
			base.UnselectWidget( widget );
			if( signingIn || !resumeValid || widget != widgets[4] ) return;
			SetStatusNoLock( "" );
		}
		
		public override void Dispose() {
			buttonFont.Dispose();
			StoreFields();
			base.Dispose();
		}
	}
}
