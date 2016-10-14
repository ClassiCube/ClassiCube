// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Gui.Widgets;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Screens {
	public abstract class MenuOptionsScreen : MenuScreen {
		
		public MenuOptionsScreen( Game game ) : base( game ) {
		}
		
		protected MenuInputWidget inputWidget;
		protected MenuInputValidator[] validators;
		protected string[][] descriptions;
		protected TextGroupWidget extendedHelp;
		Font extendedHelpFont;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			int extClipY = extendedHelp == null ? 0 : widgets[widgets.Length - 3].Y;
			int extEndY = extendedHelp == null ? 0 : extendedHelp.Y + extendedHelp.Height;
			
			if( extendedHelp != null && extEndY <= extClipY ) {
				int x = game.Width / 2 - tableWidth / 2 - 5;
				int y = game.Height / 2 + extHelpY - 5;
				gfx.Draw2DQuad( x, y, tableWidth + 10, tableHeight + 10, tableCol );
			}
			
			gfx.Texturing = true;
			RenderMenuWidgets( delta );
			if( inputWidget != null )
				inputWidget.Render( delta );
			
			if( extendedHelp != null && extEndY <= extClipY )
				extendedHelp.Render( delta );
			gfx.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			extendedHelpFont = new Font( game.FontName, 16, FontStyle.Regular );
			game.Keyboard.KeyRepeat = true;
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.Gui.SetNewScreen( null );
				return true;
			} else if( (key == Key.Enter || key == Key.KeypadEnter)
			          && inputWidget != null ) {
				ChangeSetting();
				return true;
			}
			if( inputWidget == null )
				return key < Key.F1 || key > Key.F35;
			return inputWidget.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyUp( key );
		}
		
		public override void OnResize( int width, int height ) {
			base.OnResize( width, height );
			if( extendedHelp == null ) return;
			extendedHelp.OnResize( width, height );
		}
		
		public override void Dispose() {
			DisposeWidgets();
			game.Keyboard.KeyRepeat = false;
			extendedHelpFont.Dispose();
			DisposeExtendedHelp();
			base.Dispose();
		}
		
		protected ButtonWidget selectedWidget, targetWidget;
		protected override void WidgetSelected( Widget widget ) {
			ButtonWidget button = widget as ButtonWidget;
			if( selectedWidget == button || button == null ||
			   button == widgets[widgets.Length - 2] ) return;
			
			selectedWidget = button;
			if( targetWidget != null ) return;
			UpdateDescription( selectedWidget );
		}
		
		protected void UpdateDescription( ButtonWidget widget ) {
			DisposeExtendedHelp();
			if( widget == null || widget.GetValue == null ) return;
			
			ShowExtendedHelp();
		}
		
		protected virtual void InputOpened() { }
		
		protected virtual void InputClosed() { }
		
		protected ButtonWidget MakeTitle( int dir, int y, string text, ClickHandler onClick ) {
			ButtonWidget widget = ButtonWidget.Create( game, 160 * dir, y, 301, 41, text, Anchor.Centre,
			                                          Anchor.Centre, titleFont, onClick );
			return widget;
		}
		
		protected ButtonWidget MakeOpt( int dir, int y, string text, ClickHandler onClick,
		                               Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, 160 * dir, y, 301, 41,
			                                          text + ": " + getter( game ),
			                                          Anchor.Centre, Anchor.Centre, titleFont, onClick );
			widget.Metadata = text;
			widget.GetValue = getter;
			widget.SetValue = (g, v) => {
				setter( g, v );
				widget.SetText( (string)widget.Metadata + ": " + getter( g ) );
			};
			return widget;
		}
		
		protected ButtonWidget MakeBool( int dir, int y, string text, string optKey,
		                                ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter ) {
			string optName = text;
			text = text + ": " + (getter( game ) ? "ON" : "OFF");
			ButtonWidget widget = ButtonWidget.Create( game, 160 * dir, y, 301, 41, text, Anchor.Centre,
			                                          Anchor.Centre, titleFont, onClick );
			widget.Metadata = optName;
			widget.GetValue = g => getter( g ) ? "yes" : "no";
			widget.SetValue = (g, v) => {
				setter( g, v == "yes" );
				Options.Set( optKey, v == "yes" );
				widget.SetText( (string)widget.Metadata + ": " + (v == "yes" ? "ON" : "OFF") );
			};
			return widget;
		}
		
		void ShowExtendedHelp() {
			bool canShow = inputWidget == null && selectedWidget != null && descriptions != null;
			if( !canShow ) return;
			
			int index = Array.IndexOf<Widget>( widgets, selectedWidget );
			if( index < 0 || index >= descriptions.Length ) return;
			string[] desc = descriptions[index];
			if( desc == null ) return;
			MakeExtendedHelp( desc );
		}
		
		static FastColour tableCol = new FastColour( 20, 20, 20, 200 );
		int tableWidth, tableHeight;
		protected int extHelpY = 100;
		void MakeExtendedHelp( string[] desc ) {
			extendedHelp = new TextGroupWidget( game, desc.Length, extendedHelpFont, null,
			                                   Anchor.Centre, Anchor.Centre );
			extendedHelp.Init();
			
			for( int i = 0; i < desc.Length; i++ )
				extendedHelp.SetText( i, desc[i] );
			for( int i = 0; i < desc.Length; i++ )
				extendedHelp.Textures[i].X1 = extendedHelp.X;
			
			tableWidth = extendedHelp.Width;
			tableHeight = extendedHelp.Height;
			extendedHelp.MoveTo( extendedHelp.X, extHelpY + tableHeight / 2 );
		}
		
		void DisposeExtendedHelp() {
			if( extendedHelp == null ) return;
			extendedHelp.Dispose();
			extendedHelp = null;
		}
		
		protected void OnWidgetClick( Game game, Widget widget, MouseButton mouseBtn ) {
			ButtonWidget button = widget as ButtonWidget;
			if( mouseBtn != MouseButton.Left ) return;
			if( widget == widgets[widgets.Length - 1] ) {
				ChangeSetting(); return;
			}
			if( button == null ) return;
			DisposeExtendedHelp();
			
			int index = Array.IndexOf<Widget>( widgets, button );
			MenuInputValidator validator = validators[index];
			if( validator is BooleanValidator ) {
				string value = button.GetValue( game );
				button.SetValue( game, value == "yes" ? "no" : "yes" );
				UpdateDescription( button );
				return;
			} else if( validator is EnumValidator ) {
				Type type = ((EnumValidator)validator).EnumType;
				HandleEnumOption( button, type );
				return;
			}
			
			if( inputWidget != null )
				inputWidget.Dispose();
			
			targetWidget = selectedWidget;
			inputWidget = MenuInputWidget.Create( game, 0, 110, 400, 30, button.GetValue( game ), Anchor.Centre,
			                                     Anchor.Centre, regularFont, titleFont, validator );
			widgets[widgets.Length - 2] = inputWidget;
			widgets[widgets.Length - 1] = ButtonWidget.Create( game, 240, 110, 40, 30, "OK",
			                                                  Anchor.Centre, Anchor.Centre, titleFont, OnWidgetClick );
			InputOpened();
			UpdateDescription( targetWidget );
		}
		
		void HandleEnumOption( ButtonWidget button, Type type ) {
			string value = button.GetValue( game );
			int enumValue = (int)Enum.Parse( type, value, true );
			enumValue++;
			// go back to first value
			if( !Enum.IsDefined( type, enumValue ) )
				enumValue = 0;
			button.SetValue( game, Enum.GetName( type, enumValue ) );
			UpdateDescription( button );
		}
		
		void ChangeSetting() {
			string text = inputWidget.GetText();
			if( inputWidget.Validator.IsValidValue( text ) )
				targetWidget.SetValue( game, text );
			
			DisposeWidgets();
			UpdateDescription( targetWidget );
			targetWidget = null;
			InputClosed();
		}
		
		void DisposeWidgets() {
			if( inputWidget != null )
				inputWidget.Dispose();
			widgets[widgets.Length - 2] = null;
			inputWidget = null;
			
			int okayIndex = widgets.Length - 1;
			if( widgets[okayIndex] != null )
				widgets[okayIndex].Dispose();
			widgets[okayIndex] = null;
		}
	}
}