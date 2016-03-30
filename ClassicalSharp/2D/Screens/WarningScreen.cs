// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class WarningScreen : MenuScreen {
		
		public WarningScreen( Game game, object metadata, bool confirmNo, string title,
		                     Action<WarningScreen> yesClick, Action<WarningScreen> noClick,
		                     Action<WarningScreen> renderFrame, params string[] body ) : base( game ) {
			this.Metadata = metadata;
			this.yesClick = yesClick;
			this.noClick = noClick;
			this.renderFrame = renderFrame;
			this.title = title;
			this.body = body;
			this.confirmNo = confirmNo;
		}
		
		internal Screen lastScreen;
		internal bool wasCursorVisible;
		string title, lastTitle;
		string[] body, lastBody;
		bool confirmNo, confirmMode;
		
		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			regularFont = new Font( game.FontName, 14, FontStyle.Regular );
			InitStandardButtons();
			SetText( title, body );
		}

		public void SetText( string title, params string[] body ) {
			lastTitle = title;
			lastBody = body;
			
			if( confirmMode ) {
				SetTextImpl( "&eYou might be missing out.",
				            "Texture packs can play a vital role in the look and feel of maps.",
				            "", "Sure you don't want to download the texture pack?" );
			} else {
				SetTextImpl( title, body );
			}
		}
		
		void SetTextImpl( string title, params string[] body) {
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
			api.Texturing = true;
			RenderMenuWidgets( delta );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Render( delta );
			api.Texturing = false;
			
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
		
		void InitStandardButtons() {
			widgets = new ButtonWidget[] {
				ButtonWidget.Create( game, -110, 30, 160, 35, "Yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesClick ),
				ButtonWidget.Create( game, 110, 30, 160, 35, "No", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoClick ),
				ButtonWidget.Create( game, -110, 80, 160, 35, "Always yes", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnYesAlwaysClick ),
				ButtonWidget.Create( game, 110, 80, 160, 35, "Always no", Anchor.Centre,
				                    Anchor.Centre, titleFont, OnNoAlwaysClick ),
			};
		}
		
		Action<WarningScreen> yesClick, noClick, renderFrame;
		void OnYesClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			if( yesClick != null )
				yesClick( this );
			Dispose();
			CloseScreen();
		}
		
		void OnNoClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			if( confirmNo && !confirmMode ) {
				InitConfirmButtons( false ); return;
			}
			
			if( noClick != null )
				noClick( this );
			Dispose();
			CloseScreen();
		}
		
		void OnYesAlwaysClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			OnYesClick( g, w, mouseBtn );
			string url = ((string)Metadata).Substring( 3 );
			if( !game.AcceptedUrls.HasUrl( url ) )
				game.AcceptedUrls.AddUrl( url );
		}
		
		void OnNoAlwaysClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			if( confirmNo && !confirmMode ) {
				InitConfirmButtons( true ); return;
			}
			
			OnNoClick( g, w, mouseBtn );
			string url = ((string)Metadata).Substring( 3 );
			if( !game.DeniedUrls.HasUrl( url ) )
				game.DeniedUrls.AddUrl( url );
		}

		
		void InitConfirmButtons( bool always ) {
			ClickHandler noHandler = always ? (ClickHandler)OnNoAlwaysClick: (ClickHandler)OnNoClick;
			widgets = new ButtonWidget[] {
				ButtonWidget.Create( game, -110, 30, 160, 35, "I'm sure", Anchor.Centre,
				                    Anchor.Centre, titleFont, noHandler ),
				ButtonWidget.Create( game, 110, 30, 160, 35, "Go back", Anchor.Centre,
				                    Anchor.Centre, titleFont, GoBackClick ),
			};
			confirmMode = true;
			SetText( lastTitle, lastBody );
		}
		
		void GoBackClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			InitStandardButtons();
			confirmMode = false;
			SetText( lastTitle, lastBody );
		}
	}
}