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
				MakeOpt(-1, -150, "Music",                                           onClick, GetMusic,    SetMusic),
				MakeBool(-1, -100, "Invert mouse", OptionsKey.InvertMouse,           onClick, GetInvert,   SetInvert),
				MakeOpt(-1, -50, "View distance",                                    onClick, GetViewDist, SetViewDist),
				multi ? null : MakeBool(-1, 0, "Block physics", OptionsKey.Physics,  onClick, GetPhysics,  SetPhysics),
				
				MakeOpt(1, -150, "Sound",                                            onClick, GetSounds,   SetSounds),
				MakeBool(1, -100, "Show FPS", OptionsKey.ShowFPS,                    onClick, GetShowFPS,  SetShowFPS),
				MakeBool(1, -50, "View bobbing", OptionsKey.ViewBobbing,             onClick, GetViewBob,  SetViewBob),
				MakeOpt(1, 0, "FPS mode",                                            onClick, GetFPS,      SetFPS),
				!hacks ? null : MakeBool(0, 60, "Hacks enabled", OptionsKey.HacksOn, onClick, GetHacks,    SetHacks),
				
				ButtonWidget.Create(game, 400, "Controls", titleFont, LeftOnly(SwitchClassic))
					.SetLocation(Anchor.Centre, Anchor.BottomOrRight, 0, 95),
				MakeBack(400, "Done", 25, titleFont, SwitchPause),
				null, null,
			};
		}
		
		static string GetMusic(Game g) { return GetBool(g.MusicVolume > 0); }
		static void SetMusic(Game g, string v) {
			g.MusicVolume = v == "ON" ? 100 : 0; 
			g.AudioPlayer.SetMusic(g.MusicVolume); 
			Options.Set(OptionsKey.MusicVolume, g.MusicVolume);
		}
		
		static string GetInvert(Game g) { return GetBool(g.InvertMouse); }
		static void SetInvert(Game g, bool v) { g.InvertMouse = v; }
		
		static string GetViewDist(Game g) { return g.ViewDistance.ToString(); }
		static void SetViewDist(Game g, string v) { g.SetViewDistance(Int32.Parse(v), true); }
		
		static string GetPhysics(Game g) { return GetBool(((SinglePlayerServer)g.Server).physics.Enabled); }
		static void SetPhysics(Game g, bool v) { ((SinglePlayerServer)g.Server).physics.Enabled = v; }
		
		static string GetSounds(Game g) { return GetBool(g.SoundsVolume > 0); }
		static void SetSounds(Game g, string v) { 
			g.SoundsVolume = v == "ON" ? 100 : 0; 
			g.AudioPlayer.SetSounds(g.SoundsVolume);
			Options.Set(OptionsKey.SoundsVolume, g.SoundsVolume);
		}
		
		static string GetShowFPS(Game g) { return GetBool(g.ShowFPS); }
		static void SetShowFPS(Game g, bool v) { g.ShowFPS = v; }
		
		static string GetViewBob(Game g) { return GetBool(g.ViewBobbing); }
		static void SetViewBob(Game g, bool v) { g.ViewBobbing = v; }
		
		static string GetHacks(Game g) { return GetBool(g.LocalPlayer.Hacks.Enabled); }
		static void SetHacks(Game g, bool v) { 
			g.LocalPlayer.Hacks.Enabled = v; 
			g.LocalPlayer.CheckHacksConsistency();
		}
		
		static void SwitchClassic(Game g, Widget w) { g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g)); }
		
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