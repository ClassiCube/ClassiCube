using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class OptionsScreen : MenuScreen {
		
		public OptionsScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			base.Render( delta );
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			buttons = new ButtonWidget[] {
				Make( -140, -50, "Use animations", Docking.Centre, g => { } ),
				Make( -140, 0, "View distance", Docking.Centre, g => { } ),
				Make( -140, 50, "VSync active", Docking.Centre, g => { } ),
				Make( 140, -50, "Mouse sensitivity", Docking.Centre, g => { } ),
				Make( 140, 0, "Chat font size", Docking.Centre, g => { } ),
				Make( 0, 5, "Back to menu", Docking.BottomOrRight, g => g.SetNewScreen( new NewPauseScreen( g ) ) ),
			};
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, titleFont, onClick );
		}
	}
}