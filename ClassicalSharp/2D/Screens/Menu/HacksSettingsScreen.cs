// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class HacksSettingsScreen : MenuOptionsScreen {
		
		public HacksSettingsScreen(Game game) : base(game) {
		}
		
		string[] defaultValues;
		int defaultIndex;
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				MakeBool(-1, -150, "Hacks enabled", OptionsKey.HacksEnabled,
				         OnWidgetClick, g => g.LocalPlayer.Hacks.Enabled,
				         (g, v) => { g.LocalPlayer.Hacks.Enabled = v;
				         	g.LocalPlayer.CheckHacksConsistency(); }),
				
				MakeOpt(-1, -100, "Speed multiplier", OnWidgetClick,
				        g => g.LocalPlayer.Hacks.SpeedMultiplier.ToString("F2"),
				        (g, v) => { g.LocalPlayer.Hacks.SpeedMultiplier = Utils.ParseDecimal(v);
				        	Options.Set(OptionsKey.Speed, v); }),
				
				MakeBool(-1, -50, "Camera clipping", OptionsKey.CameraClipping,
				         OnWidgetClick, g => g.CameraClipping, (g, v) => g.CameraClipping = v),
				
				MakeOpt(-1, 0, "Jump height", OnWidgetClick,
				        g => g.LocalPlayer.JumpHeight.ToString("F3"),
				        (g, v) => g.LocalPlayer.physics.CalculateJumpVelocity(true, Utils.ParseDecimal(v))),
				
				MakeBool(-1, 50, "WOM style hacks", OptionsKey.WOMStyleHacks,
				         OnWidgetClick, g => g.LocalPlayer.Hacks.WOMStyleHacks,
				         (g, v) => g.LocalPlayer.Hacks.WOMStyleHacks = v),
				
				// Column 2
				MakeBool(1, -150, "Full block stepping", OptionsKey.FullBlockStep,
				         OnWidgetClick, g => g.LocalPlayer.Hacks.FullBlockStep,
				         (g, v) => g.LocalPlayer.Hacks.FullBlockStep = v),
				
				MakeBool(1, -100, "Modifiable liquids", OptionsKey.ModifiableLiquids,
				         OnWidgetClick, g => g.ModifiableLiquids, (g, v) => g.ModifiableLiquids = v),
				
				MakeBool(1, -50, "Pushback placing", OptionsKey.PushbackPlacing,
				         OnWidgetClick, g => g.LocalPlayer.Hacks.PushbackPlacing,
				         (g, v) => g.LocalPlayer.Hacks.PushbackPlacing = v),
				
				MakeBool(1, 0, "Noclip slide", OptionsKey.NoclipSlide,
				         OnWidgetClick, g => g.LocalPlayer.Hacks.NoclipSlide,
				         (g, v) => g.LocalPlayer.Hacks.NoclipSlide = v),
				
				MakeOpt(1, 50, "Field of view", OnWidgetClick,
				        g => g.Fov.ToString(),
				        (g, v) => { g.Fov = Int32.Parse(v);
				        	Options.Set(OptionsKey.FieldOfView, v);
				        	g.UpdateProjection();
				        }),
				
				null,
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g))),
				null, null,
			};
			
			MakeDefaultValues();
			MakeValidators();
			MakeDescriptions();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			CheckHacksAllowed(null, null);
		}
		
		void CheckHacksAllowed(object sender, EventArgs e) {
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] == null) continue;
				widgets[i].Disabled = false;
			}
			
			LocalPlayer p = game.LocalPlayer;
			bool noGlobalHacks = !p.Hacks.CanAnyHacks || !p.Hacks.Enabled;
			widgets[3].Disabled = noGlobalHacks || !p.Hacks.CanSpeed;
			widgets[4].Disabled = noGlobalHacks || !p.Hacks.CanSpeed;
			widgets[5].Disabled = noGlobalHacks || !p.Hacks.CanSpeed;
			widgets[7].Disabled = noGlobalHacks || !p.Hacks.CanPushbackBlocks;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
		
		void MakeDefaultValues() {
			defaultIndex = widgets.Length - 4;
			defaultValues = new string[widgets.Length];
			defaultValues[1] = "10";
			defaultValues[3] = (1.233f).ToString();
			defaultValues[9] = "70";
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new RealValidator(0.1f, 50),
				new BooleanValidator(),
				new RealValidator(0.1f, 1024f),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator(1, 150),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[2] = new string[] {
				"&eIf &fON&e, then the third person cameras will limit",
				"&etheir zoom distance if they hit a solid block.",
			};
			descriptions[3] = new string[] {
				"&eSets how many blocks high you can jump up.",
				"&eNote: You jump much higher when holding down the Speed key binding.",
			};
			descriptions[6] = new string[] {
				"&eIf &fON&e, then water/lava can be placed and",
				"&edeleted the same way as any other block.",
			};
			descriptions[7] = new string[] {
				"&eIf &fON&e, placing blocks that intersect your own position cause",
				"&ethe block to be placed, and you to be moved out of the way.",
				"&fThis is mainly useful for quick pillaring/towering.",
			};
			descriptions[8] = new string[] {
				"&eIf &fOFF&e, you will immediately stop when in noclip",
				"&emode and no movement keys are held down.",
			};
		}
		
		protected override void InputClosed() {
			if (widgets[defaultIndex] != null)
				widgets[defaultIndex].Dispose();
			widgets[defaultIndex] = null;
		}
		
		protected override void InputOpened() {
			widgets[defaultIndex] = ButtonWidget.Create(game, 201, 40, "Default value", titleFont, DefaultButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 150);
		}
		
		void DefaultButtonClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			int index = Array.IndexOf<Widget>(widgets, targetWidget);
			string defValue = defaultValues[index];
			
			input.Clear();
			input.Append(defValue);
		}
	}
}