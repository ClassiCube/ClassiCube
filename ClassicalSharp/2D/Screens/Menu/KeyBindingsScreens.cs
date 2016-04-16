// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class ClassicKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Forward", "Back", "Left", 
			"Right", "Jump", "Load loc", "Save loc", "Chat", "Build", "Toggle fog" };
		
		public override void Init() {
			base.Init();
			btnWidth = 301; btnHeight = 40;
			btnDistance = 48;
			descriptions = normDescriptions;
			originKey = KeyBinding.Forward;
			widgets = new Widget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Forward, 0, 5, -160, -180 );
			MakeKeys( KeyBinding.Respawn, 5, 5, 160, -180 );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) );
		}
	}
	
	public sealed class ClassicHacksKeyBindingsScreen : KeyBindingsScreen {
		
		public ClassicHacksKeyBindingsScreen( Game game ) : base( game ) { }
		
		static string[] normDescriptions = new [] { "Forward", "Back", "Left", 
			"Right", "Jump", "Load loc", "Save loc", "Chat", "Build", "Toggle fog",
            null, null, null, "Speed", "Noclip", "Fly", "Fly up", "Fly down" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Forward;
			widgets = new Widget[descriptions.Length + 7];
			MakeKeys( KeyBinding.Forward, 0, 8, -150, -220 );
			MakeKeys( KeyBinding.OpenInventory, 8, 2, 150, -220 );
			index += 3; // Skip SendChat, PauseOrExit, PlayerList
			MakeKeys( KeyBinding.Speed, 13, 5, 150, -220 + 2 * btnDistance );
			
			widgets[index++] = MakeBack( false, titleFont,
			                            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) );
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
			MakeKeys( KeyBinding.Forward, 0, 7, -150, -180 );
			MakeKeys( KeyBinding.OpenChat, 7, 6, 150, -180 );
			
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
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
		
		static string[] normDescriptions = new [] { "Speed", "Noclip", "Fly",
			"Fly up", "Fly down", "Show ext input", "Hide FPS", "Take screenshot", "Fullscreen",
			"Third person", "Hide gui", "Show axis lines", "Zoom scrolling", "Half speed" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Speed;
			widgets = new Widget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Speed, 0, 7, -150, -180 );
			MakeKeys( KeyBinding.Screenshot, 7, 7, 150, -180 );
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
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
			MakeKeys( KeyBinding.MouseLeft, 0, 3, 0, -180 );
			widgets[index++] = MakeBack( "Back to menu", 5, titleFont,
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