using System;

namespace ClassicalSharp {
	
	public class GuiOptionsScreen : MenuOptionsScreen {
		
		public GuiOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				Make( -140, -150, "Block in hand", OnWidgetClick,
				     g => g.ShowBlockInHand ? "yes" : "no",
				     (g, v) => { g.ShowBlockInHand = v == "yes";
				     	Options.Set( OptionsKey.ShowBlockInHand, v == "yes" );
				     } ),
				
				Make( -140, -100, "Show FPS", OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				     (g, v) => { g.ShowFPS = v == "yes";
				     	Options.Set( OptionsKey.ShowFPS, v == "yes" ); }),
				
				Make( -140, -50, "Hotbar scale", OnWidgetClick,
				     g => g.HotbarScale.ToString(),
				     (g, v) => { g.HotbarScale = Single.Parse( v );
				     	Options.Set( OptionsKey.HotbarScale, v );
				     	g.RefreshHud();
				     } ),
				
				Make( -140, 0, "Inventory scale", OnWidgetClick,
				     g => g.InventoryScale.ToString(),
				     (g, v) => { g.InventoryScale = Single.Parse( v );
				     	Options.Set( OptionsKey.InventoryScale, v );
				     	g.RefreshHud();
				     } ),
				
				Make( -140, 50, "Tab auto-complete", OnWidgetClick,
				     g => g.TabAutocomplete ? "yes" : "no",
				     (g, v) => { g.TabAutocomplete = v == "yes";
				     	Options.Set( OptionsKey.TabAutocomplete, v == "yes" );
				     } ),
				
				// Column 2				
				Make( 140, -150, "Clickable chat", OnWidgetClick,
				     g => g.ClickableChat ? "yes" : "no",
				     (g, v) => { g.ClickableChat = v == "yes";
				     	Options.Set( OptionsKey.ClickableChat, v == "yes" );
				     } ),
				
				Make( 140, -100, "Chat scale", OnWidgetClick,
				     g => g.ChatScale.ToString(),
				     (g, v) => { g.ChatScale = Single.Parse( v );
				     	Options.Set( OptionsKey.ChatScale, v );
				     	g.RefreshHud();
				     } ),

				Make( 140, -50, "Chat lines", OnWidgetClick,
				     g => g.ChatLines.ToString(),
				     (g, v) => { g.ChatLines = Int32.Parse( v );
				     	Options.Set( OptionsKey.ChatLines, v );
				     	g.RefreshHud();
				     } ),
				
				Make( 140, 0, "Arial chat font", OnWidgetClick,
				     g => g.Drawer2D.UseBitmappedChat ? "no" : "yes",
				     (g, v) => { g.Drawer2D.UseBitmappedChat = v == "no";
				     	Options.Set( OptionsKey.ArialChatFont, v == "yes" );
				     	HandleFontChange();
				     } ),			
				
				Make( 140, 50, "Chat font name", OnWidgetClick,
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
			
			if( inputWidget != null ) {
				inputWidget.Dispose(); inputWidget = null;
				widgets[widgets.Length - 2] = null;
			}
			if( descWidget != null ) {
				descWidget.Dispose(); descWidget = null;
			}		
			game.RefreshHud();
			Recreate();
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