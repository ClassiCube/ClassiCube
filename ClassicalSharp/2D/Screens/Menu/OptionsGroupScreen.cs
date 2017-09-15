// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class OptionsGroupScreen : MenuScreen {
		
		public OptionsGroupScreen(Game game) : base(game) {
		}
		
		TextWidget descWidget;
		string descText;
		ButtonWidget selectedWidget;
		
		public override void Render(double delta) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderWidgets(widgets, delta);
			if (descWidget != null)
				descWidget.Render(delta);
			gfx.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16);
			ContextRecreated();
		}
		
		protected override void ContextRecreated() {
			widgets = new Widget[] {
				Make(-1, -100, "Misc options", SwitchMiscOptions),
				Make(-1, -50, "Gui options", SwitchGuiOptions),
				Make(-1, 0, "Graphics options", SwitchGfxOptions),
				Make(-1, 50, "Controls", SwitchControls),
				Make(1, -50, "Hacks settings", SwitchHacksOptions),
				Make(1, 0, "Env settings", SwitchEnvOptions),
				Make(1, 50, "Nostalgia options", SwitchNostalgiaOptions),				
				MakeBack(false, titleFont, SwitchPause),
			};
			
			if (descWidget != null) MakeDescWidget(descText);		
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
			if (game.UseClassicOptions) return;
			widgets[5].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // env settings
		}
		
		protected override void WidgetSelected(Widget widget) {
			ButtonWidget button = widget as ButtonWidget;
			if (selectedWidget == widget || button == null ||
			   button == widgets[widgets.Length - 1]) return;
			
			selectedWidget = button;
			if (descWidget != null) descWidget.Dispose();
			if (button == null) return;
			
			string text = descriptions[IndexOfWidget(button)];
			MakeDescWidget(text);
		}
		
		void MakeDescWidget(string text) {
			descWidget = TextWidget.Create(game, text, regularFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100);
			descText = text;
		}
		
		ButtonWidget Make(int dir, int y, string text, SimpleClickHandler onClick) {
			return ButtonWidget.Create(game, 300, text, titleFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, dir * 160, y);
		}
		
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape)
				game.Gui.SetNewScreen(null);
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void OnResize(int width, int height) {
			if (descWidget != null)
				descWidget.Reposition();
			base.OnResize(width, height);
		}
		
		protected override void ContextLost() {
			base.ContextLost();
			if (descWidget != null) descWidget.Dispose();
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
		
		static string[] descriptions = {
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