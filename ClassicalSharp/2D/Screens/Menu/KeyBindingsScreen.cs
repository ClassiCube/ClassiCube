using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class KeyBindingsScreen : MenuScreen {
		
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
		protected string[] descriptions;
		protected KeyBinding originKey;
		
		public override void Init() {
			base.Init();
			if( keyNames == null )
				keyNames = Enum.GetNames( typeof( Key ) );
			keyFont = new Font( "Arial", 15, FontStyle.Bold );
			regularFont = new Font( "Arial", 15, FontStyle.Italic );
			statusWidget = TextWidget.Create( game, 0, 130, "", Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		protected int index;
		protected void MakeKeys( KeyBinding start, int descStart, int len, int x ) {
			int y = -180;
			for( int i = 0; i < len; i++ ) {
				KeyBinding binding = (KeyBinding)((int)start + i);
				string text = descriptions[descStart + i] + ": "
					+ keyNames[(int)game.Mapping( binding )];
				
				buttons[index++] = ButtonWidget.Create( game, x, y, 260, 35, text,
				                                       Anchor.Centre, Anchor.Centre, keyFont, OnBindingClick );
				y += 45;
			}
		}
		
		ButtonWidget curWidget;
		void OnBindingClick( Game game, Widget realWidget ) {
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
				KeyBinding mapping = (KeyBinding)(index + (int)originKey);
				KeyMap map = game.InputHandler.Keys;
				Key oldKey = map[mapping];
				string reason;
				
				if( !map.IsKeyOkay( oldKey, key, out reason ) ) {
					const string format = "&eFailed to change mapping \"{0}\". &c({1})";
					statusWidget.SetText( String.Format( format, descriptions[index], reason ) );
				} else {
					const string format = "&eChanged mapping \"{0}\" from &7{1} &eto &7{2}&e.";
					statusWidget.SetText( String.Format( format, descriptions[index], oldKey, key ) );
					string text = descriptions[index] + ": " + keyNames[(int)key];
					
					curWidget.SetText( text );
					map[mapping] = key;
				}
				curWidget = null;
			}
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
			statusWidget.Dispose();
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen( Game game ) : base( game ) {
		}
		
		static string[] normDescriptions = new [] { "Forward", "Back", "Left",
			"Right", "Jump", "Respawn", "Set spawn", "Open chat", "Send chat",
			"Pause", "Open inventory", "Cycle view distance", "Show player list"  };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Forward;
			buttons = new ButtonWidget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Forward, 0, 6, -150 );
			MakeKeys( KeyBinding.SetSpawn, 6, 7, 150 );
			
			buttons[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			buttons[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Advanced key bindings", 
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget ) {
			game.SetNewScreen( new AdvancedKeyBindingsScreen( game ) );
		}
	}
	
	public sealed class AdvancedKeyBindingsScreen : KeyBindingsScreen {
		
		public AdvancedKeyBindingsScreen( Game game ) : base( game ) {
		}
		
		static string[] normDescriptions = new [] { "Speed", "Toggle noclip", "Toggle fly", 
			"Fly up", "Fly down", "Toggle ext input", "Hide FPS", "Take screenshot", "Toggle fullscreen", 
			"Toggle 3rd person", "Hide gui", "Show axis lines", "Zoom scrolling", "Half speed" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Speed;
			buttons = new ButtonWidget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Speed, 0, 7, -150 );
			MakeKeys( KeyBinding.Screenshot, 7, 7, 150 );
			buttons[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			buttons[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Normal key bindings", 
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget ) {
			game.SetNewScreen( new NormalKeyBindingsScreen( game ) );
		}
	}
}