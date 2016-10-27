// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public class DeathScreen : MenuScreen {
		
		public DeathScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			regularFont = new Font( game.FontName, 40, FontStyle.Regular );
			
			widgets = new Widget[] {
				ChatTextWidget.Create( game, 0, -150, "Game over!", Anchor.Centre, Anchor.Centre, regularFont ),
				ChatTextWidget.Create( game, 0, -75, "Score: 0", Anchor.Centre, Anchor.Centre, titleFont ),
				ButtonWidget.Create( game, 0, 25, 401, 40, "Generate new level...",
				                    Anchor.Centre, Anchor.Centre, titleFont, GenLevelClick ),
				ButtonWidget.Create( game, 0, 75, 401, 40, "Load level...",
				                    Anchor.Centre, Anchor.Centre, titleFont, LoadLevelClick ),
			};
		}

		void GenLevelClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.Gui.SetNewScreen( new GenLevelScreen( game ) );
		}
		
		void LoadLevelClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			game.Gui.SetNewScreen( new LoadLevelScreen( game ) );
		}
	}
}
