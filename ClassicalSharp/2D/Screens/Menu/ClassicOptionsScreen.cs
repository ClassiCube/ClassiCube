// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public class ClassicOptionsScreen : MenuOptionsScreen {
		
		public ClassicOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			ContextRecreated();
			MakeValidators();
		}
		
		protected override void ContextRecreated() {
			bool multi = !game.Server.IsSinglePlayer, hacks = game.ClassicHacks;
			ClickHandler onClick = OnWidgetClick;			
			widgets = new Widget[] {
				MakeVolumeBool(-1, -150, "Music", OptionsKey.MusicVolume,                     GetMusic,    SetMusic),				
				MakeBool(-1, -100, "Invert mouse", OptionsKey.InvertMouse,           onClick, GetInvert,   SetInvert),				
				MakeOpt(-1, -50, "View distance",                                    onClick, GetViewDist, SetViewDist),			
				multi ? null : MakeBool(-1, 0, "Block physics", OptionsKey.Physics,  onClick, GetPhysics,  SetPhysics),
				
				MakeVolumeBool(1, -150, "Sound", OptionsKey.SoundsVolume,                     GetSounds,  SetSounds),
				MakeBool(1, -100, "Show FPS", OptionsKey.ShowFPS,                    onClick, GetShowFPS, SetShowFPS),				
				MakeBool(1, -50, "View bobbing", OptionsKey.ViewBobbing,             onClick, GetViewBob, SetViewBob),				
				MakeOpt(1, 0, "FPS mode",                                            onClick, GetFPS,     SetFPS),				
				!hacks ? null : MakeBool(0, 60, "Hacks enabled", OptionsKey.HacksOn, onClick, GetHacks, SetHacks),
				
				ButtonWidget.Create(game, 400, "Controls", titleFont, LeftOnly(SwitchClassic))
					.SetLocation(Anchor.Centre, Anchor.BottomOrRight, 0, 95),				
				MakeBack(400, "Done", 25, titleFont, SwitchPause),
				null, null,
			};
			
			// NOTE: we need to override the default setter here, because changing FPS limit method
			// recreates the graphics context on some backends (such as Direct3D9)
			ButtonWidget btn = (ButtonWidget)widgets[7];
			btn.SetValue = SetFPSLimitMethod;
		}
		
		static string GetMusic(Game g) { return GetBool(g.MusicVolume > 0); }
		static void SetMusic(Game g, bool v) { g.MusicVolume = v ? 100 : 0; g.AudioPlayer.SetMusic(g.MusicVolume); }
		
		static string GetInvert(Game g) { return GetBool(g.InvertMouse); }
		static void SetInvert(Game g, bool v) { g.InvertMouse = v; }
		
		static string GetViewDist(Game g) { return g.ViewDistance.ToString(); }
		static void SetViewDist(Game g, string v) { g.SetViewDistance(Int32.Parse(v), true); }
		
		static string GetPhysics(Game g) { return GetBool(((SinglePlayerServer)g.Server).physics.Enabled); }
		static void SetPhysics(Game g, bool v) { ((SinglePlayerServer)g.Server).physics.Enabled = v; }
		
		static string GetSounds(Game g) { return GetBool(g.SoundsVolume > 0); }
		static void SetSounds(Game g, bool v) { g.SoundsVolume = v ? 100 : 0; g.AudioPlayer.SetSounds(g.SoundsVolume); }
		
		static string GetShowFPS(Game g) { return GetBool(g.ShowFPS); }
		static void SetShowFPS(Game g, bool v) { g.ShowFPS = v; }
		
		static string GetViewBob(Game g) { return GetBool(g.ViewBobbing); }
		static void SetViewBob(Game g, bool v) { g.ViewBobbing = v; }
		
		static string GetFPS(Game g) { return g.FpsLimit.ToString(); }
		static void SetFPS(Game g, string v) { }
		
		static string GetHacks(Game g) { return GetBool(g.LocalPlayer.Hacks.Enabled); }
		static void SetHacks(Game g, bool v) { g.LocalPlayer.Hacks.Enabled = v; g.LocalPlayer.CheckHacksConsistency(); }
		
		static void SwitchClassic(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g)); }
		
		ButtonWidget MakeVolumeBool(int dir, int y, string text, string optKey,
		                            ButtonValueGetter getter, ButtonBoolSetter setter) {
			string optName = text;
			text = text + ": " + getter;
			ButtonWidget widget = ButtonWidget.Create(game, 300, text, titleFont, OnWidgetClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			widget.OptName = optName;
			widget.GetValue = getter;
			
			widget.SetValue = (g, v) => {
				setter(g, v == "ON");
				Options.Set(optKey, v == "ON" ? 100 : 0);
				widget.SetText(widget.OptName + ": " + v);
			};
			return widget;
		}
		
		void MakeValidators() {
			IServerConnection network = game.Server;
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator(16, 4096),
				network.IsSinglePlayer ? new BooleanValidator() : null,
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new EnumValidator(typeof(FpsLimitMethod)),
				game.ClassicHacks ? new BooleanValidator() : null,
			};
		}
	}
}