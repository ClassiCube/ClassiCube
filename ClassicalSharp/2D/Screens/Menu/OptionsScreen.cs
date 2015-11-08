using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp {
	
	public class OptionsScreen : MenuInputScreen {
		
		public OptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			INetworkProcessor network = game.Network;
			
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -100, "Speed multiplier", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.SpeedMultiplier.ToString(),
				     (g, v) => { g.LocalPlayer.SpeedMultiplier = Int32.Parse( v );
				     	Options.Set( OptionsKey.Speed, v ); } ),
				
				Make( -140, -50, "Show FPS", Anchor.Centre, OnWidgetClick,
				     g => g.ShowFPS ? "yes" : "no",
				     (g, v) => g.ShowFPS = v == "yes" ),
				
				Make( -140, 0, "VSync active", Anchor.Centre, OnWidgetClick,
				     g => g.VSync ? "yes" : "no",
				     (g, v) => { g.Graphics.SetVSync( g, v == "yes" );
				     	Options.Set( OptionsKey.VSync, v == "yes" ); } ),

				Make( -140, 50, "View distance", Anchor.Centre, OnWidgetClick,
				     g => g.ViewDistance.ToString(),
				     (g, v) => g.SetViewDistance( Int32.Parse( v ) ) ),
				// Column 2
				Make( 140, -100, "Mouse sensitivity", Anchor.Centre, OnWidgetClick,
				     g => g.MouseSensitivity.ToString(),
				     (g, v) => { g.MouseSensitivity = Int32.Parse( v );
				     	Options.Set( OptionsKey.Sensitivity, v ); } ),
				
				Make( 140, -50, "Chat font size", Anchor.Centre, OnWidgetClick,
				     g => g.Chat.FontSize.ToString(),
				     (g, v) => { g.Chat.FontSize = Int32.Parse( v );
				     	Options.Set( OptionsKey.FontSize, v ); } ),

				Make( 140, 0, "Chat lines", Anchor.Centre, OnWidgetClick,
				     g => g.ChatLines.ToString(),
				     (g, v) => { g.ChatLines = Int32.Parse( v );
				     	Options.Set( OptionsKey.ChatLines, v ); } ),
				
				Make( 140, 50, "Arial chat font", Anchor.Centre, OnWidgetClick,
				     g => g.Drawer2D.UseBitmappedChat ? "no" : "yes",
				     (g, v) => {
				     	g.Drawer2D.UseBitmappedChat = v == "no";
				     	Options.Set( OptionsKey.ArialChatFont, v == "yes" );
				     	game.Events.RaiseChatFontChanged(); } ),
				
				// Extra stuff
				!network.IsSinglePlayer ? null :
					Make( -140, -150, "Singleplayer physics", Anchor.Centre, OnWidgetClick,
					     g => ((SinglePlayerServer)network).physics.Enabled ? "yes" : "no",
					     (g, v) => ((SinglePlayerServer)network).physics.Enabled = (v == "yes") ),
				Make( 140, -150, "Pushback block placing", Anchor.Centre, OnWidgetClick,
				     g => g.LocalPlayer.PushbackBlockPlacing 
				     && g.LocalPlayer.CanPushbackBlocks ? "yes" : "no",
				     (g, v) => { if( g.LocalPlayer.CanPushbackBlocks) 
				     		g.LocalPlayer.PushbackBlockPlacing = v == "yes"; }),
				
				Make( 0, 5, "Back to menu", Anchor.BottomOrRight,
				     (g, w) => g.SetNewScreen( new PauseScreen( g ) ), null, null ),
				null,
			};
			validators = new MenuInputValidator[] {
				new IntegerValidator( 1, 50 ),
				new BooleanValidator(),
				new BooleanValidator(),
				new IntegerValidator( 16, 4096 ),
				
				new IntegerValidator( 1, 100 ),
				new IntegerValidator( 6, 30 ),
				new IntegerValidator( 1, 30 ),
				new BooleanValidator(),
				
				network.IsSinglePlayer ? new BooleanValidator() : null,
				new BooleanValidator()
			};
			okayIndex = buttons.Length - 1;
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick,
		                  Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, x, y, 240, 35, text, Anchor.Centre, vDocking, titleFont, onClick );
			widget.GetValue = getter;
			widget.SetValue = setter;
			return widget;
		}
	}
}