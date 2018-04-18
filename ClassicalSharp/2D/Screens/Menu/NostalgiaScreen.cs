// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public sealed class NostalgiaScreen : MenuOptionsScreen {
		
		public NostalgiaScreen(Game game) : base(game) {
		}
		
		protected override void ContextRecreated() {
			ClickHandler onBool = OnBoolClick;
			
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Classic hand model",  onBool, GetHand,   SetHand),
				MakeOpt(-1, -100, "Classic walk anim",   onBool, GetAnim,   SetAnim),
				MakeOpt(-1, -50, "Classic gui textures", onBool, GetGui,    SetGui),
				MakeOpt(-1, 0, "Classic player list",    onBool, GetList,   SetList),
				MakeOpt(-1, 50, "Classic options",       onBool, GetOpts,   SetOpts),

				MakeOpt(1, -150, "Allow custom blocks",  onBool, GetCustom, SetCustom),
				MakeOpt(1, -100, "Use CPE",              onBool, GetCPE,    SetCPE),
				MakeOpt(1, -50, "Use server textures",   onBool, GetTexs,   SetTexs),

				TextWidget.Create(game, "&eButtons on the right require restarting game", textFont)
					.SetLocation(Anchor.Centre, Anchor.Centre, 0, 100),
				MakeBack(false, titleFont, SwitchBack),
			};
		}
		
		static string GetHand(Game g) { return GetBool(g.ClassicArmModel); }
		static void SetHand(Game g, string v) { g.ClassicArmModel = SetBool(v, OptionsKey.ClassicArmModel); }
		
		static string GetAnim(Game g) { return GetBool(!g.SimpleArmsAnim); }
		static void SetAnim(Game g, string v) { 
			g.SimpleArmsAnim = v == "OFF";
			Options.Set(OptionsKey.SimpleArmsAnim, v == "OFF");
		}
		
		static string GetGui(Game g) { return GetBool(g.UseClassicGui); }
		static void SetGui(Game g, string v) { g.UseClassicGui = SetBool(v, OptionsKey.UseClassicGui); }
		
		static string GetList(Game g) { return GetBool(g.UseClassicTabList); }
		static void SetList(Game g, string v) { g.UseClassicTabList = SetBool(v, OptionsKey.UseClassicTabList); }
		
		static string GetOpts(Game g) { return GetBool(g.UseClassicOptions); }
		static void SetOpts(Game g, string v) { g.UseClassicOptions = SetBool(v, OptionsKey.UseClassicOptions); }
		
		static string GetCustom(Game g) { return GetBool(g.AllowCustomBlocks); }
		static void SetCustom(Game g, string v) { g.AllowCustomBlocks = SetBool(v, OptionsKey.UseCustomBlocks); }
		
		static string GetCPE(Game g) { return GetBool(g.UseCPE); }
		static void SetCPE(Game g, string v) { g.UseCPE = SetBool(v, OptionsKey.UseCPE); }
		
		static string GetTexs(Game g) { return GetBool(g.AllowServerTextures); }
		static void SetTexs(Game g, string v) { g.AllowServerTextures = SetBool(v, OptionsKey.UseServerTextures); }
		
		static void SwitchBack(Game g, Widget w) {
			if (g.UseClassicOptions) {
				SwitchPause(g, w);
			} else {
				SwitchOptions(g, w);
			}
		}
	}
}