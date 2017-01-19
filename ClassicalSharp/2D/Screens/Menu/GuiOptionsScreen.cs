// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Gui.Widgets;

namespace ClassicalSharp.Gui.Screens {
	public class GuiOptionsScreen : MenuOptionsScreen {
		
		public GuiOptionsScreen(Game game) : base(game) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				MakeBool(-1, -150, "Black text shadows", OptionsKey.BlackTextShadows,
				     OnWidgetClick, g => g.Drawer2D.BlackTextShadows,
				     (g, v) => { g.Drawer2D.BlackTextShadows = v; HandleFontChange(); }),		
				
				MakeBool(-1, -100, "Show FPS", OptionsKey.ShowFPS,
				         OnWidgetClick, g => g.ShowFPS, (g, v) => g.ShowFPS = v),
				
				MakeOpt(-1, -50, "Hotbar scale", OnWidgetClick,
				     g => g.HotbarScale.ToString("F1"),
				     (g, v) => { g.HotbarScale = Utils.ParseDecimal(v);
				     	Options.Set(OptionsKey.HotbarScale, v);
				     	g.Gui.RefreshHud();
				     }),
				
				MakeOpt(-1, 0, "Inventory scale", OnWidgetClick,
				     g => g.InventoryScale.ToString("F1"),
				     (g, v) => { g.InventoryScale = Utils.ParseDecimal(v);
				     	Options.Set(OptionsKey.InventoryScale, v);
				     	g.Gui.RefreshHud();
				     }),
				
				MakeBool(-1, 50, "Tab auto-complete", OptionsKey.TabAutocomplete, 
				         OnWidgetClick, g => g.TabAutocomplete, (g, v) => g.TabAutocomplete = v),
				
				// Column 2				
				MakeBool(1, -150, "Clickable chat", OptionsKey.ClickableChat,
				     OnWidgetClick, g => g.ClickableChat, (g, v) => g.ClickableChat = v),
				
				MakeOpt(1, -100, "Chat scale", OnWidgetClick,
				     g => g.ChatScale.ToString("F1"),
				     (g, v) => { g.ChatScale = Utils.ParseDecimal(v);
				     	Options.Set(OptionsKey.ChatScale, v);
				     	g.Gui.RefreshHud();
				     }),

				MakeOpt(1, -50, "Chat lines", OnWidgetClick,
				     g => g.ChatLines.ToString(),
				     (g, v) => { g.ChatLines = Int32.Parse(v);
				     	Options.Set(OptionsKey.ChatLines, v);
				     	g.Gui.RefreshHud();
				     }),
				
				MakeBool(1, 0, "Use font", OptionsKey.ArialChatFont,
				     OnWidgetClick, g => !g.Drawer2D.UseBitmappedChat,
				     (g, v) => { g.Drawer2D.UseBitmappedChat = !v; HandleFontChange(); }),
				
				MakeOpt(1, 50, "Font", OnWidgetClick,
				     g => g.FontName,
				     (g, v) => { g.FontName = v;
				     	Options.Set(OptionsKey.FontName, v);
				     	HandleFontChange();
				     }),
				
				MakeBack(false, titleFont,
				         (g, w) => g.Gui.SetNewScreen(new OptionsGroupScreen(g))),
				null, null,
			};			
			MakeValidators();
			MakeDescriptions();
		}
		
		void HandleFontChange() {
			int selIndex = Array.IndexOf<Widget>(widgets, selectedWidget);
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
				new StringValidator(32),
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