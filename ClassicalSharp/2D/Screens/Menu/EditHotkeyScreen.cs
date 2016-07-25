// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using ClassicalSharp.Hotkeys;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed class EditHotkeyScreen : MenuScreen {
		
		const int keyI = 0, modifyI = 1, actionI = 2;
		HotkeyList hotkeys;
		Hotkey curHotkey, origHotkey;
		Widget focusWidget;
		static FastColour grey = new FastColour( 150, 150, 150 );
		
		public EditHotkeyScreen( Game game, Hotkey original ) : base( game ) {
			hotkeys = game.InputHandler.Hotkeys;
			origHotkey = original;
			curHotkey = original;
		}
		
		public override void Render( double delta ) {
			RenderMenuBounds();
			api.Texturing = true;
			RenderMenuWidgets( delta );
			api.Texturing = false;
			
			float cX = game.Width / 2, cY = game.Height / 2;
			api.Draw2DQuad( cX - 250, cY - 65, 500, 2, grey );
			api.Draw2DQuad( cX - 250, cY + 45, 500, 2, grey );
		}
		
		public override bool HandlesMouseMove( int mouseX, int mouseY ) {
			return HandleMouseMove( widgets, mouseX, mouseY );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			return HandleMouseClick( widgets, mouseX, mouseY, button );
		}
		
		bool supressNextPress;
		public override bool HandlesKeyPress( char key ) {
			if( supressNextPress ) {
				supressNextPress = false;
				return true;
			}
			return widgets[actionI].HandlesKeyPress( key );
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( key == Key.Escape ) {
				game.Gui.SetNewScreen( null );
				return true;
			} else if( focusWidget != null ) {
				FocusKeyDown( key );
				return true;
			}
			return widgets[actionI].HandlesKeyDown( key );
		}
		
		public override bool HandlesKeyUp( Key key ) {
			return widgets[actionI].HandlesKeyUp( key );
		}
		
		public override void Init() {
			game.Keyboard.KeyRepeat = true;
			base.Init();
			regularFont = new Font( game.FontName, 16, FontStyle.Regular );
			string flags = HotkeyListScreen.MakeFlagsString( curHotkey.Flags );
			if( curHotkey.Text == null ) curHotkey.Text = "";
			string staysOpen = curHotkey.StaysOpen ? "yes" : "no";
			bool existed = origHotkey.BaseKey != Key.Unknown;
			
			widgets = new Widget[] {
				Make( 0, -150, "Key: " + curHotkey.BaseKey,
				     301, 40, titleFont, BaseKeyClick ),
				Make( 0, -100, "Modifiers:" + flags,
				     301, 40, titleFont, ModifiersClick ),
				
				MenuInputWidget.Create(
					game, 0, -35, 500, 30, curHotkey.Text,
					Anchor.Centre, Anchor.Centre,
					regularFont, titleFont, new StringValidator( 64 ) ),			
				Make( -100, 10, "Input stays open: " + staysOpen,
				     301, 40, titleFont, LeaveOpenClick ),
				
				Make( 0, 80, existed ? "Save changes" : "Add hotkey",
				     301, 40, titleFont, SaveChangesClick ),
				Make( 0, 130, existed ? "Remove hotkey" : "Cancel",
				     301, 40, titleFont, RemoveHotkeyClick ),
				
				MakeBack( false, titleFont,
				         (g, w) => g.Gui.SetNewScreen( new PauseScreen( g ) ) ),
			};
		}
		
		public override void Dispose() {
			game.Keyboard.KeyRepeat = false;
			focusWidget = null;
			base.Dispose();
		}
		
		ButtonWidget Make( int x, int y, string text, int width, int height,
		                  Font font, Action<Game, Widget> onClick ) {
			return ButtonWidget.Create( game, x, y, width, height, text,
			                           Anchor.Centre, Anchor.Centre, font, LeftOnly( onClick ) );
		}
		
		void LeaveOpenClick( Game game, Widget widget ) {
			LostFocus();
			curHotkey.StaysOpen = !curHotkey.StaysOpen;
			string staysOpen = curHotkey.StaysOpen ? "yes" : "no";
			staysOpen = "Input stays open: " + staysOpen;
			SetButton( widgets[3], staysOpen );
		}
		
		void SaveChangesClick( Game game, Widget widget ) {
			if( origHotkey.BaseKey != Key.Unknown ) {
				hotkeys.RemoveHotkey( origHotkey.BaseKey, origHotkey.Flags );
				hotkeys.UserRemovedHotkey( origHotkey.BaseKey, origHotkey.Flags );
			}
			MenuInputWidget input = (MenuInputWidget)widgets[actionI];
			
			if( curHotkey.BaseKey != Key.Unknown ) {
				hotkeys.AddHotkey( curHotkey.BaseKey, curHotkey.Flags,
				                  input.GetText(), curHotkey.StaysOpen );
				hotkeys.UserAddedHotkey( curHotkey.BaseKey, curHotkey.Flags,
				                        curHotkey.StaysOpen, input.GetText() );
			}
			game.Gui.SetNewScreen( new HotkeyListScreen( game ) );
		}
		
		void RemoveHotkeyClick( Game game, Widget widget ) {
			if( origHotkey.BaseKey != Key.Unknown ) {
				hotkeys.RemoveHotkey( origHotkey.BaseKey, origHotkey.Flags );
				hotkeys.UserRemovedHotkey( origHotkey.BaseKey, origHotkey.Flags );
			}
			game.Gui.SetNewScreen( new HotkeyListScreen( game ) );
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
			} else if( focusWidget == widgets[modifyI] ) {
				if( key == Key.ControlLeft || key == Key.ControlRight ) curHotkey.Flags |= 1;
				else if( key == Key.ShiftLeft || key == Key.ShiftRight ) curHotkey.Flags |= 2;
				else if( key == Key.AltLeft || key == Key.AltRight ) curHotkey.Flags |= 4;
				else curHotkey.Flags = 0;
				
				string flags = HotkeyListScreen.MakeFlagsString( curHotkey.Flags );
				SetButton( widgets[modifyI], "Modifiers:" + flags );
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
				SetButton( widgets[modifyI], "Modifiers:" + flags );
			}
			focusWidget = null;
			supressNextPress = false;
		}
		
		void SetButton( Widget widget, string text ) {
			((ButtonWidget)widget).SetText( text );
		}
	}
}