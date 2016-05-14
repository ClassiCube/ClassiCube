// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class EditHotkeyScreen : MenuScreen {
		
		const int keyI = 0, modifyI = 1;
		HotkeyList hotkeys;
		public EditHotkeyScreen( Game game, Hotkey original ) : base( game ) {
			hotkeys = game.InputHandler.Hotkeys;
			origHotkey = original;
			curHotkey = original;
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			currentAction.Render( delta );
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
			return currentAction.HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.SetNewScreen( null );
				return true;
			} else if( focusWidget != null ) {
				FocusKeyDown( key );
				return true;
			}
			return currentAction.HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return currentAction.HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			titleFont = new Font( game.FontName, 16, FontStyle.Bold );
			string flags = HotkeyListScreen.MakeFlagsString( curHotkey.Flags );
			
			widgets = new Widget[] {
				Make( -140, 45, "Key: " + curHotkey.BaseKey,
				     250, 35, titleFont, BaseKeyClick ),
				Make( 140, 45, "Modifiers: " + flags,
				     250, 35, titleFont, ModifiersClick ),
				Make( -10, 110, curHotkey.MoreInput ? "yes" : "no",
				     50, 25, titleFont, LeaveOpenClick ),
				Make( -120, 150, "Save changes",
				     180, 35, titleFont, SaveChangesClick ),
				Make( 120, 150, "Remove hotkey",
				     180, 35, titleFont, RemoveHotkeyClick ),
				ChatTextWidget.Create(
					game, -150, 110, "Keep input bar open:",
					Anchor.Centre, Anchor.Centre, titleFont ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.SetNewScreen( new PauseScreen( g ) ) ),
			};
			currentAction = MenuInputWidget.Create(
				game, 0, 80, 600, 30, "", Anchor.Centre, Anchor.Centre,
				regularFont, titleFont, new StringValidator( 64 ) );
			
			if( curHotkey.Text == null ) curHotkey.Text = "";
			currentAction.SetText( curHotkey.Text );
		}
		
		public override void OnResize( int oldWidth, int oldHeight, int width, int height ) {
			currentAction.OnResize( oldWidth, oldHeight, width, height );
			base.OnResize( oldWidth, oldHeight, width, height );
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			currentAction.Dispose();
			focusWidget = null;
			base.Dispose();
		}
		
		ButtonWidget Make( int x, int y, string text, int width, int height,
		                  Font font, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, height, text,
			                           Anchor.Centre, Anchor.Centre, font, LeftOnly( onClick ) );
		}
		
		Hotkey curHotkey, origHotkey;
		MenuInputWidget currentAction;
		Widget focusWidget;
		
		void LeaveOpenClick( Game game, Widget widget ) {
			LostFocus();
			curHotkey.MoreInput = !curHotkey.MoreInput;
			SetButton( widgets[2], curHotkey.MoreInput ? "yes" : "no" );
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
			game.SetNewScreen( new HotkeyListScreen( game ) );
		}
		
		void RemoveHotkeyClick( Game game, Widget widget ) {
			if( origHotkey.BaseKey != Key.Unknown ) {
				hotkeys.RemoveHotkey( origHotkey.BaseKey, origHotkey.Flags );
				hotkeys.UserRemovedHotkey( origHotkey.BaseKey, origHotkey.Flags );
			}
			game.SetNewScreen( new HotkeyListScreen( game ) );
		}
		
		void BaseKeyClick( Game game, Widget widget ) {
			focusWidget = widgets[keyI];
			SetButton( widgets[keyI], "Key: press a key.." );
			supressNextPress = true;
		}
		
		void ModifiersClick( Game game, Widget widget ) {
			focusWidget = widgets[modifyI];
			SetButton( widgets[modifyI], "Modifiers: press a key.." );
			supressNextPress = true;
		}
		
		void FocusKeyDown( Key key ) {
			if( focusWidget == widgets[keyI] ) {
				curHotkey.BaseKey = key;
				SetButton( widgets[keyI], "Key: " + curHotkey.BaseKey );
				supressNextPress = true;
			} else if( focusWidget == widgets[9] ) {
				if( key == Key.ControlLeft || key == Key.ControlRight ) curHotkey.Flags |= 1;
				else if( key == Key.ShiftLeft || key == Key.ShiftRight ) curHotkey.Flags |= 2;
				else if( key == Key.AltLeft || key == Key.AltRight ) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				string flags = HotkeyListScreen.MakeFlagsString( curHotkey.Flags );
				SetButton( widgets[modifyI], "Modifiers: " + flags );
				supressNextPress = true;
			}
			focusWidget = null;
		}
		
		void LostFocus() {
			if( focusWidget == null ) return;
			
			if( focusWidget == widgets[keyI] ) {
				SetButton( widgets[keyI], "Key: " + curHotkey.BaseKey );
			} else if( focusWidget == widgets[modifyI] ) {
				string flags = HotkeyListScreen.MakeFlagsString( curHotkey.Flags );
				SetButton( widgets[modifyI], "Modifiers: " + flags );
			}
			focusWidget = null;
			supressNextPress = false;
		}
		
		void SetButton( Widget widget, string text ) {
			((ButtonWidget)widget).SetText( text );
		}
	}
}