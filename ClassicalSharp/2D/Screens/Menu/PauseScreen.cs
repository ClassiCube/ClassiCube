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
			RenderMenuWidgets( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			if( game.UseClassicOptions )
				MakeClassic();
			else 
				MakeNormal();			
			if( !game.Network.IsSinglePlayer ) {
				widgets[1].Disabled = true;
				widgets[2].Disabled = true;
			}
			CheckHacksAllowed( null, null );
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				Make( -140, -50, "Options",
				     (g, w) => g.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				Make( 140, -50, "Generate level",
					     (g, w) => g.SetNewScreen( new GenLevelScreen( g ) ) ),	
				Make( 140, 0, "Load level",
					     (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),
				Make( 140, 50, "Save level",
				     (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),
				Make( -140, 0, "Select texture pack",
				     (g, w) => g.SetNewScreen( new TexturePackScreen( g ) ) ),
				Make( -140, 50, "Hotkeys",
				     (g, w) => g.SetNewScreen( new HotkeyScreen( g ) ) ),
				
				// Other
				MakeOther( 10, 5, 120, "Quit game", Anchor.BottomOrRight,
				          (g, w) => g.Exit() ),
				MakeBack( true, titleFont, (g, w) => g.SetNewScreen( null ) ),
			};
		}
		
		void MakeClassic() {
			widgets = new Widget[] {
				MakeClassic( 0, -100, "Options",
				            (g, w) => g.SetNewScreen( new ClassicOptionsScreen( g ) ) ),				
				MakeClassic( 0, -50, "Generate level",
				            (g, w) => g.SetNewScreen( new GenLevelScreen( g ) ) ),				
				MakeClassic( 0, 0, "Load level",
				            (g, w) => g.SetNewScreen( new LoadLevelScreen( g ) ) ),				
				MakeClassic( 0, 50, "Save level",
				            (g, w) => g.SetNewScreen( new SaveLevelScreen( g ) ) ),				
				MakeBack( true, titleFont, (g, w) => g.SetNewScreen( null ) ),
				
				game.PureClassicMode ? null :
					MakeClassic( 0, 150, "Nostalgia options",
					            (g, w) => g.SetNewScreen( new NostalgiaScreen( g ) ) ),
			};
		}
		
		void CheckHacksAllowed( object sender, EventArgs e ) {
			if( game.UseClassicOptions ) return;
			widgets[4].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // select texture pack
		}
		
		ButtonWidget Make( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeClassic( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 401, 41, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeOther( int x, int y, int width, string text, Anchor hAnchor, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, 35, text,
			                           hAnchor, Anchor.BottomOrRight, titleFont, LeftOnly( onClick ) );
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