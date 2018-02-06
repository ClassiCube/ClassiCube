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
			ContextRecreated();
			
			MakeDefaultValues();
			MakeValidators();
			MakeDescriptions();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnWidgetClick;
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Hacks enabled",      onClick, GetHacks,    SetHacks),
				MakeOpt(-1, -100, "Speed multiplier",   onClick, GetSpeed,    SetSpeed),
				MakeOpt(-1, -50, "Camera clipping",     onClick, GetClipping, SetClipping),
				MakeOpt(-1, 0, "Jump height",           onClick, GetJump,     SetJump),
				MakeOpt(-1, 50, "WOM style hacks",      onClick, GetWOMHacks, SetWOMHacks),

				MakeOpt(1, -150, "Full block stepping", onClick, GetFullStep, SetFullStep),
				MakeOpt(1, -100, "Modifiable liquids",  onClick, GetLiquids,  SetLiquids),
				MakeOpt(1, -50, "Pushback placing",     onClick, GetPushback, SetPushback),
				MakeOpt(1, 0, "Noclip slide",           onClick, GetSlide,    SetSlide),
				MakeOpt(1, 50, "Field of view",         onClick, GetFOV,      SetFOV),
				
				null,
				MakeBack(false, titleFont, SwitchOptions),
				null, null,
			};
			CheckHacksAllowed(null, null);
		}
		
		static string GetHacks(Game g) { return GetBool(g.LocalPlayer.Hacks.Enabled); }
		static void SetHacks(Game g, string v) { 
			g.LocalPlayer.Hacks.Enabled = SetBool(v, OptionsKey.HacksOn);
			g.LocalPlayer.CheckHacksConsistency();
		}
		
		static string GetSpeed(Game g) { return g.LocalPlayer.Hacks.SpeedMultiplier.ToString("F2"); }
		static void SetSpeed(Game g, string v) {
			g.LocalPlayer.Hacks.SpeedMultiplier = Utils.ParseDecimal(v); 
			Options.Set(OptionsKey.Speed, v);
		}
		
		static string GetClipping(Game g) { return GetBool(g.CameraClipping); }
		static void SetClipping(Game g, string v) { 
			g.CameraClipping = SetBool(v, OptionsKey.CameraClipping); 
		}
		
		static string GetJump(Game g) { return g.LocalPlayer.JumpHeight.ToString("F3"); }
		static void SetJump(Game g, string v) {
			g.LocalPlayer.physics.CalculateJumpVelocity(true, Utils.ParseDecimal(v));
			float jumpVel = g.LocalPlayer.physics.jumpVel;
			Options.Set(OptionsKey.JumpVelocity, jumpVel.ToString());
		}		
		
		static string GetWOMHacks(Game g) { return GetBool(g.LocalPlayer.Hacks.WOMStyleHacks); }
		static void SetWOMHacks(Game g, string v) { 
			g.LocalPlayer.Hacks.WOMStyleHacks = SetBool(v, OptionsKey.WOMStyleHacks); 
		}
		
		static string GetFullStep(Game g) { return GetBool(g.LocalPlayer.Hacks.FullBlockStep); }
		static void SetFullStep(Game g, string v) { 
			g.LocalPlayer.Hacks.FullBlockStep = SetBool(v, OptionsKey.FullBlockStep); 
		}
		
		static string GetPushback(Game g) { return GetBool(g.LocalPlayer.Hacks.PushbackPlacing); }
		static void SetPushback(Game g, string v) { 
			g.LocalPlayer.Hacks.PushbackPlacing = SetBool(v, OptionsKey.PushbackPlacing); 
		}
		
		static string GetLiquids(Game g) { return GetBool(g.ModifiableLiquids); }
		static void SetLiquids(Game g, string v) { 
			g.ModifiableLiquids = SetBool(v, OptionsKey.ModifiableLiquids); 
		}
		
		static string GetSlide(Game g) { return GetBool(g.LocalPlayer.Hacks.NoclipSlide); }
		static void SetSlide(Game g, string v) { 
			g.LocalPlayer.Hacks.NoclipSlide = SetBool(v, OptionsKey.NoclipSlide); 
		}
		
		static string GetFOV(Game g) { return g.Fov.ToString(); }
		static void SetFOV(Game g, string v) {
			g.Fov = Int32.Parse(v);
			if (g.ZoomFov > g.Fov) g.ZoomFov = g.Fov;
			
			Options.Set(OptionsKey.FieldOfView, v);
			g.UpdateProjection();
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
				new RealValidator(0.1f, 2048f),
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
			base.InputClosed();
			if (widgets[defaultIndex] != null)
				widgets[defaultIndex].Dispose();
			widgets[defaultIndex] = null;
		}
		
		protected override void InputOpened() {
			widgets[defaultIndex] = ButtonWidget.Create(game, 200, "Default value", titleFont, DefaultButtonClick)
				.SetLocation(Anchor.Centre, Anchor.Centre, 0, 150);
		}
		
		void DefaultButtonClick(Game game, Widget widget, MouseButton btn, int x, int y) {
			if (btn != MouseButton.Left) return;
			int index = IndexOfWidget(activeButton);
			string defValue = defaultValues[index];
			
			input.Clear();
			input.Append(defValue);
		}
	}
}