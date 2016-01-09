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
		internal bool wasCursorVisible;
		string title;
		string[] body;
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 14, FontStyle.Regular );
			
			buttons = new ButtonWidget[] {
				ButtonWidget.Create( game, -110, 30, 160, 35, "Yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesClick ),
				ButtonWidget.Create( game, 110, 30, 160, 35, "No", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoClick ),
				ButtonWidget.Create( game, -110, 80, 160, 35, "Always yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesAlwaysClick ),
				ButtonWidget.Create( game, 110, 80, 160, 35, "Always no", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoAlwaysClick ),
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
		
		void OnYesAlwaysClick( Game g, Widget w ) {
			OnYesClick( g, w );
			string url = ((string)Metadata).Substring( 3 );
			if( !game.AcceptedUrls.HasUrl( url ) )
				game.AcceptedUrls.AddUrl( url );
		}
		
		void OnNoAlwaysClick( Game g, Widget w ) {
			OnNoClick( g, w );
			string url = ((string)Metadata).Substring( 3 );
			if( !game.DeniedUrls.HasUrl( url ) )
				game.DeniedUrls.AddUrl( url );
		}
		
		void CloseScreen() {
			game.WarningScreens.RemoveAt( 0 );
			if( game.WarningScreens.Count > 0 ) {
				game.activeScreen = game.WarningScreens[0];
			} else {
				game.activeScreen = lastScreen;
				if( game.CursorVisible != wasCursorVisible )
					game.CursorVisible = wasCursorVisible;
				if( game.activeScreen == null && game.CursorVisible )
					game.CursorVisible = false;
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