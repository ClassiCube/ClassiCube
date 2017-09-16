// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class GuiOptionsScreen : MenuOptionsScreen {
		
		public GuiOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			ContextRecreated();
			MakeValidators();
			MakeDescriptions();
		}
		
		protected override void ContextRecreated() {
			ClickHandler onClick = OnWidgetClick;
			widgets = new Widget[] {
				MakeOpt(-1, -150, "Black text shadows", onClick, GetShadows,   SetShadows),
				MakeOpt(-1, -100, "Show FPS",           onClick, GetShowFPS,   SetShowFPS),
				MakeOpt(-1, -50, "Hotbar scale",        onClick, GetHotbar,    SetHotbar),
				MakeOpt(-1, 0, "Inventory scale",       onClick, GetInventory, SetInventory),
				MakeOpt(-1, 50, "Tab auto-complete",    onClick, GetTabAuto,   SetTabAuto),

				MakeOpt(1, -150, "Clickable chat",      onClick, GetClickable, SetClickable),
				MakeOpt(1, -100, "Chat scale",          onClick, GetChatScale, SetChatScale),
				MakeOpt(1, -50, "Chat lines",           onClick, GetChatlines, SetChatlines),
				MakeOpt(1, 0, "Use font",               onClick, GetUseFont,   SetUseFont),
				MakeOpt(1, 50, "Font",                  onClick, GetFont,      SetFont),
				
				MakeBack(false, titleFont, SwitchOptions),
				null, null,
			};
		}
		
		static string GetShadows(Game g) { return GetBool(g.Drawer2D.BlackTextShadows); }
		void SetShadows(Game g, string v) { 
			g.Drawer2D.BlackTextShadows = SetBool(v, OptionsKey.BlackText);
			HandleFontChange(); 
		}
		
		static string GetShowFPS(Game g) { return GetBool(g.ShowFPS); }
		static void SetShowFPS(Game g, string v) { g.ShowFPS = SetBool(v, OptionsKey.ShowFPS); }
		
		static void SetScale(Game g, string v, ref float target, string optKey) {
			target = Utils.ParseDecimal(v);
			Options.Set(optKey, v);
			g.Gui.RefreshHud();
		}
		
		static string GetHotbar(Game g) { return g.HotbarScale.ToString("F1"); }
		static void SetHotbar(Game g, string v) { SetScale(g, v, ref g.HotbarScale, OptionsKey.HotbarScale); }
		
		static string GetInventory(Game g) { return g.InventoryScale.ToString("F1"); }
		static void SetInventory(Game g, string v) { SetScale(g, v, ref g.InventoryScale, OptionsKey.InventoryScale); }
		
		static string GetTabAuto(Game g) { return GetBool(g.TabAutocomplete); }
		static void SetTabAuto(Game g, string v) { g.TabAutocomplete = SetBool(v, OptionsKey.TabAutocomplete); }
		
		static string GetClickable(Game g) { return GetBool(g.ClickableChat); }
		static void SetClickable(Game g, string v) { g.ClickableChat = SetBool(v, OptionsKey.ClickableChat); }
		
		static string GetChatScale(Game g) { return g.ChatScale.ToString("F1"); }
		static void SetChatScale(Game g, string v) { SetScale(g, v, ref g.ChatScale, OptionsKey.ChatScale); }
		
		static string GetChatlines(Game g) { return g.ChatLines.ToString(); }
		static void SetChatlines(Game g, string v) {
			g.ChatLines = Int32.Parse(v);
			Options.Set(OptionsKey.ChatLines, v);
			g.Gui.RefreshHud();
		}
		
		static string GetUseFont(Game g) { return GetBool(!g.Drawer2D.UseBitmappedChat); }
		void SetUseFont(Game g, string v) { 
			g.Drawer2D.UseBitmappedChat = !SetBool(v, OptionsKey.UseChatFont);
			HandleFontChange(); 
		}
		
		static string GetFont(Game g) { return g.FontName; }
		void SetFont(Game g, string v) {
			g.FontName = v;
			Options.Set(OptionsKey.FontName, v);
			HandleFontChange();
		}
		
		void HandleFontChange() {
			int selIndex = IndexOfWidget(selectedWidget);
			game.Events.RaiseChatFontChanged();
			base.Dispose();
			base.Init();
			game.Gui.RefreshHud();
			
			for (int i = 0; i < widgets.Length; i++) {
				if (widgets[i] == null || !(widgets[i] is ButtonWidget)) {
					widgets[i] = null; continue;
				}
				
				ButtonWidget btn = widgets[i] as ButtonWidget;
				btn.font = titleFont;
				btn.SetText(btn.Text);
			}
			
			if (selIndex >= 0)
				selectedWidget = (ButtonWidget)widgets[selIndex];
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new RealValidator(0.25f, 4f),
				new RealValidator(0.25f, 4f),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new RealValidator(0.25f, 4f),
				new IntegerValidator(0, 30),
				new BooleanValidator(),
				new StringValidator(),
			};
		}
		
		void MakeDescriptions() {
			descriptions = new string[widgets.Length][];
			descriptions[8] = new string[] {
				"&eWhether a system font is used instead of default.png for drawing text",
				"&fThe default system font used is Arial.",
			};
		}
	}
}