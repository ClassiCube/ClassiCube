// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Windows.Forms;
using ClassicalSharp.Entities;
using OpenTK.Input;

namespace ClassicalSharp.Gui.Widgets {
	public sealed class InputWidgetHandler {
		
		Game game;
		InputWidget w;
		
		public InputWidgetHandler( Game game, InputWidget w ) {
			this.game = game;
			this.w = w;
		}
		
		public bool ControlDown() {
			return OpenTK.Configuration.RunningOnMacOS ?
				(game.IsKeyDown( Key.WinLeft ) || game.IsKeyDown( Key.WinRight ))
				: (game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight ));
		}
		
		public bool HandlesKeyDown( Key key ) {
			bool clipboardDown = ControlDown();
			
			if( key == Key.Tab ) TabKey();
			else if( key == Key.Left ) LeftKey( clipboardDown );
			else if( key == Key.Right ) RightKey( clipboardDown );
			else if( key == Key.BackSpace ) BackspaceKey( clipboardDown );
			else if( key == Key.Delete ) DeleteKey();
			else if( key == Key.Home ) HomeKey();
			else if( key == Key.End ) EndKey();
			else if( clipboardDown && !OtherKey( key ) ) return false;
			
			return true;
		}
		
		void TabKey() {
			int pos = w.caretPos == -1 ? w.buffer.Length - 1 : w.caretPos;
			int start = pos;
			char[] value = w.buffer.value;
			
			while( start >= 0 && IsNameChar( value[start] ) )
				start--;
			start++;
			if( pos < 0 || start > pos ) return;
			
			string part = new String( value, start, pos + 1 - start );
			List<string> matches = new List<string>();
			game.Chat.Add( null, MessageType.ClientStatus5 );
			
			TabListEntry[] entries = game.TabList.Entries;
			for( int i = 0; i < EntityList.MaxCount; i++ ) {
				if( entries[i] == null ) continue;
				
				string rawName = entries[i].PlayerName;
				string name = Utils.StripColours( rawName );
				if( name.StartsWith( part, StringComparison.OrdinalIgnoreCase ) )
					matches.Add( name );
			}
			
			if( matches.Count == 1 ) {
				if( w.caretPos == -1 ) pos++;
				int len = pos - start;
				for( int i = 0; i < len; i++ )
					w.buffer.DeleteAt( start );
				if( w.caretPos != -1 ) w.caretPos -= len;
				w.AppendText( matches[0] );
			} else if( matches.Count > 1 ) {
				StringBuffer sb = new StringBuffer( Utils.StringLength );
				int index = 0;
				sb.Append( ref index, "&e" );
				sb.AppendNum( ref index, matches.Count );
				sb.Append( ref index, " matching names: " );
				
				for( int i = 0; i < matches.Count; i++) {
					string match = matches[i];
					if( (sb.Length + match.Length + 1) > sb.Capacity ) break;
					sb.Append( ref index, match );
					sb.Append( ref index, ' ' );
				}
				game.Chat.Add( sb.ToString(), MessageType.ClientStatus5 );
			}
		}
		
		bool IsNameChar( char c ) {
			return c == '_' || c == '.' || (c >= '0' && c <= '9')
				|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
		}
		
		void BackspaceKey( bool controlDown ) {
			if( controlDown ) {
				if( w.caretPos == -1 ) w.caretPos = w.buffer.Length - 1;
				int len = w.buffer.GetBackLength( w.caretPos );
				if( len == 0 ) return;
				
				w.caretPos -= len;
				if( w.caretPos < 0 ) w.caretPos = 0;
				for( int i = 0; i <= len; i++ )
					w.buffer.DeleteAt( w.caretPos );
				
				if( w.caretPos >= w.buffer.Length ) w.caretPos = -1;
				if( w.caretPos == -1 &&  w.buffer.Length > 0 ) {
					w.buffer.value[w.buffer.Length] = ' ';
				} else if( w.caretPos >= 0 && w.buffer.value[w.caretPos] != ' ' ) {
					w.buffer.InsertAt( w.caretPos, ' ' );
				}
				w.Recreate();
			} else if( !w.buffer.Empty && w.caretPos != 0 ) {
				int index = w.caretPos == -1 ? w.buffer.Length - 1 : w.caretPos;
				if( CheckColour( index - 1 ) ) {
					DeleteChar(); // backspace XYZ%e to XYZ
				} else if( CheckColour( index - 2 ) ) {
					DeleteChar(); DeleteChar(); // backspace XYZ%eH to XYZ
				}
				DeleteChar();
				w.Recreate();
			}
		}

		bool CheckColour( int index ) {
			if( index < 0 ) return false;
			char code = w.buffer.value[index], col = w.buffer.value[index + 1];
			return (code == '%' || code == '&') && game.Drawer2D.ValidColour( col );
		}
		
		void DeleteChar() {
			if( w.caretPos == -1 ) {
				w.buffer.DeleteAt( w.buffer.Length - 1 );
			} else {
				w.caretPos--;
				w.buffer.DeleteAt( w.caretPos );
			}
		}
		
		void DeleteKey() {
			if( !w.buffer.Empty && w.caretPos != -1 ) {
				w.buffer.DeleteAt( w.caretPos );
				if( w.caretPos >= w.buffer.Length ) w.caretPos = -1;
				w.Recreate();
			}
		}
		
		void LeftKey( bool controlDown ) {
			if( controlDown ) {
				if( w.caretPos == -1 )
					w.caretPos = w.buffer.Length - 1;
				w.caretPos -= w.buffer.GetBackLength( w.caretPos );
				w.CalculateCaretData();
				return;
			}
			
			if( !w.buffer.Empty ) {
				if( w.caretPos == -1 ) w.caretPos = w.buffer.Length;
				w.caretPos--;
				if( w.caretPos < 0 ) w.caretPos = 0;
				w.CalculateCaretData();
			}
		}
		
		void RightKey( bool controlDown ) {
			if( controlDown ) {
				w.caretPos += w.buffer.GetForwardLength( w.caretPos );
				if( w.caretPos >= w.buffer.Length ) w.caretPos = -1;
				w.CalculateCaretData();
				return;
			}
			
			if( !w.buffer.Empty && w.caretPos != -1 ) {
				w.caretPos++;
				if( w.caretPos >= w.buffer.Length ) w.caretPos = -1;
				w.CalculateCaretData();
			}
		}
		
		void HomeKey() {
			if( w.buffer.Empty ) return;
			w.caretPos = 0;
			w.CalculateCaretData();
		}
		
		void EndKey() {
			w.caretPos = -1;
			w.CalculateCaretData();
		}
		
		static char[] trimChars = {'\r', '\n', '\v', '\f', ' ', '\t', '\0'};
		bool OtherKey( Key key ) {
			if( key == Key.V && w.buffer.Length < w.TotalChars ) {
				string text = null;
				try {
					text = game.window.ClipboardText.Trim( trimChars );
				} catch( Exception ex ) {
					ErrorHandler.LogError( "Paste from clipboard", ex );
					const string warning = "&cError while trying to paste from clipboard.";
					game.Chat.Add( warning, MessageType.ClientStatus4 );
					return true;
				}

				if( String.IsNullOrEmpty( text ) ) return true;
				game.Chat.Add( null, MessageType.ClientStatus4 );
				
				for( int i = 0; i < text.Length; i++ ) {
					if( Utils.IsValidInputChar( text[i], game ) ) continue;
					const string warning = "&eClipboard contained some characters that can't be sent.";
					game.Chat.Add( warning, MessageType.ClientStatus4 );
					text = RemoveInvalidChars( text );
					break;
				}
				w.AppendText( text );
				return true;
			} else if( key == Key.C ) {
				if( w.buffer.Empty ) return true;
				try {
					game.window.ClipboardText = w.buffer.ToString();
				} catch( Exception ex ) {
					ErrorHandler.LogError( "Copy to clipboard", ex );
					const string warning = "&cError while trying to copy to clipboard.";
					game.Chat.Add( warning, MessageType.ClientStatus4 );
				}
				return true;
			}
			return false;
		}
		
		string RemoveInvalidChars( string input ) {
			char[] chars = new char[input.Length];
			int length = 0;
			for( int i = 0; i < input.Length; i++ ) {
				char c = input[i];
				if( !Utils.IsValidInputChar( c, game ) ) continue;
				chars[length++] = c;
			}
			return new String( chars, 0, length );
		}
		
		public bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button == MouseButton.Left )
				SetCaretToCursor( mouseX, mouseY );
			return true;
		}
		
		unsafe void SetCaretToCursor( int mouseX, int mouseY ) {
			mouseX -= w.inputTex.X1; mouseY -= w.inputTex.Y1;
			DrawTextArgs args = new DrawTextArgs( null, w.font, true );
			IDrawer2D drawer = game.Drawer2D;
			int offset = 0, elemHeight = w.defaultHeight;
			string oneChar = new String( 'A', 1 );
			
			for( int y = 0; y < w.parts.Length; y++ ) {
				string line = w.parts[y];
				int xOffset = y == 0 ? w.defaultWidth : 0;
				if( line == null ) continue;
				
				for( int x = 0; x < line.Length; x++ ) {
					args.Text = line.Substring( 0, x );
					int trimmedWidth = drawer.MeasureChatSize( ref args ).Width + xOffset;
					// avoid allocating an unnecessary string
					fixed( char* ptr = oneChar )
						ptr[0] = line[x];
					
					args.Text = oneChar;
					int elemWidth = drawer.MeasureChatSize( ref args ).Width;
					if( GuiElement.Contains( trimmedWidth, y * elemHeight, elemWidth, elemHeight, mouseX, mouseY ) ) {
						w.caretPos = offset + x;
						w.CalculateCaretData(); return;
					}
				}
				offset += line.Length;
			}
			w.caretPos = -1;
			w.CalculateCaretData();
		}
	}
}