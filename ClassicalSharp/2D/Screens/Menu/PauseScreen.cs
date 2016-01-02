using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class PauseScreen : MenuScreen {
		
		public PauseScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			buttons = new ButtonWidget[] {
				// Column 1
				Make( -140, -150, "Misc options", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new OptionsScreen( g ) ) ),
				Make( -140, -100, "Gui options", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new GuiOptionsScreen( g ) ) ),
				Make( -140, -50, "Hacks settings", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new HacksSettingsScreen( g ) ) ),
				Make( -140, 0, "Env settings", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new EnvSettingsScreen( g ) ) ),
				Make( -140, 50, "Key bindings", Anchor.Centre,
				     (g, w) => g.SetNewScreen( new NormalKeyBindingsScreen( g ) ) ),	
				
				// Column 2
				Make( 140, -150, "Save level", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					Make( 140, -100, "Load level", Anchor.Centre, 
					     (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
				!game.Network.IsSinglePlayer ? null :
					Make( 140, -50, "Generate level", Anchor.Centre, 
					     (g, w) => g.SetNewScreen( new GenLevelScreen( g ) ) ),
				Make( 140, 0, "Select texture pack", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
				Make( 140, 50, "Hotkeys", Anchor.Centre, 
				     (g, w) => g.SetNewScreen( new HotkeyScreen( g ) ) ),
				
				// Other
				MakeOther( 10, 5, 120, "Quit game", Anchor.BottomOrRight,
				          (g, w) => g.Exit() ),
				MakeBack( true, titleFont,
				         (g, w) => g.SetNewScreen( null ) ),
			};
			CheckHacksAllowed( null, null );
		}

		void CheckHacksAllowed( object sender, EventArgs e ) { 
			for( int i = 0; i < buttons.Length; i++ ) {
				if( buttons[i] == null ) continue;
				buttons[i].Disabled = false;
			}
			if( !game.LocalPlayer.CanAnyHacks ) {
				buttons[3].Disabled = true; // env settings
				buttons[8].Disabled = true; // select texture pack
			}
		}
		
		ButtonWidget Make( int x, int y, string text, Anchor vDocking, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, 
			                           Anchor.Centre, vDocking, titleFont, onClick );
		}
		
		ButtonWidget MakeOther( int x, int y, int width, string text, Anchor hAnchor, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, 35, text, 
			                           hAnchor, Anchor.BottomOrRight, titleFont, onClick );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.SetNewScreen( null );
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
	}
}