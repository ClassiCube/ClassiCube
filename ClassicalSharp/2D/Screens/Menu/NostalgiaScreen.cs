// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui.Screens {
	public sealed class NostalgiaScreen : MenuOptionsScreen {
		
		public NostalgiaScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
				// Column 1
				MakeBool( -1, -100, "Classic arms anim", OptionsKey.SimpleArmsAnim,
				         OnWidgetClick, g => !g.SimpleArmsAnim, (g, v) => g.SimpleArmsAnim = !v ),				
				MakeBool( -1, -50, "Classic gui textures", OptionsKey.UseClassicGui,
				         OnWidgetClick, g => g.UseClassicGui, (g, v) => g.UseClassicGui = v ),				
				MakeBool( -1, 0, "Classic player list", OptionsKey.UseClassicTabList,
				         OnWidgetClick, g => g.UseClassicTabList, (g, v) => g.UseClassicTabList = v ),				
				MakeBool( -1, 50, "Classic options", OptionsKey.UseClassicOptions,
				         OnWidgetClick, g => g.UseClassicOptions, (g, v) => g.UseClassicOptions = v ),
				
				// Column 2
				MakeBool( 1, -100, "Allow custom blocks", OptionsKey.AllowCustomBlocks,
				         OnWidgetClick, g => g.AllowCustomBlocks, (g, v) => g.AllowCustomBlocks = v ),			
				MakeBool( 1, -50, "Use CPE", OptionsKey.UseCPE,
				         OnWidgetClick, g => g.UseCPE, (g, v) => g.UseCPE = v ),				
				MakeBool( 1, 0, "Use server textures", OptionsKey.AllowServerTextures,
				         OnWidgetClick, g => g.AllowServerTextures, (g, v) => g.AllowServerTextures = v ),

				ChatTextWidget.Create( game, 0, 100,
				                      "&eButtons on the right require a client restart",
				                      Anchor.Centre, Anchor.Centre, regularFont ),				
				MakeBack( false, titleFont,
				         (g, w) => g.Gui.SetNewScreen( PreviousScreen() ) ),
				null, null,
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
		}
		
		Screen PreviousScreen() {
			if( game.UseClassicOptions ) return new PauseScreen( game );
			return new OptionsGroupScreen( game );
		}
	}
}