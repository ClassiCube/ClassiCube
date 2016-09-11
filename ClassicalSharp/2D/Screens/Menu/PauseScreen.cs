// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public class PauseScreen : MenuScreen {
		
		public PauseScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			api.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			game.Events.HackPermissionsChanged += CheckHacksAllowed;
			if( game.UseClassicOptions )
				MakeClassic();
			else 
				MakeNormal();			
			if( !game.Server.IsSinglePlayer ) {
				widgets[1].Disabled = true;
				widgets[2].Disabled = true;
			}
			CheckHacksAllowed( null, null );
		}
		
		void MakeNormal() {
			widgets = new Widget[] {
				Make( -1, -50, "Options",
				     (g, w) => g.Gui.SetNewScreen( new OptionsGroupScreen( g ) ) ),
				Make( 1, -50, "Generate level",
					     (g, w) => g.Gui.SetNewScreen( new GenLevelScreen( g ) ) ),	
				Make( 1, 0, "Load level",
					     (g, w) => g.Gui.SetNewScreen( new LoadLevelScreen( g ) ) ),
				Make( 1, 50, "Save level",
				     (g, w) => g.Gui.SetNewScreen( new SaveLevelScreen( g ) ) ),
				Make( -1, 0, "Select texture pack",
				     (g, w) => g.Gui.SetNewScreen( new TexturePackScreen( g ) ) ),
				Make( -1, 50, "Hotkeys",
				     (g, w) => g.Gui.SetNewScreen( new HotkeyListScreen( g ) ) ),
				
				// Other
				ButtonWidget.Create( game, 5, 5, 120, 40, "Quit game",
				                    Anchor.BottomOrRight, Anchor.BottomOrRight, 
				                    titleFont, LeftOnly( (g, w) => g.Exit() ) ),
				MakeBack( true, titleFont, (g, w) => g.Gui.SetNewScreen( null ) ),
			};
		}
		
		void MakeClassic() {
			widgets = new Widget[] {
				MakeClassic( 0, -100, "Options",
				            (g, w) => g.Gui.SetNewScreen( new ClassicOptionsScreen( g ) ) ),				
				MakeClassic( 0, -50, "Generate level",
				            (g, w) => g.Gui.SetNewScreen( new GenLevelScreen( g ) ) ),				
				MakeClassic( 0, 0, "Load level",
				            (g, w) => g.Gui.SetNewScreen( new LoadLevelScreen( g ) ) ),				
				MakeClassic( 0, 50, "Save level",
				            (g, w) => g.Gui.SetNewScreen( new SaveLevelScreen( g ) ) ),

				MakeBack( 401, "Back to game", 22, titleFont, (g, w) => g.Gui.SetNewScreen( null ) ),
				
				game.ClassicMode ? null :
					MakeClassic( 0, 150, "Nostalgia options",
					            (g, w) => g.Gui.SetNewScreen( new NostalgiaScreen( g ) ) ),
			};
		}
		
		void CheckHacksAllowed( object sender, EventArgs e ) {
			if( game.UseClassicOptions ) return;
			widgets[4].Disabled = !game.LocalPlayer.Hacks.CanAnyHacks; // select texture pack
		}
		
		ButtonWidget Make( int dir, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, dir * 160, y, 301, 40, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeClassic( int x, int y, string text, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, 401, 40, text,
			                           Anchor.Centre, Anchor.Centre, titleFont, LeftOnly( onClick ) );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape )
				game.Gui.SetNewScreen( null );
			return key < Key.F1 || key > Key.F35;
		}
		
		public override void Dispose() {
			base.Dispose();
			game.Events.HackPermissionsChanged -= CheckHacksAllowed;
		}
	}
}