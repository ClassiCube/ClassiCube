using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp {
	
	public abstract class MenuInputScreen : MenuScreen {
		
		public MenuInputScreen( Game game ) : base( game ) {
		}
		
		protected MenuInputWidget inputWidget;
		protected MenuInputValidator[] validators;
		protected TextWidget descWidget;
		protected Font hintFont;
		protected int okayIndex;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			if( inputWidget != null )
				inputWidget.Render( delta );
			if( descWidget != null )
				descWidget.Render( delta );
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			game.Keyboard.KeyRepeat = true;
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( new NormalScreen( game ) );
				return true;
			} else if( (key == Key.Enter || key == Key.KeypadEnter) 
			          && inputWidget != null ) {
				ChangeSetting();
				return true;
			}
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyUp( key );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			if( descWidget != null )
				descWidget.OnResize( oldWidth, oldHeight, width, height );
			if( inputWidget != null )
				inputWidget.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			if( descWidget != null )
				descWidget.Dispose();
			if( inputWidget != null )
				inputWidget.Dispose();
			hintFont.Dispose();
			game.Keyboard.KeyRepeat = false;
			base.Dispose();
		}
		
		ButtonWidget selectedWidget, targetWidget;
		protected override void WidgetSelected( ButtonWidget widget ) {
			if( selectedWidget == widget || widget == null ||
			   widget == buttons[buttons.Length - 2] ) return;
			
			selectedWidget = widget;
			if( targetWidget != null ) return;
			UpdateDescription( selectedWidget );
		}
		
		protected void UpdateDescription( ButtonWidget widget ) {
			if( descWidget != null )
				descWidget.Dispose();
			
			string text = widget.Text + ": " + widget.GetValue( game );
			descWidget = TextWidget.Create( game, 0, 100, text, Docking.Centre, Docking.Centre, regularFont );
		}

		protected void OnWidgetClick( Game game, ButtonWidget widget ) {
			if( widget == buttons[okayIndex] ) {
				ChangeSetting();
				return;
			}
			
			int index = Array.IndexOf<ButtonWidget>( buttons, widget );
			MenuInputValidator validator = validators[index];
			if( validator is BooleanValidator ) {
				string value = widget.GetValue( game );
				widget.SetValue( game, value == "yes" ? "no" : "yes" );
				UpdateDescription( widget );
				return;
			}
			
			if( inputWidget != null )
				inputWidget.Dispose();
			
			targetWidget = selectedWidget;
			inputWidget = MenuInputWidget.Create( game, 0, 150, 400, 25, widget.GetValue( game ),
			                                     Docking.Centre, Docking.Centre, regularFont, titleFont,
			                                     hintFont, validator );
			buttons[okayIndex] = ButtonWidget.Create( game, 240, 150, 30, 30, "OK",
			                                         Docking.Centre, Docking.Centre, titleFont, OnWidgetClick );
			UpdateDescription( targetWidget );
		}
		
		void ChangeSetting() {
			string text = inputWidget.GetText();
			if( inputWidget.Validator.IsValidValue( text ) )
				targetWidget.SetValue( game, text );
			inputWidget.Dispose();
			inputWidget = null;
			UpdateDescription( targetWidget );
			targetWidget = null;
			buttons[okayIndex].Dispose();
			buttons[okayIndex] = null;
		}
	}
}