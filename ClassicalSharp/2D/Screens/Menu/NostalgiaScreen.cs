// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public sealed class NostalgiaScreen : MenuOptionsScreen {
		
		public NostalgiaScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			ContextRecreated();
			
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
			};
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnWidgetClick;
			widgets = new Widget[] {
				MakeBool(-1, -150, "Classic hand model", OptionsKey.ClassicArmModel,     onClick, GetHand,   SetHand),
				MakeBool(-1, -100, "Classic walk anim", OptionsKey.SimpleArmsAnim, true, onClick, GetAnim,   SetAnim),
				MakeBool(-1, -50, "Classic gui textures", OptionsKey.UseClassicGui,      onClick, GetGui,    SetGui),
				MakeBool(-1, 0, "Classic player list", OptionsKey.UseClassicTabList,     onClick, GetList,   SetList),
				MakeBool(-1, 50, "Classic options", OptionsKey.UseClassicOptions,        onClick, GetOpts,   SetOpts),				

				MakeBool(1, -150, "Allow custom blocks", OptionsKey.UseCustomBlocks,     onClick, GetCustom, SetCustom),
				MakeBool(1, -100, "Use CPE", OptionsKey.UseCPE,                          onClick, GetCPE,    SetCPE),
				MakeBool(1, -50, "Use server textures", OptionsKey.UseServerTextures,    onClick, GetTexs,   SetTexs),

				TextWidget.Create(game, "&eButtons on the right require a client restart", regularFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100),
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(PreviousScreen())),
				null, null,
			};
		}
		
		static bool GetHand(Game g) { return  g.ClassicArmModel; }
		static void SetHand(Game g, bool v) { g.ClassicArmModel = v; }
		
		static bool GetAnim(Game g) { return  !g.SimpleArmsAnim; }
		static void SetAnim(Game g, bool v) { g.SimpleArmsAnim = !v; }
		
		static bool GetGui(Game g) { return  g.UseClassicGui; }
		static void SetGui(Game g, bool v) { g.UseClassicGui = v; }
		
		static bool GetList(Game g) { return  g.UseClassicTabList; }
		static void SetList(Game g, bool v) { g.UseClassicTabList = v; }
		
		static bool GetOpts(Game g) { return  g.UseClassicOptions; }
		static void SetOpts(Game g, bool v) { g.UseClassicOptions = v; }
		
		static bool GetCustom(Game g) { return  g.UseCustomBlocks; }
		static void SetCustom(Game g, bool v) { g.UseCustomBlocks = v; }
		
		static bool GetCPE(Game g) { return  g.UseCPE; }
		static void SetCPE(Game g, bool v) { g.UseCPE = v; }
		
		static bool GetTexs(Game g) { return  g.AllowServerTextures; }
		static void SetTexs(Game g, bool v) { g.AllowServerTextures = v; }
				
		Screen PreviousScreen() {
			if (game.UseClassicOptions) return new PauseScreen(game);
			return new OptionsGroupScreen(game);
		}
	}
}