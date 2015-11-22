using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen( Game game ) : base( game ) {
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
			"Set spawn", "Open chat", "Send chat", "Pause", "Open inventory", "Cycle view distance",
			"Show player list", "Speed", "Toggle noclip", "Toggle fly", "Fly up", "Fly down",
			"Hide gui", "Hide FPS", "Take screenshot", "Toggle fullscreen", "Toggle 3rd person",
				"Toggle extended input", };
		
		public override void Init() {
			if( keyNames == null )
				keyNames = Enum.GetNames( typeof( Key ) );
			keyFont = new Font( "Arial", 14, FontStyle.Bold );
			regularFont = new Font( "Arial", 14, FontStyle.Italic );
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[descriptions.Length + 1];
			
			MakeKeys( 0, 12, -140 );
			MakeKeys( 12, 12, 140 );
			buttons[index] = Make( 0, 5, "Back to menu", Anchor.BottomOrRight, 
			                      (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			statusWidget = TextWidget.Create( game, 0, 160, "", Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		int index;
		void MakeKeys( int start, int len, int x ) {
			int y = -200;
			for( int i = 0; i < len; i++ ) {
				KeyBinding binding = (KeyBinding)((int)start + i);
				string text = descriptions[start + i] + ": " 
					+ keyNames[(int)game.Mapping( binding )];
				
				buttons[index++] = ButtonWidget.Create( game, x, y, 240, 25, text,
				                                       Anchor.Centre, Anchor.Centre, keyFont, OnWidgetClick );
				y += 30;
			}
		}
		
		ButtonWidget curWidget;
		void OnWidgetClick( Game game, Widget realWidget ) {
			this.curWidget = (ButtonWidget)realWidget;
			int index = Array.IndexOf<ButtonWidget>( buttons, curWidget );
			string text = "&ePress new key binding for " + descriptions[index] + ":";
			statusWidget.SetText( text );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
			} else if( curWidget != null ) {
				int index = Array.IndexOf<ButtonWidget>( buttons, curWidget );
				KeyBinding mapping = (KeyBinding)index;
				KeyMap map = game.InputHandler.Keys;
				Key oldKey = map[mapping];
				string reason;
				
				if( !map.IsKeyOkay( oldKey, key, out reason ) ) {
					const string format = "&eFailed to change mapping \"{0}\". &c({1})";
					statusWidget.SetText( String.Format( format, descriptions[index], reason ) );
				} else {
					const string format = "&eChanged mapping \"{0}\" from &7{1} &eto &7{2}&e.";
					statusWidget.SetText( String.Format( format, descriptions[index], oldKey, key ) );
					string text = descriptions[index] + " : " + keyNames[(int)key];
					
					curWidget.SetText( text );
					map[mapping] = key;
				}
				curWidget = null;
			}
			return true;
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, vDocking, keyFont, onClick );
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
			statusWidget.Dispose();
		}
	}
}