using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class NormalKeyBindingsScreen : KeyBindingsScreen {
		
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
}