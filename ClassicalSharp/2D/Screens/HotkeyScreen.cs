// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	// TODO: Hotkey added event for CPE
	public sealed class HotkeyScreen : MenuScreen {
		
		HotkeyList hotkeys;
		public HotkeyScreen( Game game ) : base( game ) {
			hotkeys = game.InputHandler.Hotkeys;
		}
		
		Font arrowFont, textFont;
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			
			if( currentAction != null ) {
				currentAction.Render( delta );
				currentMoreInputLabel.Render( delta );
			}
			api.Texturing = false;
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( widgets, mouseX, mouseY );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( widgets, mouseX, mouseY, button );
		}
		
		bool supressNextPress;
		const int numButtons = 5;
		public override bool HandlesKeyPress( char key ) {
			if( supressNextPress ) {
				supressNextPress = false;
				return true;
			}
			
			return currentAction == null ? false :
				currentAction.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
				return true;
			} else if( focusWidget != null ) {
				FocusKeyDown( key );
				return true;
			}
			
			if( key == Key.Left ) {
				PageClick( false );
				return true;
			} else if( key == Key.Right ) {
				PageClick( true );
				return true;
			}
			return currentAction == null ? false :
				currentAction.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return currentAction == null ? false :
				currentAction.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			arrowFont = new Font( game.FontName, 18, FontStyle.Bold );
			textFont = new Font( game.FontName, 16, FontStyle.Bold );
			
			widgets = new [] {
				MakeHotkey( 0, -180, 0 ),
				MakeHotkey( 0, -140, 1 ),
				MakeHotkey( 0, -100, 2 ),
				MakeHotkey( 0, -60, 3 ),
				MakeHotkey( 0, -20, 4 ),
				Make( -160, -100, "<", 40, 40, arrowFont, (g, w) => PageClick( false ) ),
				Make( 160, -100, ">", 40, 40, arrowFont, (g, w) => PageClick( true ) ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
				null, // current key
				null, // current modifiers
				null, // leave open current
				null, // save current
				null, // remove current
				null,
			};
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			if( currentAction != null ) {
				currentAction.OnResize( oldWidth, oldHeight, width, height );
				currentMoreInputLabel.OnResize( oldWidth, oldHeight, width, height );
			}
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			DisposeEditingWidgets();
			
			arrowFont.Dispose();
			textFont.Dispose();
			base.Dispose();
		}
		
		ButtonWidget Make( int x, int y, string text, int width, int height,
		                  Font font, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, height, text,
			                           Anchor.Centre, Anchor.Centre, font, LeftOnly( onClick ) );
		}
		
		ButtonWidget MakeHotkey( int x, int y, int index ) {
			string text = Get( index + currentIndex );
			ButtonWidget button = ButtonWidget.Create(
				game, x, y, 240, 30, text, Anchor.Centre, Anchor.Centre,
				textFont, TextButtonClick );
			
			button.Metadata = default( Hotkey );
			if( text != "-----" )
				button.Metadata =  hotkeys.Hotkeys[index + currentIndex];
			return button;
		}
		
		string Get( int index ) {
			if( index >= hotkeys.Hotkeys.Count ) return "-----";
			
			Hotkey hKey = hotkeys.Hotkeys[index];
			return hKey.BaseKey + " | " + MakeFlagsString( hKey.Flags );
		}
		
		void Set( int index ) {
			string text = Get( index + currentIndex );
			Widget widget = widgets[index];
			SetButton( widget, text );
			
			widget.Metadata = default( Hotkey );
			if( text != "-----" )
				widget.Metadata = hotkeys.Hotkeys[index + currentIndex];
		}
		
		string MakeFlagsString( byte flags ) {
			if( flags == 0 ) return "None";
			
			return ((flags & 1) == 0 ? " " : "Ctrl ") +
				((flags & 2) == 0 ? " " : "Shift ") +
				((flags & 4) == 0 ? " " : "Alt ");
		}
		
		int currentIndex;
		void PageClick( bool forward ) {
			currentIndex += forward ? numButtons : -numButtons;
			if( currentIndex >= hotkeys.Hotkeys.Count + numButtons )
				currentIndex -= numButtons;
			if( currentIndex < 0 ) currentIndex = 0;
			
			LostFocus();
			for( int i = 0; i < numButtons; i++ )
				Set( i );
		}
		
		void TextButtonClick( Game game, Widget widget, MouseButton mouseBtn ) {
			if( mouseBtn != MouseButton.Left ) return;
			LostFocus();
			ButtonWidget button = (ButtonWidget)widget;
			
			curHotkey = (Hotkey)button.Metadata;
			origHotkey = curHotkey;
			CreateEditingWidgets();
		}
		
		#region Modifying hotkeys
		
		Hotkey curHotkey, origHotkey;
		MenuInputWidget currentAction;
		TextWidget currentMoreInputLabel;
		Widget focusWidget;
		
		void CreateEditingWidgets() {
			DisposeEditingWidgets();
			
			widgets[8] = Make( -140, 45, "Key: " + curHotkey.BaseKey,
			                  250, 35, textFont, BaseKeyClick );
			widgets[9] = Make( 140, 45, "Modifiers: " + MakeFlagsString( curHotkey.Flags ),
			                  250, 35, textFont, ModifiersClick );
			widgets[10] = Make( -10, 110, curHotkey.MoreInput ? "yes" : "no",
			                   50, 25, textFont, LeaveOpenClick );
			widgets[11] = Make( -120, 150, "Save changes",
			                   180, 35, textFont, SaveChangesClick );
			widgets[12] = Make( 120, 150, "Remove hotkey",
			                   180, 35, textFont, RemoveHotkeyClick );
			
			currentAction = MenuInputWidget.Create(
				game, 0, 80, 600, 30, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, new StringValidator( 64 ) );
			currentMoreInputLabel = ChatTextWidget.Create(
				game, -150, 110, "Keep input bar open:",
				Anchor.Centre, Anchor.Centre, titleFont );
			
			if( curHotkey.Text == null ) curHotkey.Text = "";
			currentAction.SetText( curHotkey.Text );
		}
		
		void DisposeEditingWidgets() {
			if( currentAction != null ) {
				currentAction.Dispose();
				currentAction = null;
			}
			
			for( int i = 8; i < widgets.Length - 1; i++ ) {
				if( widgets[i] != null ) {
					widgets[i].Dispose();
					widgets[i] = null;
				}
			}
			focusWidget = null;
		}
		
		void LeaveOpenClick( Game game, Widget widget ) {
			LostFocus();
			curHotkey.MoreInput = !curHotkey.MoreInput;
			SetButton( widgets[10], curHotkey.MoreInput ? "yes" : "no" );
		}
		
		void SaveChangesClick( Game game, Widget widget ) {
			if( curHotkey.BaseKey != Key.Unknown ) {
				hotkeys.AddHotkey( curHotkey.BaseKey, curHotkey.Flags,
				                  currentAction.GetText(), curHotkey.MoreInput );
				hotkeys.UserAddedHotkey( curHotkey.BaseKey, curHotkey.Flags,
				                        curHotkey.MoreInput, currentAction.GetText() );
			}
			
			for( int i = 0; i < numButtons; i++ )
				Set( i );
			DisposeEditingWidgets();
		}
		
		void RemoveHotkeyClick( Game game, Widget widget ) {
			if( origHotkey.BaseKey != Key.Unknown ) {
				hotkeys.RemoveHotkey( origHotkey.BaseKey, origHotkey.Flags );
				hotkeys.UserRemovedHotkey( origHotkey.BaseKey, origHotkey.Flags );
			}
			
			for( int i = 0; i < numButtons; i++ )
				Set( i );
			DisposeEditingWidgets();
		}
		
		void BaseKeyClick( Game game, Widget widget ) {
			focusWidget = widgets[8];
			SetButton( widgets[8], "Key: press a key.." );
			supressNextPress = true;
		}
		
		void ModifiersClick( Game game, Widget widget ) {
			focusWidget = widgets[9];
			SetButton( widgets[9], "Modifiers: press a key.." );
			supressNextPress = true;
		}
		
		void FocusKeyDown( Key key ) {
			if( focusWidget == widgets[8] ) {
				curHotkey.BaseKey = key;
				SetButton( widgets[8], "Key: " + curHotkey.BaseKey );
				supressNextPress = true;
			} else if( focusWidget == widgets[9] ) {
				if( key == Key.ControlLeft || key == Key.ControlRight ) curHotkey.Flags |= 1;
				else if( key == Key.ShiftLeft || key == Key.ShiftRight ) curHotkey.Flags |= 2;
				else if( key == Key.AltLeft || key == Key.AltRight ) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				SetButton( widgets[9], "Modifiers: " + MakeFlagsString( curHotkey.Flags ) );
				supressNextPress = true;
			}
			focusWidget = null;
		}
		
		void LostFocus() {
			if( focusWidget == null ) return;
			
			if( focusWidget == widgets[8] ) {
				SetButton( widgets[8], "Key: " + curHotkey.BaseKey );
			} else if( focusWidget == widgets[9] ) {
				SetButton( widgets[9], "Modifiers: " + MakeFlagsString( curHotkey.Flags ) );
			}
			focusWidget = null;
			supressNextPress = false;
		}
		
		void SetButton( Widget widget, string text ) {
			((ButtonWidget)widget).SetText( text );
		}
		
		#endregion
	}
}