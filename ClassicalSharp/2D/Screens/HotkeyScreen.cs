using System;
using System.Drawing;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp {
	
	// TODO: Hotkey added event for CPE
	public sealed class HotkeyScreen : MenuScreen {
		
		HotkeyList hotkeys;
		public HotkeyScreen( Game game ) : base( game ) {
			hotkeys = game.InputHandler.Hotkeys;
		}
		
		Font hintFont, arrowFont, textFont;
		public override void Render( double delta ) {
			RenderMenuBounds();
			graphicsApi.Texturing = true;
			RenderMenuButtons( delta );
			
			if( currentAction != null ) {
				currentAction.Render( delta );
				currentMoreInputLabel.Render( delta );
			}
			graphicsApi.Texturing = false;
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( buttons, mouseX, mouseY );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( buttons, mouseX, mouseY, button );
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
			titleFont = new Font( "Arial", 16, FontStyle.Bold );
			regularFont = new Font( "Arial", 16, FontStyle.Regular );
			hintFont = new Font( "Arial", 14, FontStyle.Italic );
			arrowFont = new Font( "Arial", 18, FontStyle.Bold );
			textFont = new Font( "Arial", 14, FontStyle.Bold );
			
			buttons = new [] {
				MakeHotkey( 0, -160, 0 ),
				MakeHotkey( 0, -120, 1 ),
				MakeHotkey( 0, -80, 2 ),
				MakeHotkey( 0, -40, 3 ),
				MakeHotkey( 0, 0, 4 ),
				Make( -160, -80, "<", 40, 40, arrowFont, (g, w) => PageClick( false ) ),
				Make( 160, -80, ">", 40, 40, arrowFont, (g, w) => PageClick( true ) ),
				
				ButtonWidget.Create( game, 0, 5, 240, 35, "Back to menu", Anchor.Centre, Anchor.BottomOrRight,
				                    titleFont, (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
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
			
			hintFont.Dispose();
			arrowFont.Dispose();
			textFont.Dispose();
			base.Dispose();
		}
		
		ButtonWidget Make( int x, int y, string text, int width, int height,
		                  Font font, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, height, text,
			                           Anchor.Centre, Anchor.Centre, font, onClick );
		}
		
		ButtonWidget MakeHotkey( int x, int y, int index ) {
			string text = Get( index );
			ButtonWidget button = ButtonWidget.Create(
				game, x, y, 240, 30, text, Anchor.Centre, Anchor.Centre,
				textFont, TextButtonClick );
			
			button.Metadata = default( Hotkey );
			if( text != "-----" )
				button.Metadata =  hotkeys.Hotkeys[index];
			return button;
		}
		
		string Get( int index ) {
			if( index >= hotkeys.Hotkeys.Count ) return "-----";
			
			Hotkey hKey = hotkeys.Hotkeys[index];
			return hKey.BaseKey + " | " + MakeFlagsString( hKey.Flags );
		}
		
		void Set( int index ) {
			string text = Get( index + currentIndex );
			ButtonWidget button = buttons[index];
			button.SetText( text );
			button.Metadata = default( Hotkey );
			if( text != "-----" )
				button.Metadata =  hotkeys.Hotkeys[index];
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
			if( currentIndex >= hotkeys.Hotkeys.Count )
				currentIndex -= numButtons;
			if( currentIndex < 0 ) currentIndex = 0;
			
			LostFocus();
			for( int i = 0; i < numButtons; i++ )
				Set( i );
		}
		
		void TextButtonClick( Game game, Widget widget ) {
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
		ButtonWidget focusWidget;
		
		void CreateEditingWidgets() {
			DisposeEditingWidgets();
			
			buttons[8] = Make( -140, 60, "Key: " + curHotkey.BaseKey,
			                  250, 30, textFont, BaseKeyClick );
			buttons[9] = Make( 140, 60, "Modifiers: " + MakeFlagsString( curHotkey.Flags ),
			                  250, 30, textFont, ModifiersClick );
			buttons[10] = Make( -10, 120, curHotkey.MoreInput ? "yes" : "no",
			                   50, 25, textFont, LeaveOpenClick );
			buttons[11] = Make( -100, 160, "Save changes",
			                   160, 30, textFont, SaveChangesClick );
			buttons[12] = Make( 100, 160, "Remove hotkey",
			                   160, 30, textFont, RemoveHotkeyClick );
			
			currentAction = MenuInputWidget.Create(
				game, 0, 90, 600, 25, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, hintFont, new StringValidator( 64 ) );
			currentMoreInputLabel = TextWidget.Create(
				game, -170, 120, "Keep input bar open:",
				Anchor.Centre, Anchor.Centre, textFont );
			
			if( curHotkey.Text == null ) curHotkey.Text = "";
			currentAction.SetText( curHotkey.Text );
		}
		
		void DisposeEditingWidgets() {
			if( currentAction != null ) {
				currentAction.Dispose();
				currentAction = null;
			}
			
			for( int i = 8; i < buttons.Length - 1; i++ ) {
				if( buttons[i] != null ) {
					buttons[i].Dispose();
					buttons[i] = null;
				}
			}
			focusWidget = null;
		}
		
		void LeaveOpenClick( Game game, Widget widget ) {
			LostFocus();
			curHotkey.MoreInput = !curHotkey.MoreInput;
			buttons[10].SetText( curHotkey.MoreInput ? "yes" : "no" );
		}
		
		void SaveChangesClick( Game game, Widget widget ) {
			if( origHotkey.BaseKey != Key.Unknown ) {
				hotkeys.RemoveHotkey( origHotkey.BaseKey, origHotkey.Flags );
				hotkeys.UserRemovedHotkey( origHotkey.BaseKey, origHotkey.Flags );
			}
			
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
			focusWidget = buttons[8];
			focusWidget.SetText( "Key: press a key.." );
			supressNextPress = true;
		}
		
		void ModifiersClick( Game game, Widget widget ) {
			focusWidget = buttons[9];
			focusWidget.SetText( "Modifiers: press a key.." );
			supressNextPress = true;
		}
		
		void FocusKeyDown( Key key ) {
			if( focusWidget == buttons[8] ) {
				curHotkey.BaseKey = key;
				buttons[8].SetText( "Key: " + curHotkey.BaseKey );
				supressNextPress = true;
			} else if( focusWidget == buttons[9] ) {
				if( key == Key.ControlLeft || key == Key.ControlRight ) curHotkey.Flags |= 1;
				else if( key == Key.ShiftLeft || key == Key.ShiftRight ) curHotkey.Flags |= 2;
				else if( key == Key.AltLeft || key == Key.AltRight ) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				buttons[9].SetText( "Modifiers: " + MakeFlagsString( curHotkey.Flags ) );
				supressNextPress = true;
			}
			focusWidget = null;
		}
		
		void LostFocus() {
			if( focusWidget == null ) return;
			
			if( focusWidget == buttons[8] ) {
				buttons[8].SetText( "Key: " + curHotkey.BaseKey );
			} else if( focusWidget == buttons[9] ) {
				buttons[9].SetText( "Modifiers: " + MakeFlagsString( curHotkey.Flags ) );
			}
			focusWidget = null;
			supressNextPress = false;
		}
		
		#endregion
	}
}