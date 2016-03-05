using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class KeyBindingsScreen : MenuScreen {
		
		public KeyBindingsScreen( Game game ) : base( game ) { }
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuWidgets( delta );
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
			keyFont = new Font( game.FontName, 15, FontStyle.Bold );
			regularFont = new Font( game.FontName, 15, FontStyle.Italic );
			statusWidget = TextWidget.Create( game, 0, 130, "", Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		protected int index;
		protected void MakeKeys( KeyBinding start, int descStart, int len, int x ) {
			int y = -180;
			for( int i = 0; i < len; i++ ) {
				KeyBinding binding = (KeyBinding)((int)start + i);
				string text = descriptions[descStart + i] + ": "
					+ keyNames[(int)game.Mapping( binding )];
				
				widgets[index++] = ButtonWidget.Create( game, x, y, 260, 35, text,
				                                       Anchor.Centre, Anchor.Centre, keyFont, OnBindingClick );
				y += 45;
			}
		}
		
		ButtonWidget curWidget;
		void OnBindingClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn == MouseButton.Right && (curWidget == null || curWidget == widget) ) {
				curWidget = (ButtonWidget)widget;
				int index = Array.IndexOf<Widget>( widgets, curWidget );
				KeyBinding mapping = (KeyBinding)(index + (int)originKey);
				HandlesKeyDown( game.InputHandler.Keys.GetDefault( mapping ) );
			}
			if( mouseBtn != MouseButton.Left ) return;
			
			if( curWidget == widget ) {
				curWidget = null;
				statusWidget.SetText( "" );				
			} else {
				curWidget = (ButtonWidget)widget;
				int index = Array.IndexOf<Widget>( widgets, curWidget );
				string text = "&ePress new key binding for " + descriptions[index] + ":";
				statusWidget.SetText( text );				
			}
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
			} else if( curWidget != null ) {
				int index = Array.IndexOf<Widget>( widgets, curWidget );
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
	
	public sealed class ClassicKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Forward", "Back", "Left", 
			"Right", "Jump", "Load loc", "Save loc", "Chat", "Build", "Toggle fog" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Forward;
			widgets = new Widget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Forward, 0, 5, -150 );
			MakeKeys( KeyBinding.Respawn, 5, 5, 150 );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
		}
	}
	
	public sealed class NormalKeyBindingsScreen : KeyBindingsScreen {
		
		public NormalKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Forward", "Back", "Left",
			"Right", "Jump", "Respawn", "Set spawn", "Open chat", "Open inventory", 
			"View distance", "Send chat", "Pause", "Player list"  };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Forward;
			widgets = new Widget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Forward, 0, 6, -150 );
			MakeKeys( KeyBinding.SetSpawn, 6, 7, 150 );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Advanced key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new AdvancedKeyBindingsScreen( game ) );
		}
	}
	
	public sealed class AdvancedKeyBindingsScreen : KeyBindingsScreen {
		
		public AdvancedKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Speed", "Noclip mode", "Fly mode",
			"Fly up", "Fly down", "Show ext input", "Hide FPS", "Take screenshot", "Fullscreen",
			"Third person", "Hide gui", "Show axis lines", "Zoom scrolling", "Half speed" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Speed;
			widgets = new Widget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Speed, 0, 7, -150 );
			MakeKeys( KeyBinding.Screenshot, 7, 7, 150 );
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Mouse key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new MouseKeyBindingsScreen( game ) );
		}
	}
	
	public sealed class MouseKeyBindingsScreen : KeyBindingsScreen {
		
		public MouseKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Left mouse", "Middle mouse", "Right mouse" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.MouseLeft;
			widgets = new Widget[descriptions.Length + 3];
			MakeKeys( KeyBinding.MouseLeft, 0, 3, 0 );
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new PauseScreen( g ) ) );
			widgets[index++] = ButtonWidget.Create(
				game, 0, 170, 300, 35, "Normal key bindings",
				Anchor.Centre, Anchor.Centre, titleFont, NextClick );
			widgets[index++] = ChatTextWidget.Create(
				game, 0, -40, "&eRight click to remove the key binding", 
				Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		void NextClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.SetNewScreen( new NormalKeyBindingsScreen( game ) );
		}
	}
}