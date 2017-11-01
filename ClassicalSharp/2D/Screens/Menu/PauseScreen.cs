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
				Make(-1, -50, "Options...",             SwitchOptions),
				Make( 1, -50, "Generate new level...",  SwitchGenLevel),
				Make( 1,   0, "Load level...",          SwitchLoadLevel),
				Make( 1,  50, "Save level...",          SwitchSaveLevel),
				Make(-1,   0, "Change texture pack...", SwitchTexPack),
				#if !ANDROID
				Make(-1,  50, "Hotkeys...",             SwitchHotkeys),
				#else
				null,
				#endif
				
				// Other
				ButtonWidget.Create(game, 120, "Quit game", titleFont, LeftOnly(QuitGame))
					.SetLocation(Anchor.BottomOrRight, Anchor.BottomOrRight, 5, 5),
				MakeBack(true, titleFont, SwitchGame),
			};
		}
		
		void MakeClassic() {
			widgets = new Widget[] {
				MakeClassic(0, -100, "Options...",            SwitchClassicOptions),
				MakeClassic(0,  -50, "Generate new level...", SwitchClassicGenLevel),
				MakeClassic(0,    0, "Load level...",         SwitchLoadLevel),
				MakeClassic(0,   50, "Save level...",         SwitchSaveLevel),
				
				game.ClassicMode ? null :
					MakeClassic(0, 150, "Nostalgia options...", SwitchNostalgiaOptions),				
				MakeBack(400, "Back to game", 25, titleFont, SwitchGame),
			};
		}
		
		static void SwitchGenLevel(Game g, Widget w) { g.Gui.SetNewScreen(new GenLevelScreen(g)); }
		static void SwitchClassicGenLevel(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicGenLevelScreen(g)); }
		static void SwitchLoadLevel(Game g, Widget w) { g.Gui.SetNewScreen(new LoadLevelScreen(g)); }
		static void SwitchSaveLevel(Game g, Widget w) { g.Gui.SetNewScreen(new SaveLevelScreen(g)); }
		static void SwitchTexPack(Game g, Widget w) { g.Gui.SetNewScreen(new TexturePackScreen(g)); }
		static void SwitchHotkeys(Game g, Widget w) { g.Gui.SetNewScreen(new HotkeyListScreen(g)); }
		static void SwitchNostalgiaOptions(Game g, Widget w) { g.Gui.SetNewScreen(new NostalgiaScreen(g)); }
		static void SwitchGame(Game g, Widget w) { g.Gui.SetNewScreen(null); }
		static void SwitchClassicOptions(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicOptionsScreen(g)); }
		static void QuitGame(Game g, Widget w) { g.Exit(); }
		
		void CheckHacksAllowed(object sender, EventArgs e) {
			if (game.UseClassicOptions) return;
			widgets[4].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // select texture pack
		}
		
		ButtonWidget Make(int dir, int y, string text, SimpleClickHandler onClick) {
			return ButtonWidget.Create(game, 300, text, titleFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, dir * 160, y);
		}
		
		ButtonWidget MakeClassic(int x, int y, string text, SimpleClickHandler onClick) {
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