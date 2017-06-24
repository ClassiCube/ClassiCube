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
			IServerConnection network = game.Server;
			
			widgets = new Widget[] {
				// Column 1
				MakeVolumeBool(-1, -150, "Music", OptionsKey.MusicVolume,
				        g => g.MusicVolume > 0,
				        (g, v) => { g.MusicVolume = v ? 100 : 0; g.AudioPlayer.SetMusic(g.MusicVolume); }),
				
				MakeBool(-1, -100, "Invert mouse", OptionsKey.InvertMouse,
				         OnWidgetClick, g => g.InvertMouse, (g, v) => g.InvertMouse = v),
				
				MakeOpt(-1, -50, "View distance", OnWidgetClick,
				        g => g.ViewDistance.ToString(),
				        (g, v) => g.SetViewDistance(Int32.Parse(v), true)),
				
				!network.IsSinglePlayer ? null :
					MakeBool(-1, 0, "Block physics", OptionsKey.SingleplayerPhysics, OnWidgetClick,
					         g => ((SinglePlayerServer)network).physics.Enabled,
					         (g, v) => ((SinglePlayerServer)network).physics.Enabled = v),
				
				// Column 2
				MakeVolumeBool(1, -150, "Sound", OptionsKey.SoundsVolume,
				        g => g.SoundsVolume > 0,
				        (g, v) => { g.SoundsVolume = v ? 100 : 0; g.AudioPlayer.SetSounds(g.SoundsVolume); }),
				
				MakeBool(1, -100, "Show FPS", OptionsKey.ShowFPS,
				         OnWidgetClick, g => g.ShowFPS, (g, v) => g.ShowFPS = v),
				
				MakeBool(1, -50, "View bobbing", OptionsKey.ViewBobbing,
				         OnWidgetClick, g => g.ViewBobbing, (g, v) => g.ViewBobbing = v),
				
				MakeOpt(1, 0, "FPS mode", OnWidgetClick,
				        g => g.FpsLimit.ToString(),
				        (g, v) => { }),
				
				!game.ClassicHacks ? null :
					MakeBool(0, 60, "Hacks enabled", OptionsKey.HacksEnabled,
					         OnWidgetClick, g => g.LocalPlayer.Hacks.Enabled,
					         (g, v) => { g.LocalPlayer.Hacks.Enabled = v;
					         	g.LocalPlayer.CheckHacksConsistency(); }),
				
				ButtonWidget.Create(game, 400, "Controls", titleFont,
				                    LeftOnly((g, w) => g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g))))
					.SetLocation(Anchor.Centre, Anchor.BottomOrRight, 0, 95),
				
				MakeBack(400, "Done", 25, titleFont, (g, w) => g.Gui.SetNewScreen(new PauseScreen(g))),
				null, null,
			};
			
			// NOTE: we need to override the default setter here, because changing FPS limit method
			// recreates the graphics context on some backends (such as Direct3D9)
			ButtonWidget btn = (ButtonWidget)widgets[7];
			btn.SetValue = SetFPSLimitMethod;
		}
		
		ButtonWidget MakeVolumeBool(int dir, int y, string text, string optKey,
		                            ButtonBoolGetter getter, ButtonBoolSetter setter) {
			string optName = text;
			text = text + ": " + (getter(game) ? "ON" : "OFF");
			ButtonWidget widget = ButtonWidget.Create(game, 300, text, titleFont, OnWidgetClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 160 * dir, y);
			widget.Metadata = optName;
			widget.GetValue = g => getter(g) ? "yes" : "no";
			
			widget.SetValue = (g, v) => {
				setter(g, v == "yes");
				Options.Set(optKey, v == "yes" ? 100 : 0);
				widget.SetText((string)widget.Metadata + ": " + (v == "yes" ? "ON" : "OFF"));
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