using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class EnvSettingsScreen : MenuScreen {
		
		public EnvSettingsScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			base.Render( delta );
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[] {
				Make( -140, -100, "Clouds colour", Docking.Centre, g => { } ),
				Make( -140, -50, "Sky colour", Docking.Centre, g => { } ),
				Make( -140, 0, "Fog colour", Docking.Centre, g => { } ),
				Make( -140, 50, "Clouds speed", Docking.Centre, g => { } ),
				Make( 140, -100, "Sunlight colour", Docking.Centre, g => { } ),
				Make( 140, -50, "Shadow colour", Docking.Centre, g => { } ),
				Make( 140, 0, "Weather", Docking.Centre, g => { } ),
				Make( 140, 50, "Cloud offset", Docking.Centre, g => { } ),
				Make( 0, 5, "Back to menu", Docking.BottomOrRight, g => g.SetNewScreen( new NewPauseScreen( g ) ) ),
			};
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, titleFont, onClick );
		}
	}
}