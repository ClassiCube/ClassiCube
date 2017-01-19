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
		ButtonWidget selectedWidget;
		
		public override void Render(double delta) {
			RenderMenuBounds();
			gfx.Texturing = true;
			RenderMenuWidgets(delta);
			if (descWidget != null)
				descWidget.Render(delta);
			gfx.Texturing = false;
		}
		
		public override void Init() {
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			titleFont = new Font(game.FontName, 16, FontStyle.Bold);
			regularFont = new Font(game.FontName, 16, FontStyle.Regular);
			
			MakeNormal();
			CheckHacksAllowed(null, null);
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				Make(-1, -100, "Misc options",
				     (g, w) => g.Gui.SetNewScreen(new MiscOptionsScreen(g))),
				Make(-1, -50, "Gui options",
				     (g, w) => g.Gui.SetNewScreen(new GuiOptionsScreen(g))),
				Make(-1, 0, "Graphics options",
				     (g, w) => g.Gui.SetNewScreen(new GraphicsOptionsScreen(g))),
				Make(-1, 50, "Controls",
				     (g, w) => g.Gui.SetNewScreen(new NormalKeyBindingsScreen(g))),
				Make(1, -50, "Hacks settings",
				     (g, w) => g.Gui.SetNewScreen(new HacksSettingsScreen(g))),
				Make(1, 0, "Env settings",
				     (g, w) => g.Gui.SetNewScreen(new EnvSettingsScreen(g))),
				Make(1, 50, "Nostalgia options",
				     (g, w) => g.Gui.SetNewScreen(new NostalgiaScreen(g))),
				
				MakeBack(false, titleFont, 
				         (g, w) => g.Gui.SetNewScreen(new PauseScreen(g))),
			};
		}
		
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
			
			string text = descriptions[Array.IndexOf<Widget>(widgets, button)];
			descWidget = TextWidget.Create(game, text, regularFont)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100);
		}
		
		ButtonWidget Make(int dir, int y, string text, Action<Game, Widget> onClick) {
			return ButtonWidget.Create(game, 301, 40, text, titleFont, LeftOnly(onClick))
				.SetLocation(Anchor.Centre, Anchor.Centre, dir * 160, y);
		}
		
		public override bool HandlesKeyDown(Key key) {
			if (key == Key.Escape)
				game.Gui.SetNewScreen(null);
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void OnResize(int width, int height) {
			if (descWidget != null)
				descWidget.CalculatePosition();
			base.OnResize(width, height);
		}
		
		public override void Dispose() {
			if (descWidget != null)
				descWidget.Dispose();
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