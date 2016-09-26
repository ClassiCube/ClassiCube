// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public sealed class WarningScreen : MenuScreen {
		
		public WarningScreen( Game game, object metadata, bool showAlways, bool confirmNo, string title,
		                     Action<WarningScreen> yesClick, Action<WarningScreen> noClick,
		                     Action<WarningScreen> renderFrame, params string[] body ) : base( game ) {
			this.Metadata = metadata;
			this.yesClick = yesClick;
			this.noClick = noClick;
			this.renderFrame = renderFrame;
			this.title = title;
			this.body = body;
			this.confirmNo = confirmNo;
			this.showAlways = showAlways;
		}
		
		string title, lastTitle;
		string[] body, lastBody;
		bool confirmNo, confirmMode, showAlways;
		
		public override void Init() {
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			backCol.A = 210;
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
				labels[i + 1] = ChatTextWidget.Create( game, 0, -70 + 20 * i, body[i],
				                                  Anchor.Centre, Anchor.Centre, regularFont );
				labels[i + 1].Colour = new FastColour( 224, 224, 224 );
			}
		}
		TextWidget[] labels;
		
		void CloseScreen() {
			if( game.Gui.overlays.Count > 0 )
				game.Gui.overlays.RemoveAt( 0 );
			if( game.Gui.overlays.Count == 0 )
				game.CursorVisible = game.realVisible;
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
		
		public override void OnResize( int width, int height ) {
			base.OnResize( width, height );
			for( int i = 0; i < labels.Length; i++ )
				labels[i].OnResize( width, height );
		}
		
		public override void Dispose() {
			base.Dispose();
			for( int i = 0; i < labels.Length; i++ )
				labels[i].Dispose();
		}
		
		void InitStandardButtons() {
			widgets = new ButtonWidget[showAlways ? 4 : 2];
			widgets[0] = ButtonWidget.Create( game, -110, 30, 160, 35, "Yes", Anchor.Centre,
			                                 Anchor.Centre, titleFont, OnYesClick );
			widgets[1] = ButtonWidget.Create( game, 110, 30, 160, 35, "No", Anchor.Centre,
			                                 Anchor.Centre, titleFont, OnNoClick );
			if( !showAlways ) return;
			
			widgets[2] = ButtonWidget.Create( game, -110, 80, 160, 35, "Always yes", Anchor.Centre,
			                                 Anchor.Centre, titleFont, OnYesAlwaysClick );
			widgets[3] = ButtonWidget.Create( game, 110, 80, 160, 35, "Always no", Anchor.Centre,
			                                 Anchor.Centre, titleFont, OnNoAlwaysClick );
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
			if( !game.AcceptedUrls.HasEntry( url ) )
				game.AcceptedUrls.AddEntry( url );
		}
		
		void OnNoAlwaysClick( Game g, Widget w, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			if( confirmNo && !confirmMode ) {
				InitConfirmButtons( true ); return;
			}
			
			OnNoClick( g, w, mouseBtn );
			string url = ((string)Metadata).Substring( 3 );
			if( !game.DeniedUrls.HasEntry( url ) )
				game.DeniedUrls.AddEntry( url );
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