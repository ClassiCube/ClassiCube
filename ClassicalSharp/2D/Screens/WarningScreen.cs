using System;
using System.Drawing;

namespace ClassicalSharp {
	
	// TODO: get and set activescreen.
	public sealed class WarningScreen : MenuScreen {
		
		public WarningScreen( Game game ) : base( game ) {
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 14, FontStyle.Regular );
			
			buttons = new ButtonWidget[] {
				ButtonWidget.Create( game, -60, 30, 60, 20, "Yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesClick ),
				ButtonWidget.Create( game, 60, 30, 60, 20, "No", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoClick ),
			};
			labels = new TextWidget[] {
				TextWidget.Create( game, 0, -120, "Do you want to XYZ?", 
				                  Anchor.Centre, Anchor.Centre, titleFont ),
				TextWidget.Create( game, 0, -70, "Warning text here",
				                  Anchor.Centre, Anchor.Centre, regularFont ),
			};
		}		
		TextWidget[] labels;
		
		void OnYesClick( Game g, Widget w ) {
			game.SetNewScreen( null );
		}
		
		void OnNoClick( Game g, Widget w ) {
			game.SetNewScreen( null );
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			base.OnResize( oldWidth, oldHeight, width, height );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].OnResize( oldWidth, oldHeight, width, height );
		}
	}
}