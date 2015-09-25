using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class KeyMappingsScreen : MenuScreen {
		
		public KeyMappingsScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			statusWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		Font keyFont;
		TextWidget statusWidget;
		static string[] keyNames;
		static string[] descriptions = new [] { "Forward", "Back", "Left", "Right", "Jump", "Respawn",
			"Set spawn", "Open chat", "Send chat", "Pause", "Open inventory", "Take screenshot",
			"Toggle fullscreen", "Toggle 3rd person", "Cycle view distance", "Toggle fly", "Speed",
			"Toggle noclip", "Fly up", "Fly down", "Display player list", "Hide gui" };
		
		public override void Init() {
			if( keyNames == null )
				keyNames = Enum.GetNames( typeof( Key ) );
			keyFont = new Font( "Arial", 14, FontStyle.Bold );
			regularFont = new Font( "Arial", 14, FontStyle.Italic );
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[descriptions.Length + 1];
			
			MakeKeys( 0, 11, -140 );
			MakeKeys( 11, 11, 140 );
			buttons[index] = Make( 0, 5, "Back to menu", Docking.BottomOrRight, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			statusWidget = TextWidget.Create( game, 0, 150, "", Docking.Centre, Docking.Centre, regularFont );
		}
		
		int index;
		void MakeKeys( int start, int len, int x ) {
			int y = -180;
			for( int i = 0; i < len; i++ ) {
				KeyMapping mapping = (KeyMapping)( (int)start + i );
				string text = descriptions[start + i] + ": " + keyNames[(int)game.Keys[mapping]];
				
				buttons[index++] = ButtonWidget.Create( game, x, y, 240, 25, text,
				                                       Docking.Centre, Docking.Centre, keyFont, OnWidgetClick );
				y += 30;
			}
		}
		
		ButtonWidget widget;
		void OnWidgetClick( Game game, ButtonWidget widget ) {
			this.widget = widget;
			int index = Array.IndexOf<ButtonWidget>( buttons, widget );
			statusWidget.Dispose();
			
			string text = "Press new key binding for " + descriptions[index] + ":";
			statusWidget = TextWidget.Create( game, 0, 150, text, Docking.Centre, Docking.Centre, regularFont );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( new NormalScreen( game ) );
			} else if( widget != null ) {
				int index = Array.IndexOf<ButtonWidget>( buttons, widget );
				KeyMapping mapping = (KeyMapping)index;
				Key oldKey = game.Keys[mapping];
				string reason;
				
				if( !game.Keys.IsKeyOkay( oldKey, key, out reason ) ) {
					const string format = "&eFailed to change mapping \"{0}\". &c({1})";
					statusWidget.SetText( String.Format( format, descriptions[index], reason ) );
				} else {
					const string format = "&eChanged mapping \"{0}\" from &7{1} &eto &7{2}&e.";					
					statusWidget.SetText( String.Format( format, descriptions[index], oldKey, key ) );					
					string text = descriptions[index] + " : " + keyNames[(int)key];
					widget.SetText( text );
					game.Keys[mapping] = key;
				}
				widget = null;
			}
			return true;
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game, ButtonWidget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, keyFont, onClick );
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
			statusWidget.Dispose();
		}
	}
}