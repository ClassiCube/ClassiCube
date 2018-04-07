// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class DeathScreen : MenuScreen {
		
		public DeathScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			textFont = new Font(game.FontName, 40);
			base.Init();
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			string score = game.Chat.Status1.Text;
			widgets = new Widget[] {
				TextWidget.Create(game, "Game over!", textFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -150),
				TextWidget.Create(game, score, titleFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, -75),
				ButtonWidget.Create(game, 400, "Generate new level...", titleFont, GenLevelClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 25),
				ButtonWidget.Create(game, 400, "Load level...", titleFont, LoadLevelClick)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 75),
			};
		}
		
		public override bool HandlesKeyDown(Key key) { return true; }

		void GenLevelClick(Game g, Widget w) {
			game.Gui.SetNewScreen(new GenLevelScreen(game));
		}
		
		void LoadLevelClick(Game g, Widget w) {
			game.Gui.SetNewScreen(new LoadLevelScreen(game));
		}
	}
}
