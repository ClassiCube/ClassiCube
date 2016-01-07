using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed partial class TextInputWidget : Widget {

		public override bool HandlesKeyPress( char key ) {
			if( game.HideGui )
				return true;
			if( chatInputText.Length < len && IsValidInputChar( key ) && key != '&' ) {
				if( caretPos == -1 ) {
					chatInputText.Append( chatInputText.Length, key );
				} else {
					chatInputText.InsertAt( caretPos, key );
					caretPos++;
				}
				Dispose();
				Init();
				return true;
			}
			return false;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( game.HideGui )
				return key < Key.F1 || key > Key.F35;
			if( key == Key.Down ) DownKey();
			else if( key == Key.Up ) UpKey();
			else if( key == Key.Left ) LeftKey();
			else if( key == Key.Right ) RightKey();
			else if( key == Key.BackSpace ) BackspaceKey();
			else if( key == Key.Delete ) DeleteKey();
			else if( key == Key.Home ) HomeKey();
			else if( key == Key.End ) EndKey();
			else if( game.Network.ServerSupportsFullCP437 &&
			        key == game.InputHandler.Keys[KeyBinding.ExtendedInput] )
				altText.SetActive( !altText.Active );
			else if( !OtherKey( key ) ) return false;
			
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( altText.Active && altText.Bounds.Contains( mouseX, mouseY ) ) {
				altText.HandlesMouseClick( mouseX, mouseY, button );
				altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
				altText.Y = altText.texture.Y1;
			}
			return true;
		}
		
		void BackspaceKey() {
			if( !chatInputText.Empty && caretPos != 0 ) {
				if( caretPos == -1 ) {
					chatInputText.DeleteAt( chatInputText.Length - 1 );
				} else {
					caretPos--;
					chatInputText.DeleteAt( caretPos );
				}
				Dispose();
				Init();
			}
		}
		
		void DeleteKey() {
			if( !chatInputText.Empty && caretPos != -1 ) {
				chatInputText.DeleteAt( caretPos );
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void RightKey() {
			if( !chatInputText.Empty && caretPos != -1 ) {
				caretPos++;
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void LeftKey() {
			if( !chatInputText.Empty ) {
				if( caretPos == -1 ) caretPos = chatInputText.Length;
				caretPos--;
				if( caretPos < 0 ) caretPos = 0;
				Dispose();
				Init();
			}
		}
		
		string originalText;
		void UpKey() {
			if( typingLogPos == game.Chat.InputLog.Count )
				originalText = chatInputText.ToString();
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				chatInputText.Clear();
				chatInputText.Append( 0, game.Chat.InputLog[typingLogPos] );
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void DownKey() {
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos++;
				chatInputText.Clear();
				if( typingLogPos >= game.Chat.InputLog.Count ) {
					typingLogPos = game.Chat.InputLog.Count;
					if( originalText != null )
						chatInputText.Append( 0, originalText );
				} else {
					chatInputText.Append( 0, game.Chat.InputLog[typingLogPos] );
				}
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void HomeKey() {
			if( chatInputText.Empty ) return;
			
			caretPos = 0;
			Dispose();
			Init();
		}
		
		void EndKey() {
			caretPos = -1;
			Dispose();
			Init();
		}
		
		bool OtherKey( Key key ) {
			bool controlDown = game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight );
			if( key == Key.V && controlDown && chatInputText.Length < len ) {
				string text = Clipboard.GetText();
				if( String.IsNullOrEmpty( text ) ) return true;
				
				for( int i = 0; i < text.Length; i++ ) {
					if( !IsValidInputChar( text[i] ) ) {
						game.Chat.Add( "&eClipboard contained characters that can't be sent on this server." );
						return true;
					}
				}
				AppendText( text );
				return true;
			} else if( key == Key.C && controlDown ) {
				if( !chatInputText.Empty ) {
					Clipboard.SetText( chatInputText.ToString() );
				}
				return true;
			}
			return false;
		}
	}
}