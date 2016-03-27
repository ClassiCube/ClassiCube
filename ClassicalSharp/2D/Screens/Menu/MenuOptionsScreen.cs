// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public abstract class MenuOptionsScreen : MenuScreen {
		
		public MenuOptionsScreen( Game game ) : base( game ) {
		}
		
		protected MenuInputWidget inputWidget;
		protected MenuInputValidator[] validators;
		protected TextWidget descWidget;
		protected string[][] descriptions;
		protected ChatTextWidget[] extendedHelp;
		Font extendedHelpFont;
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			int extEndY = extendedHelp == null ? 0 : extendedHelp[extendedHelp.Length - 1].BottomRight.Y;
			if( extendedHelp != null && extEndY <= widgets[widgets.Length - 3].Y ) {
				int x = game.Width / 2 - tableWidth / 2 - 5;
				int y = game.Height / 2 + extHelpY - 5;
				graphicsApi.Draw2DQuad( x, y, tableWidth + 10, tableHeight + 10, tableCol );
			}
			
			graphicsApi.Texturing = true;
			RenderMenuWidgets( delta );
			if( inputWidget != null )
				inputWidget.Render( delta );
			
			
			if( extendedHelp != null && extEndY <= widgets[widgets.Length - 3].Y ) {
				for( int i = 0; i < extendedHelp.Length; i++ )
					extendedHelp[i].Render( delta );
			}
			if( descWidget != null )
				descWidget.Render( delta );
			
			graphicsApi.Texturing = false;
		}
		
		public override void Init() {
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			int size = game.Drawer2D.UseBitmappedChat ? 11 : 12;
			extendedHelpFont = new Font( game.FontName, size, FontStyle.Regular );
			game.Keyboard.KeyRepeat = true;
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( inputWidget == null ) return true;
			return inputWidget.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
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
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			if( descWidget != null )
				descWidget.OnResize( oldWidth, oldHeight, width, height );
			if( inputWidget != null )
				inputWidget.OnResize( oldWidth, oldHeight, width, height );
			
			base.OnResize( oldWidth, oldHeight, width, height );
			if( extendedHelp == null ) return;
			for( int i = 0; i < extendedHelp.Length; i++ )
				extendedHelp[i].OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			if( descWidget != null ) {
				descWidget.Dispose();
				descWidget = null;
			}
			if( inputWidget != null ) {
				inputWidget.Dispose();
				inputWidget = null;
			}
			
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
			if( descWidget != null )
				descWidget.Dispose();
			DisposeExtendedHelp();
			if( widget == null || widget.GetValue == null ) return;
			
			int index = Array.IndexOf<Widget>( widgets, widget );			
			if( index >= validators.Length || !(validators[index] is BooleanValidator) ) {
				string text = widget.Text + ": " + widget.GetValue( game );
				descWidget = TextWidget.Create( game, 0, 100, text, Anchor.Centre, Anchor.Centre, regularFont );
			}
			ShowExtendedHelp();
		}
		
		protected virtual void InputOpened() { }
		
		protected virtual void InputClosed() { }
		
		protected ButtonWidget Make( int dir, int y, string text, ClickHandler onClick,
		                            Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, 160 * dir, y, 280, 35, text, Anchor.Centre,
			                                          Anchor.Centre, titleFont, onClick );
			widget.GetValue = getter; widget.SetValue = setter;
			return widget;
		}
		
		protected ButtonWidget MakeBool( int dir, int y, string optName, string optKey, 
		                                       ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter ) {
			return MakeBoolImpl( 160 * dir, y, 280, 35, optName, optKey, onClick, getter, setter );
		}
		
		protected ButtonWidget MakeClassic( int dir, int y, string text, ClickHandler onClick,
		                                   Func<Game, string> getter, Action<Game, string> setter ) {
			ButtonWidget widget = ButtonWidget.Create( game, 165 * dir, y, 301, 41, text, Anchor.Centre,
			                                          Anchor.Centre, titleFont, onClick );
			widget.GetValue = getter; widget.SetValue = setter;
			return widget;
		}
		
		protected ButtonWidget MakeClassicBool( int dir, int y, string text, string optKey, 
		                                       ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter ) {
			return MakeBoolImpl( 165 * dir, y, 301, 41, text, optKey, onClick, getter, setter );
		}
		
		ButtonWidget MakeBoolImpl( int x, int y, int width, int height, string text, string optKey, 
		                                       ClickHandler onClick, Func<Game, bool> getter, Action<Game, bool> setter ) {
			string optName = text;
			text = text + ": " + (getter( game ) ? "ON" : "OFF");
			ButtonWidget widget = ButtonWidget.Create( game, x, y, width, height, text, Anchor.Centre,
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
			string[] desc = descriptions[index];
			if( desc == null ) return;
			MakeExtendedHelp( desc );
		}
		
		static FastColour tableCol = new FastColour( 20, 20, 20, 200 );
		int tableWidth, tableHeight;
		const int extHelpY = 120;
		void MakeExtendedHelp( string[] desc ) {
			extendedHelp = new ChatTextWidget[desc.Length];
			int x = 0, y = extHelpY;
			tableWidth = 0;
			
			for( int i = 0; i < desc.Length; i++ ) {
				extendedHelp[i] = ChatTextWidget.Create( game, 0, y,
				                                        desc[i], Anchor.Centre, Anchor.Centre, extendedHelpFont );
				tableWidth = Math.Max( extendedHelp[i].Width, tableWidth );
				y += extendedHelp[i].Height + 5;
			}
			tableHeight = y - extHelpY;
			
			int yOffset = 0;
			for( int i = 0; i < desc.Length; i++ ) {
				ChatTextWidget widget = extendedHelp[i];
				widget.XOffset = (widget.Width - tableWidth) / 2;
				x = CalcOffset( game.Width, widget.Width, widget.XOffset, Anchor.Centre );
				
				widget.YOffset = yOffset + extHelpY + extendedHelp[0].Height / 2;
				y = CalcOffset( game.Height, widget.Height, widget.YOffset, Anchor.Centre );
				yOffset += extendedHelp[i].Height + 5;
				widget.MoveTo( x, y );
			}
		}
		
		void DisposeExtendedHelp() {
			if( extendedHelp == null ) return;
			for( int i = 0; i < extendedHelp.Length; i++ )
				extendedHelp[i].Dispose();
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
				HandleEnumOption( button );
				return;
			}
			
			if( inputWidget != null )
				inputWidget.Dispose();
			
			targetWidget = selectedWidget;
			inputWidget = MenuInputWidget.Create( game, 0, 150, 400, 30, button.GetValue( game ), Anchor.Centre,
			                                     Anchor.Centre, regularFont, titleFont, validator );
			widgets[widgets.Length - 2] = inputWidget;
			widgets[widgets.Length - 1] = ButtonWidget.Create( game, 240, 150, 40, 30, "OK",
			                                                  Anchor.Centre, Anchor.Centre, titleFont, OnWidgetClick );
			InputOpened();
			UpdateDescription( targetWidget );
		}
		
		void HandleEnumOption( ButtonWidget button ) {
			string value = button.GetValue( game );
			Type type = (Type)button.Metadata;
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
			if( inputWidget != null )
				inputWidget.Dispose();
			widgets[widgets.Length - 2] = null;
			inputWidget = null;
			UpdateDescription( targetWidget );
			targetWidget = null;
			
			int okayIndex = widgets.Length - 1;
			if( widgets[okayIndex] != null )
				widgets[okayIndex].Dispose();
			widgets[okayIndex] = null;
			InputClosed();
		}
	}
}