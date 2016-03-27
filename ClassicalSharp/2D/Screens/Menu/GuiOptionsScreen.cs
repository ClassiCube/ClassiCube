// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace ClassicalSharp.Gui {
	
	public class GuiOptionsScreen : MenuOptionsScreen {
		
		public GuiOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				MakeBool( -1, -150, "Block in hand", OptionsKey.ShowBlockInHand,
				     OnWidgetClick, g => g.ShowBlockInHand, (g, v) => g.ShowBlockInHand = v ),
				
				MakeBool( -1, -100, "Show FPS", OptionsKey.ShowFPS,
				         OnWidgetClick, g => g.ShowFPS, (g, v) => g.ShowFPS = v ),
				
				Make( -1, -50, "Hotbar scale", OnWidgetClick,
				     g => g.HotbarScale.ToString(),
				     (g, v) => { g.HotbarScale = Single.Parse( v );
				     	Options.Set( OptionsKey.HotbarScale, v );
				     	g.RefreshHud();
				     } ),
				
				Make( -1, 0, "Inventory scale", OnWidgetClick,
				     g => g.InventoryScale.ToString(),
				     (g, v) => { g.InventoryScale = Single.Parse( v );
				     	Options.Set( OptionsKey.InventoryScale, v );
				     	g.RefreshHud();
				     } ),
				
				MakeBool( -1, 50, "Tab auto-complete", OptionsKey.TabAutocomplete, 
				         OnWidgetClick, g => g.TabAutocomplete, (g, v) => g.TabAutocomplete = v ),
				
				// Column 2				
				MakeBool( 1, -150, "Clickable chat", OptionsKey.ClickableChat,
				     OnWidgetClick, g => g.ClickableChat, (g, v) => g.ClickableChat = v ),
				
				Make( 1, -100, "Chat scale", OnWidgetClick,
				     g => g.ChatScale.ToString(),
				     (g, v) => { g.ChatScale = Single.Parse( v );
				     	Options.Set( OptionsKey.ChatScale, v );
				     	g.RefreshHud();
				     } ),

				Make( 1, -50, "Chat lines", OnWidgetClick,
				     g => g.ChatLines.ToString(),
				     (g, v) => { g.ChatLines = Int32.Parse( v );
				     	Options.Set( OptionsKey.ChatLines, v );
				     	g.RefreshHud();
				     } ),
				
				MakeBool( 1, 0, "Arial chat font", OptionsKey.ArialChatFont,
				     OnWidgetClick, g => !g.Drawer2D.UseBitmappedChat,
				     (g, v) => { g.Drawer2D.UseBitmappedChat = !v; HandleFontChange(); } ),		
				
				Make( 1, 50, "Chat font name", OnWidgetClick,
				     g => g.FontName,
				     (g, v) => { g.FontName = v;
				     	Options.Set( OptionsKey.FontName, v );
				     	HandleFontChange();
				     } ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				null, null,
			};			
			MakeValidators();
		}
		
		void HandleFontChange() {
			game.Events.RaiseChatFontChanged();
			base.Dispose();
			base.Init();
			game.RefreshHud();
			
			for( int i = 0; i < widgets.Length; i++) {
				if( widgets[i] == null || !(widgets[i] is ButtonWidget)) {
					widgets[i] = null; continue;
				}
				
				ButtonWidget btn = widgets[i] as ButtonWidget;
				btn.font = titleFont;
				btn.SetText( btn.Text );
			}
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new RealValidator( 0.25f, 4f ),
				new RealValidator( 0.25f, 4f ),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new RealValidator( 0.25f, 4f ),
				new IntegerValidator( 0, 30 ),
				new BooleanValidator(),
				new StringValidator( 32 ),
			};
		}
	}
}