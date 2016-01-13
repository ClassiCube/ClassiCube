using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public sealed class NostalgiaScreen : MenuInputScreen {
		
		TextWidget infoWidget;
		public NostalgiaScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -100, "Simple arms anim", OnWidgetClick,
				     g => g.SimpleArmsAnim? "yes" : "no",
				     (g, v) => { g.SimpleArmsAnim = v == "yes";
				     	Options.Set( OptionsKey.SimpleArmsAnim, v == "yes" ); }),
				
				Make( -140, -50, "Classic gui textures", OnWidgetClick,
				     g => g.UseClassicGui ? "yes" : "no",
				     (g, v) => { g.UseClassicGui = v == "yes";
				     	Options.Set( OptionsKey.UseClassicGui, v == "yes" );
				     } ),
				
				Make( -140, 0, "Classic player list", OnWidgetClick,
				     g => g.UseClassicTabList ? "yes" : "no",
				     (g, v) => { g.UseClassicTabList = v == "yes";
				     	Options.Set( OptionsKey.UseClassicTabList, v == "yes" );
				     } ),
				
				Make( -140, 50, "Arial chat font", OnWidgetClick,
				     g => g.Drawer2D.UseBitmappedChat ? "no" : "yes",
				     (g, v) => {
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
				     } ),
				
				// Column 2
				Make( 140, -100, "Allow custom blocks", OnWidgetClick,
				     g => g.AllowCustomBlocks ? "yes" : "no",
				     (g, v) => { g.AllowCustomBlocks = v == "yes";
				     	Options.Set( OptionsKey.AllowCustomBlocks, v == "yes" );
				     } ),
				
				Make( 140, -50, "Use CPE", OnWidgetClick,
				     g => g.UseCPE ? "yes" : "no",
				     (g, v) => { g.UseCPE = v == "yes";
				     	Options.Set( OptionsKey.UseCPE, v == "yes" );
				     } ),
				
				Make( 140, 0, "Allow server textures", OnWidgetClick,
				     g => g.AllowServerTextures ? "yes" : "no",
				     (g, v) => { g.AllowServerTextures = v == "yes";
				     	Options.Set( OptionsKey.AllowServerTextures, v == "yes" );
				     } ),
				
				MakeBack( false, titleFont,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null,
			};
			
			validators = new MenuInputValidator[] {
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
				
				new BooleanValidator(),
				new BooleanValidator(),
				new BooleanValidator(),
			};
			okayIndex = buttons.Length - 1;
			infoWidget = TextWidget.Create( game, 0, 150, "&eButtons on the right require a client restart.", 
			                               Anchor.Centre, Anchor.Centre, regularFont );
		}
		
		public override void Render( double delta ) {
			base.Render( delta );
			graphicsApi.Texturing = true;
			infoWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Dispose() {
			base.Dispose();
			infoWidget.Dispose();
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