using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class WarningScreen : MenuScreen {
		
		public WarningScreen( Game game, object metadata, string title,
		                     Action<WarningScreen> yesClick, Action<WarningScreen> noClick,
		                     Action<WarningScreen> renderFrame, params string[] body ) : base( game ) {
			this.Metadata = metadata;
			this.yesClick = yesClick;
			this.noClick = noClick;
			this.renderFrame = renderFrame;
			this.title = title;
			this.body = body;
		}
		internal Screen lastScreen;
		internal bool lastCursorVisible;
		string title;
		string[] body;
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 14, FontStyle.Regular );
			
			buttons = new ButtonWidget[] {
				ButtonWidget.Create( game, -60, 30, 60, 25, "Yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesClick ),
				ButtonWidget.Create( game, 60, 30, 60, 25, "No", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoClick ),
			};
			SetText( title, body );
		}

		public void SetText( string title, params string[] body) {
			if( labels != null ) {
				for( int i = 0; i < labels.Length; i++ )
					labels[i].Dispose();
			}
			this.title = title;
			this.body = body;
			
			
			labels = new TextWidget[body.Length + 1];
			labels[0] = TextWidget.Create( game, 0, -120, title,
			                              Anchor.Centre, Anchor.Centre, titleFont );
			for( int i = 0; i < body.Length; i++ ) {
				labels[i + 1] = TextWidget.Create( game, 0, -70 + 20 * i, body[i],
				                                  Anchor.Centre, Anchor.Centre, regularFont );
			}
		}
		TextWidget[] labels;
		Action<WarningScreen> yesClick, noClick, renderFrame;
		
		void OnYesClick( Game g, Widget w ) {
			if( yesClick != null )
				yesClick( this );
			Dispose();
			CloseScreen();
		}
		
		void OnNoClick( Game g, Widget w ) {
			if( noClick != null )
				noClick( this );
			Dispose();
			CloseScreen();
		}
		
		void CloseScreen() {
			game.WarningScreens.RemoveAt( 0 );
			if( game.WarningScreens.Count > 0 ) {
				game.activeScreen = game.WarningScreens[0];
			} else {
				game.activeScreen = lastScreen;
				if( game.CursorVisible != lastCursorVisible )
					game.CursorVisible = lastCursorVisible;
			}
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Render( delta );
			graphicsApi.Texturing = false;
			
			if( renderFrame != null )
				renderFrame( this );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			base.OnResize( oldWidth, oldHeight, width, height );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			base.Dispose();
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Dispose();
		}
	}
}