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
			bool controlDown = game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight );
			
			if( key == Key.Down ) DownKey( controlDown );
			else if( key == Key.Up ) UpKey( controlDown );
			else if( key == Key.Left ) LeftKey( controlDown );
			else if( key == Key.Right ) RightKey( controlDown );
			else if( key == Key.BackSpace ) BackspaceKey( controlDown );
			else if( key == Key.Delete ) DeleteKey();
			else if( key == Key.Home ) HomeKey();
			else if( key == Key.End ) EndKey();
			else if( game.Network.ServerSupportsFullCP437 &&
			        key == game.InputHandler.Keys[KeyBinding.ExtendedInput] )
				altText.SetActive( !altText.Active );
			else if( controlDown && !OtherKey( key ) ) return false;
			
			return true;
		}
		
		void BackspaceKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 ) 
					caretPos = chatInputText.Length - 1;
				int len = chatInputText.GetBackLength( caretPos );
				caretPos -= len;
				
				if( caretPos < 0 ) caretPos = 0;
				for( int i = 0; i <= len; i++ )
					chatInputText.DeleteAt( caretPos );
				
				Dispose();
				Init();
				return;
			}
			
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
		
		void LeftKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 ) 
					caretPos = chatInputText.Length - 1;
				caretPos -= chatInputText.GetBackLength( caretPos );
				CalculateCaretData();
				return;
			}
			
			if( !chatInputText.Empty ) {
				if( caretPos == -1 ) caretPos = chatInputText.Length;
				caretPos--;
				if( caretPos < 0 ) caretPos = 0;
				CalculateCaretData();
			}
		}
		
		void RightKey( bool controlDown ) {
			if( controlDown ) {
				caretPos += chatInputText.GetForwardLength( caretPos );			
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				CalculateCaretData();
				return;
			}
			
			if( !chatInputText.Empty && caretPos != -1 ) {
				caretPos++;
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				CalculateCaretData();
			}
		}
		
		string originalText;
		void UpKey( bool controlDown ) {
			if( controlDown ) {
				int pos = caretPos == -1 ? chatInputText.Length : caretPos;
				if( pos < 64 ) return;
				
				caretPos = pos - 64;
				CalculateCaretData();
				return;
			}
			
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
		
		void DownKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 || caretPos >= (lines - 1) * 64 ) return;
				caretPos += 64;
				CalculateCaretData();
				return;
			}
			
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
			CalculateCaretData();
		}
		
		void EndKey() {
			caretPos = -1;
			CalculateCaretData();
		}
		
		bool OtherKey( Key key ) {
			if( key == Key.V && chatInputText.Length < len ) {
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
			} else if( key == Key.C ) {
				if( !chatInputText.Empty ) {
					Clipboard.SetText( chatInputText.ToString() );
				}
				return true;
			}
			return false;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( altText.Active && altText.Bounds.Contains( mouseX, mouseY ) ) {
				altText.HandlesMouseClick( mouseX, mouseY, button );
				altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
				altText.Y = altText.texture.Y1;
			} else if( button == MouseButton.Left ) {
				SetCaretToCursor( mouseX, mouseY );
			}
			return true;
		}
		
		unsafe void SetCaretToCursor( int mouseX, int mouseY ) {
			mouseX -= inputTex.X1; mouseY -= inputTex.Y1;
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			IDrawer2D drawer = game.Drawer2D;
			int offset = 0, elemHeight = defaultHeight;
			string oneChar = new String( 'A', 1 );
			
			for( int y = 0; y < lines; y++ ) {
				string line = parts[y];
				if( line == null ) continue;
				
				for( int x = 0; x < line.Length; x++ ) {
					args.Text = line.Substring( 0, x );
					int trimmedWidth = drawer.MeasureChatSize( ref args ).Width;
					// avoid allocating an unnecessary string
					fixed( char* ptr = oneChar )
						ptr[0] = line[x];
					
					args.Text = oneChar;
					int elemWidth = drawer.MeasureChatSize( ref args ).Width;
					if( Contains( trimmedWidth, y * elemHeight, elemWidth, elemHeight, mouseX, mouseY ) ) {
						caretPos = offset + x;
						CalculateCaretData(); return;
					}
				}
				offset += partLens[y];
			}
			caretPos = -1;
			CalculateCaretData();
		}
	}
}