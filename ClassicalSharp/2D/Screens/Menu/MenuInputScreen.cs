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
			base.Render( delta );
			if( inputWidget != null )
				inputWidget.Render( delta );
			if( descWidget != null )
				descWidget.Render( delta );
		}
		
		public override bool HandlesKeyPress( char key ) {
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
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
			
			string text = widget.Text + " : " + widget.GetValue( game );
			descWidget = TextWidget.Create( game, 0, 100, text, Docking.Centre, Docking.Centre, regularFont );
		}
		
		protected void OnWidgetClick( Game game ) {
			if( selectedWidget == buttons[okayIndex] ) {
				string text = inputWidget.GetText();
				if( inputWidget.Validator.IsValidValue( text ) )
					targetWidget.SetValue( game, text );
				inputWidget.Dispose();
				inputWidget = null;
				UpdateDescription( targetWidget );
				targetWidget = null;
				buttons[okayIndex].Dispose();
				buttons[okayIndex] = null;
				return;
			}
			
			if( inputWidget != null )
				inputWidget.Dispose();
			targetWidget = selectedWidget;
			
			int index = Array.IndexOf<ButtonWidget>( buttons, selectedWidget );
			MenuInputValidator validator = validators[index];
			inputWidget = MenuInputWidget.Create( game, 0, 150, 400, 25, selectedWidget.GetValue( game ),
			                                     Docking.Centre, Docking.Centre, regularFont, titleFont,
			                                     hintFont, validator );
			buttons[okayIndex] = ButtonWidget.Create( game, 240, 150, 30, 30, "OK",
			                                         Docking.Centre, Docking.Centre, titleFont, OnWidgetClick );
			UpdateDescription( targetWidget );
		}
	}
}