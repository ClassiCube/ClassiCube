using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class AdvancedKeyBindingsScreen : KeyBindingsScreen {
		
		public AdvancedKeyBindingsScreen( Game game ) : base( game ) {
		}
		
		static string[] normDescriptions = new [] { "Speed", "Toggle noclip", "Toggle fly", 
			"Fly up", "Fly down", "Toggle extended input", "Hide FPS", "Take screenshot", 
			"Toggle fullscreen", "Toggle 3rd person", "Hide gui", "Show axis lines", "Cycle zoom" };
		
		public override void Init() {
			base.Init();
			descriptions = normDescriptions;
			originKey = KeyBinding.Speed;
			buttons = new ButtonWidget[descriptions.Length + 2];
			MakeKeys( KeyBinding.Speed, 0, 6, -150 );
			MakeKeys( KeyBinding.HideFps, 6, 7, 150 );
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