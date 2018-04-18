// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public class MiscOptionsScreen : MenuOptionsScreen {
		
		public MiscOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			validators = new MenuInputValidator[widgets.Length];
			defaultValues = new string[widgets.Length];
			
			validators[0]    = new RealValidator(1, 1024);
			defaultValues[0] = "5";
			validators[1]    = new IntegerValidator(0, 100);
			defaultValues[1] = "0";
			validators[2]    = new IntegerValidator(0, 100);
			defaultValues[2] = "0";
			validators[7]    = new IntegerValidator(1, 200);
			defaultValues[7] = "30";
		}
		
		protected override void ContextRecreated() {
			bool multi = !game.Server.IsSinglePlayer;
			ClickHandler onClick = OnInputClick;
			ClickHandler onBool = OnBoolClick;
			
			widgets = new Widget[] {
				multi ? null : MakeOpt(-1, -100, "Reach distance", onClick, GetReach,       SetReach),
				MakeOpt(-1, -50, "Music volume",                   onClick, GetMusic,       SetMusic),
				MakeOpt(-1, 0, "Sounds volume",                    onClick, GetSounds,      SetSounds),
				MakeOpt(-1, 50, "View bobbing",                    onBool,  GetViewBob,     SetViewBob),

				multi ? null : MakeOpt(1, -100, "Block physics",  onBool,  GetPhysics,     SetPhysics),
				MakeOpt(1, -50, "Auto close launcher",            onBool,  GetAutoClose,   SetAutoClose),
				MakeOpt(1, 0, "Invert mouse",                     onBool,  GetInvert,      SetInvert),
				MakeOpt(1, 50, "Mouse sensitivity",               onClick, GetSensitivity, SetSensitivity),

				MakeBack(false, titleFont, SwitchOptions),
				null, null, null,
			};
		}
		
		static string GetReach(Game g) { return g.LocalPlayer.ReachDistance.ToString(); }
		static void SetReach(Game g, string v) { g.LocalPlayer.ReachDistance = Utils.ParseDecimal(v); }
		
		static string GetMusic(Game g) { return g.MusicVolume.ToString(); }
		static void SetMusic(Game g, string v) {
			g.MusicVolume = Int32.Parse(v);
			Options.Set(OptionsKey.MusicVolume, v);
			g.AudioPlayer.SetMusic(g.MusicVolume);
		}
		
		static string GetSounds(Game g) { return g.SoundsVolume.ToString(); }
		static void SetSounds(Game g, string v) {
			g.SoundsVolume = Int32.Parse(v);
			Options.Set(OptionsKey.SoundsVolume, v);
			g.AudioPlayer.SetSounds(g.SoundsVolume);
		}
		
		static string GetViewBob(Game g) { return GetBool(g.ViewBobbing); }
		static void SetViewBob(Game g, string v) { g.ViewBobbing = SetBool(v, OptionsKey.ViewBobbing); }
		
		static string GetPhysics(Game g) { return GetBool(((SinglePlayerServer)g.Server).physics.Enabled); }
		static void SetPhysics(Game g, string v) { 
			((SinglePlayerServer)g.Server).physics.Enabled = SetBool(v, OptionsKey.BlockPhysics);
		}
		
		static string GetAutoClose(Game g) { return GetBool(Options.GetBool(OptionsKey.AutoCloseLauncher, false)); }
		static void SetAutoClose(Game g, string v) { SetBool(v, OptionsKey.AutoCloseLauncher); }
		
		static string GetInvert(Game g) { return GetBool(g.InvertMouse); }
		static void SetInvert(Game g, string v) { g.InvertMouse = SetBool(v, OptionsKey.InvertMouse); }
		
		static string GetSensitivity(Game g) { return g.MouseSensitivity.ToString(); }
		static void SetSensitivity(Game g, string v) {
			g.MouseSensitivity = Int32.Parse(v);
			Options.Set(OptionsKey.Sensitivity, v);
		}
	}
}