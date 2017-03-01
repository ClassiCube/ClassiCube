// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class PauseScreen : MenuScreen {
		
		public PauseScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			if (game.UseClassicOptions) {
				MakeClassic();
			} else {
				MakeNormal();
			}
			
			if (!game.Server.IsSinglePlayer) {
				widgets[1].Disabled = true;
				widgets[2].Disabled = true;
			}
			CheckHacksAllowed(null, null);
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				Make(-1, -50, "Options",
				     (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g))),
				Make(1, -50, "Generate level",
				     (g, w) => g.Gui.SetNewScreen(new GenLevelScreen(g))),
				Make(1, 0, "Load level",
				     (g, w) => g.Gui.SetNewScreen(new LoadLevelScreen(g))),
				Make(1, 50, "Save level",
				     (g, w) => g.Gui.SetNewScreen(new SaveLevelScreen(g))),
				Make(-1, 0, "Select texture pack",
				     (g, w) => g.Gui.SetNewScreen(new TexturePackScreen(g))),
				#if !ANDROID
				Make(-1, 50, "Hotkeys",
				     (g, w) => g.Gui.SetNewScreen(new HotkeyListScreen(g))),
				#else
				null,
				#endif
				
				// Other
				ButtonWidget.Create(game, 120, "Quit game", titleFont, LeftOnly((g, w) => g.Exit()))
					.SetLocation(Anchor.BottomOrRight, Anchor.BottomOrRight, 5, 5),
				MakeBack(true, titleFont, (g, w) => g.Gui.SetNewScreen(null)),
			};
		}
		
		void MakeClassic() {
			widgets = new Widget[] {
				MakeClassic(0, -100, "Options",
				            (g, w) => g.Gui.SetNewScreen(new ClassicOptionsScreen(g))),
				MakeClassic(0, -50, "Generate level",
				            (g, w) => g.Gui.SetNewScreen(new GenLevelScreen(g))),
				MakeClassic(0, 0, "Load level",
				            (g, w) => g.Gui.SetNewScreen(new LoadLevelScreen(g))),
				MakeClassic(0, 50, "Save level",
				            (g, w) => g.Gui.SetNewScreen(new SaveLevelScreen(g))),

				MakeBack(400, "Back to game", 25, titleFont, (g, w) => g.Gui.SetNewScreen(null)),
				
				game.ClassicMode ? null :
					MakeClassic(0, 150, "Nostalgia options",
					            (g, w) => g.Gui.SetNewScreen(new NostalgiaScreen(g))),
			};
		}
		
		void CheckHacksAllowed(object sender, EventArgs e) {
			if (game.UseClassicOptions) return;
			widgets[4].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // select texture pack
		}
		
		ButtonWidget Make(int dir, int y, string text, Action<Game, Widget> onClick) {
			return ButtonWidget.Create(game, 300, text, titleFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, dir * 160, y);
		}
		
		ButtonWidget MakeClassic(int x, int y, string text, Action<Game, Widget> onClick) {
			return ButtonWidget.Create(game, 400, text, titleFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, x, y);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape)
				game.Gui.SetNewScreen(null);
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
	}
}