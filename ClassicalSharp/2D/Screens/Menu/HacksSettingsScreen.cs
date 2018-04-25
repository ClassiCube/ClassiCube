// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class HacksSettingsScreen : MenuOptionsScreen {
		
		public HacksSettingsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			validators = new MenuInputValidator[widgets.Length];
			defaultValues = new string[widgets.Length];
			
			validators[1]    = new RealValidator(0.1f, 50);
			defaultValues[1] = "10";
			validators[3]    = new RealValidator(0.1f, 2048f);
			defaultValues[3] = (1.233f).ToString();
			validators[9]    = new IntegerValidator(1, 150);
			defaultValues[9] = "70";
			
			MakeDescriptions();
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;	
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
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnInputClick;
			ClickHandler onBool = OnBoolClick;
			
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Hacks enabled",      onBool,  GetHacks,    SetHacks),
				MakeOpt(-1, -100, "Speed multiplier",   onClick, GetSpeed,    SetSpeed),
				MakeOpt(-1, -50, "Camera clipping",     onBool,  GetClipping, SetClipping),
				MakeOpt(-1, 0, "Jump height",           onClick, GetJump,     SetJump),
				MakeOpt(-1, 50, "WOM style hacks",      onBool,  GetWOMHacks, SetWOMHacks),

				MakeOpt(1, -150, "Full block stepping", onBool,  GetFullStep, SetFullStep),
				MakeOpt(1, -100, "Modifiable liquids",  onBool,  GetLiquids,  SetLiquids),
				MakeOpt(1, -50, "Pushback placing",     onBool,  GetPushback, SetPushback),
				MakeOpt(1, 0, "Noclip slide",           onBool,  GetSlide,    SetSlide),
				MakeOpt(1, 50, "Field of view",         onClick, GetFOV,      SetFOV),
								
				MakeBack(false, titleFont, SwitchOptions),
				null, null, null,
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
			PhysicsComponent physics = g.LocalPlayer.physics;
			physics.CalculateJumpVelocity(Utils.ParseDecimal(v));
			physics.userJumpVel = physics.jumpVel;
			Options.Set(OptionsKey.JumpVelocity, physics.jumpVel.ToString());
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
		
		static string GetLiquids(Game g) { return GetBool(g.BreakableLiquids); }
		static void SetLiquids(Game g, string v) { 
			g.BreakableLiquids = SetBool(v, OptionsKey.ModifiableLiquids); 
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
		
		void MakeDescriptions() {
			string[][] descs = new string[widgets.Length][];
			descs[2] = new string[] {
				"&eIf &fON&e, then the third person cameras will limit",
				"&etheir zoom distance if they hit a solid block.",
			};
			descs[3] = new string[] {
				"&eSets how many blocks high you can jump up.",
				"&eNote: You jump much higher when holding down the Speed key binding.",
			};
			descs[6] = new string[] {
				"&eIf &fON&e, then water/lava can be placed and",
				"&edeleted the same way as any other block.",
			};
			descs[7] = new string[] {
				"&eIf &fON&e, placing blocks that intersect your own position cause",
				"&ethe block to be placed, and you to be moved out of the way.",
				"&fThis is mainly useful for quick pillaring/towering.",
			};
			descs[8] = new string[] {
				"&eIf &fOFF&e, you will immediately stop when in noclip",
				"&emode and no movement keys are held down.",
			};
			descriptions = descs;
		}
	}
}