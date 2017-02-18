﻿// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
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
				MakeBool(-1, -150, "Music", OptionsKey.UseMusic,
				         OnWidgetClick, g => g.UseMusic,
				         (g, v) => { g.UseMusic = v; g.AudioPlayer.SetMusic(g.UseMusic); }),
				
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
				MakeBool(1, -150, "Sound", OptionsKey.UseSound,
				         OnWidgetClick, g => g.UseSound,
				         (g, v) => { g.UseSound = v; g.AudioPlayer.SetSound(g.UseSound); }),
				
				MakeBool(1, -100, "Show FPS", OptionsKey.ShowFPS,
				         OnWidgetClick, g => g.ShowFPS, (g, v) => g.ShowFPS = v),
				
				MakeBool(1, -50, "View bobbing", OptionsKey.ViewBobbing,
				         OnWidgetClick, g => g.ViewBobbing, (g, v) => g.ViewBobbing = v),
				
				MakeOpt(1, 0, "FPS mode", OnWidgetClick,
				        g => g.FpsLimit.ToString(),
				        (g, v) => { object raw = Enum.Parse(typeof(FpsLimitMethod), v);
				        	g.SetFpsLimitMethod((FpsLimitMethod)raw);
				        	Options.Set(OptionsKey.FpsLimit, v); }),
				
				!game.ClassicHacks ? null :
					MakeBool(0, 60, "Hacks enabled", OptionsKey.HacksEnabled,
					         OnWidgetClick, g => g.LocalPlayer.Hacks.Enabled,
					         (g, v) => { g.LocalPlayer.Hacks.Enabled = v;
					         	g.LocalPlayer.CheckHacksConsistency(); }),
				
				ButtonWidget.Create(game, 400, "Controls", titleFont,
				                    LeftOnly((g, w) => g.Gui.SetNewScreen(new ClassicKeyBindingsScreen(g))))
					.SetLocation(Anchor.Centre, Anchor.BottomOrRight, 0, 95),
				
				MakeBack(400, "Done", 22, titleFont, (g, w) => g.Gui.SetNewScreen(new PauseScreen(g))),
				null, null,
			};
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