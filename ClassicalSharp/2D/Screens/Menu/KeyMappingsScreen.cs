using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public class KeyMappingsScreen : MenuScreen {
		
		public KeyMappingsScreen( Game game ) : base( game ) {
		}
		
		public override void Render( double delta ) {
			base.Render( delta );
		}
		
		Font keyFont;
		public override void Init() {
			keyFont = new Font( "Arial", 14, FontStyle.Bold );
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			string[] descriptionsLeft = { "Forward", "Back", "Left", "Right", "Jump", "Respawn", "Set spawn",
				"Open chat", "Send chat", "Pause", "Open inventory" };
			string[] descriptionsRight = { "Take screenshot", "Toggle fullscreen", "Toggle 3rd person camera", "Change view distance", 
				"Toggle fly", "Speed", "Toggle noclip", "Fly up", "Fly down", "Display player list", "Hide gui" };
			buttons = new ButtonWidget[descriptionsLeft.Length + descriptionsRight.Length + 1];
			
			MakeKeys( KeyMapping.Forward, descriptionsLeft, -140 );	
			MakeKeys( KeyMapping.Screenshot, descriptionsRight, 140 );
			buttons[index] = Make( 0, 5, "Back to menu", Docking.BottomOrRight, g => g.SetNewScreen( new PauseScreen( g ) ) );
		}
		
		int index;
		void MakeKeys( KeyMapping start, string[] descriptions, int x ) {
			int y = -180;
			for( int i = 0; i < descriptions.Length; i++ ) {
				int width = x < 0 ? 180 : 240;
				buttons[index++] = ButtonWidget.Create( game, x, y, width, 25, descriptions[i],
				                                       Docking.Centre, Docking.Centre, keyFont, g => { } );
				y += 30;
			}
		}
		
		ButtonWidget Make( int x, int y, string text, Docking vDocking, Action<Game> onClick ) {
			return ButtonWidget.Create( game, x, y, 240, 35, text, Docking.Centre, vDocking, keyFont, onClick );
		}
		
		public override void Dispose() {
			keyFont.Dispose();
			base.Dispose();
		}
	}
}