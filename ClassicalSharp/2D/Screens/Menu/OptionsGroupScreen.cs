// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class OptionsGroupScreen : MenuScreen {
		
		public OptionsGroupScreen(Game game) : base(game) {
		}
		
		int selectedI = -1;
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				Make(-1, -100, "Misc options...",      SwitchMiscOptions),
				Make(-1,  -50, "Gui options...",       SwitchGuiOptions),
				Make(-1,    0, "Graphics options...",  SwitchGfxOptions),
				Make(-1,   50, "Controls...",          SwitchControls),
				Make( 1,  -50, "Hacks settings...",    SwitchHacksOptions),
				Make( 1,    0, "Env settings...",      SwitchEnvOptions),
				Make( 1,   50, "Nostalgia options...", SwitchNostalgiaOptions),
				MakeBack(false, titleFont, SwitchPause),
				null, // description text widget placeholder
			};
			
			if (selectedI >= 0) MakeDesc();
			CheckHacksAllowed(null, null);
		}
		
		static void SwitchMiscOptions(Game g, Widget w) { g.Gui.SetNewScreen(new MiscOptionsScreen(g)); }
		static void SwitchGuiOptions(Game g, Widget w) { g.Gui.SetNewScreen(new GuiOptionsScreen(g)); }
		static void SwitchGfxOptions(Game g, Widget w) { g.Gui.SetNewScreen(new GraphicsOptionsScreen(g)); }
		static void SwitchControls(Game g, Widget w) { g.Gui.SetNewScreen(new NormalKeyBindingsScreen(g)); }
		static void SwitchHacksOptions(Game g, Widget w) { g.Gui.SetNewScreen(new HacksSettingsScreen(g)); }
		static void SwitchEnvOptions(Game g, Widget w) { g.Gui.SetNewScreen(new EnvSettingsScreen(g)); }
		static void SwitchNostalgiaOptions(Game g, Widget w) { g.Gui.SetNewScreen(new NostalgiaScreen(g)); }
		
		void CheckHacksAllowed(object sender, EventArgs e) {
			widgets[5].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // env settings
		}
		
		void MakeDesc() {
			string text = descriptions[selectedI];
			widgets[widgets.Length - 1] = TextWidget.Create(game, text, textFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100);
		}
		
		ButtonWidget Make(int dir, int y, string text, ClickHandler onClick) {
			return ButtonWidget.Create(game, 300, text, titleFont, onClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, dir * 160, y);
		}	
		
		public override bool HandlesMouseMove(int mouseX, int mouseY) {
			int i = HandleMouseMove(widgets, mouseX, mouseY);
			if (i == -1 || i == selectedI) return true;
			if (i >= descriptions.Length)  return true;
			
			selectedI = i;
			Widget desc = widgets[widgets.Length - 1];
			if (desc != null) desc.Dispose();
			MakeDesc();
			return true;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
		
		static string[] descriptions = new string[] {
			"&eMusic/Sound, view bobbing, and more",
			"&eChat options, gui scale, font settings, and more",
			"&eFPS limit, view distance, entity names/shadows",
			"&eSet key bindings, bind keys to act as mouse clicks",
			"&eHacks allowed, jump settings, and more",
			"&eEnv colours, water level, weather, and more",
			"&eSettings for resembling the original classic",
		};
	}
}