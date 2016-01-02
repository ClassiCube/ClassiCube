using System;

namespace ClassicalSharp {
	
	public class GuiOptionsScreen : MenuInputScreen {
		
		public GuiOptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -150, "Block in hand", OnWidgetClick,
				     g => g.ShowBlockInHand ? "yes" : "no",
				     (g, v) => { g.ShowBlockInHand = v == "yes";
				     	Options.Set( OptionsKey.ShowBlockInHand, v == "yes" );
				     } ),
				
				Make( -140, -100, "Show FPS", OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				     (g, v) => g.ShowFPS = v == "yes" ),
				
				Make( -140, -50, "Hud scale", OnWidgetClick,
				     g => g.HudScale.ToString(),
				     (g, v) => { g.HudScale = Single.Parse( v );
				     	Options.Set( OptionsKey.HudScale, v );
				     	g.RefreshHud();
				     } ),
				
				Make( -140, 0, "Use classic gui", OnWidgetClick,
				     g => g.UseClassicGui ? "yes" : "no",
				     (g, v) => { g.UseClassicGui = v == "yes";
				     	Options.Set( OptionsKey.UseClassicGui, v == "yes" );
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
				     (g, v) => {
				     	g.Drawer2D.UseBitmappedChat = v == "no";
				     	Options.Set( OptionsKey.ArialChatFont, v == "yes" );
				     	game.Events.RaiseChatFontChanged();
				     	g.RefreshHud();
				     	Recreate();
				     } ),
				
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new RealValidator( 0.25f, 5f ),
				new BooleanValidator(),
				
				new BooleanValidator(),		
				new RealValidator( 0.25f, 5f ),
				new IntegerValidator( 1, 30 ),
				new BooleanValidator(),
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, 
			                                          Anchor.Centre, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}