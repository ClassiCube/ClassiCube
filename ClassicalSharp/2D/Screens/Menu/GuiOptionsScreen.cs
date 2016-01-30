using System;

namespace ClassicalSharp {
	
	public class GuiOptionsScreen : MenuOptionsScreen {
		
		public GuiOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new ButtonWidget[] {
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
				
				// Column 2
				Make( 140, -150, "Tab auto-complete", OnWidgetClick,
				     g => g.TabAutocomplete ? "yes" : "no",
				     (g, v) => { g.TabAutocomplete = v == "yes";
				     	Options.Set( OptionsKey.TabAutocomplete, v == "yes" );
				     } ),
				
				Make( 140, -100, "Clickable chat", OnWidgetClick,
				     g => g.ClickableChat ? "yes" : "no",
				     (g, v) => { g.ClickableChat = v == "yes";
				     	Options.Set( OptionsKey.ClickableChat, v == "yes" );
				     } ),
				
				Make( 140, -50, "Chat scale", OnWidgetClick,
				     g => g.ChatScale.ToString(),
				     (g, v) => { g.ChatScale = Single.Parse( v );
				     	Options.Set( OptionsKey.ChatScale, v );
				     	g.RefreshHud();
				     } ),

				Make( 140, 0, "Chat lines", OnWidgetClick,
				     g => g.ChatLines.ToString(),
				     (g, v) => { g.ChatLines = Int32.Parse( v );
				     	Options.Set( OptionsKey.ChatLines, v );
				     	g.RefreshHud();
				     } ),
				
				Make( 140, 50, "Arial chat font", OnWidgetClick,
				     g => g.Drawer2D.UseBitmappedChat ? "no" : "yes",
				     HandleArialChatFont ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			
			MakeValidators();
			okayIndex = widgets.Length - 1;
		}
		
		void HandleArialChatFont( Game g, string v ) {
			g.Drawer2D.UseBitmappedChat = v == "no";
			Options.Set( OptionsKey.ArialChatFont, v == "yes" );
			game.Events.RaiseChatFontChanged();
			
			if( inputWidget != null ) {
				inputWidget.Dispose(); inputWidget = null;
			}
			if( descWidget != null ) {
				descWidget.Dispose(); descWidget = null;
			}
			
			g.RefreshHud();
			Recreate();
		}
		
		void MakeValidators() {
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new RealValidator( 0.25f, 5f ),
				new RealValidator( 0.25f, 5f ),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new RealValidator( 0.25f, 5f ),
				new IntegerValidator( 1, 30 ),
				new BooleanValidator(),
			};
		}
	}
}