// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public class ClassicOptionsScreen : MenuOptionsScreen {
		
		enum ViewDist { TINY, SHORT, NORMAL, FAR }
		
		public ClassicOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			validators = new MenuInputValidator[widgets.Length];
			
			validators[2] = new EnumValidator(typeof(ViewDist));
			validators[7] = new EnumValidator(typeof(FpsLimitMethod));
		}
		
		protected override void ContextRecreated() {
			bool multi = !game.Server.IsSinglePlayer, hacks = game.ClassicHacks;
			ClickHandler onEnum = OnEnumClick;
			ClickHandler onBool = OnBoolClick;
			
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Music",                      onBool, GetMusic,    SetMusic),
				MakeOpt(-1, -100, "Invert mouse",               onBool, GetInvert,   SetInvert),
				MakeOpt(-1, -50, "Render distance",             onEnum, GetViewDist, SetViewDist),
				multi ? null : MakeOpt(-1, 0, "Block physics",  onBool, GetPhysics,  SetPhysics),
				
				MakeOpt(1, -150, "Sound",                       onBool, GetSounds,   SetSounds),
				MakeOpt(1, -100, "Show FPS",                    onBool, GetShowFPS,  SetShowFPS),
				MakeOpt(1, -50, "View bobbing",                 onBool, GetViewBob,  SetViewBob),
				MakeOpt(1, 0, "FPS mode",                       onEnum, GetFPS,      SetFPS),
				!hacks ? null : MakeOpt(0, 60, "Hacks enabled", onBool, GetHacks,    SetHacks),
				
				ButtonWidget.Create(game, 400, "Controls...", titleFont, SwitchClassic)
					.SetLocation(Anchor.Centre, Anchor.Max, 0, 95),
				MakeBack(400, "Done", 25, titleFont, SwitchPause),
			};
		}
		
		static string GetMusic(Game g) { return GetBool(g.MusicVolume > 0); }
		static void SetMusic(Game g, string v) {
			g.MusicVolume = v == "ON" ? 100 : 0;
			g.AudioPlayer.SetMusic(g.MusicVolume);
			Options.Set(OptionsKey.MusicVolume, g.MusicVolume);
		}
		
		static string GetInvert(Game g) { return GetBool(g.InvertMouse); }
		static void SetInvert(Game g, string v) { g.InvertMouse = SetBool(v, OptionsKey.InvertMouse); }
		
		static string GetViewDist(Game g) { 
			if (g.ViewDistance >= 512) return ViewDist.FAR.ToString();
			if (g.ViewDistance >= 128) return ViewDist.NORMAL.ToString();
			if (g.ViewDistance >= 32 ) return ViewDist.SHORT.ToString();
			
			return ViewDist.TINY.ToString();
		}
		static void SetViewDist(Game g, string v) {
			ViewDist raw = (ViewDist)Enum.Parse(typeof(ViewDist), v);
			int dist = raw == ViewDist.FAR ? 512 : (raw == ViewDist.NORMAL ? 128 : (raw == ViewDist.SHORT ? 32 : 8));
			g.SetViewDistance(dist, true); 
		}
		
		static string GetPhysics(Game g) { return GetBool(((SinglePlayerServer)g.Server).physics.Enabled); }
		static void SetPhysics(Game g, string v) {
			((SinglePlayerServer)g.Server).physics.Enabled = SetBool(v, OptionsKey.BlockPhysics);
		}
		
		static string GetSounds(Game g) { return GetBool(g.SoundsVolume > 0); }
		static void SetSounds(Game g, string v) {
			g.SoundsVolume = v == "ON" ? 100 : 0;
			g.AudioPlayer.SetSounds(g.SoundsVolume);
			Options.Set(OptionsKey.SoundsVolume, g.SoundsVolume);
		}
		
		static string GetShowFPS(Game g) { return GetBool(g.ShowFPS); }
		static void SetShowFPS(Game g, string v) { g.ShowFPS = SetBool(v, OptionsKey.ShowFPS); }
		
		static string GetViewBob(Game g) { return GetBool(g.ViewBobbing); }
		static void SetViewBob(Game g, string v) { g.ViewBobbing = SetBool(v, OptionsKey.ViewBobbing); }
		
		static string GetHacks(Game g) { return GetBool(g.LocalPlayer.Hacks.Enabled); }
		static void SetHacks(Game g, string v) {
			g.LocalPlayer.Hacks.Enabled = SetBool(v, OptionsKey.HacksOn);
			g.LocalPlayer.CheckHacksConsistency();
		}
		
		static void SwitchClassic(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g)); }
	}
}