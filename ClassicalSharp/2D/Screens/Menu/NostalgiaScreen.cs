// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Singleplayer;

namespace ClassicalSharp.Gui {
	
	public sealed class NostalgiaScreen : MenuOptionsScreen {
		
		TextWidget infoWidget;
		public NostalgiaScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			base.Init();
			
			widgets = new Widget[] {
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
				
				Make( -140, 50, "Classic options menu", OnWidgetClick,
				     g => g.UseClassicOptions ? "yes" : "no",
				     (g, v) => { g.UseClassicOptions = v == "yes";
				     	Options.Set( OptionsKey.UseClassicOptions, v == "yes" );
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
	}
}