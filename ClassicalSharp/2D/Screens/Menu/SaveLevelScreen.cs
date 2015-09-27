using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public sealed class SaveLevelScreen : MenuScreen {
		
		public SaveLevelScreen( Game game ) : base( game ) {
		}
		
		MenuInputWidget inputWidget;
		Font hintFont;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			inputWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override bool HandlesKeyPress( char key ) {
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( new NormalScreen( game ) );
				return true;
			}
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return inputWidget.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			
			inputWidget = MenuInputWidget.Create(
				game, 0, 50, 400, 25, "", Docking.Centre, Docking.Centre,
				regularFont, titleFont, hintFont, new PathValidator() );
			buttons = new [] {
				ButtonWidget.Create( game, 240, 50, 30, 30, "OK", Docking.Centre,
				                    Docking.Centre, titleFont, OkButtonClick ),
				ButtonWidget.Create( game, 0, 5, 240, 35, "Back to menu", Docking.Centre, Docking.BottomOrRight,
				                    titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			inputWidget.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			inputWidget.Dispose();
			hintFont.Dispose();
			base.Dispose();
		}
		
		void OkButtonClick( Game game, ButtonWidget widget ) {
			string text = inputWidget.GetText();
			
			if( inputWidget.Validator.IsValidValue( text ) )
				game.AddChat( "okay path" );
			game.SetNewScreen( new PauseScreen( game ) );
		}
	}
}