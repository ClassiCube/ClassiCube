// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using System.Windows.Forms;
using ClassicalSharp.Entities;
using OpenTK.Input;

namespace ClassicalSharp.Gui {
	
	public sealed partial class TextInputWidget : Widget {
		
		public override bool HandlesKeyPress( char key ) {
			if( game.HideGui ) return true;
			
			if( IsValidInputChar( key ) && key != '&' ) {
				AppendChar( key );
				return true;
			}
			return false;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( game.HideGui )
				return key < Key.F1 || key > Key.F35;
			bool clipboardDown = OpenTK.Configuration.RunningOnMacOS ?
				(game.IsKeyDown( Key.WinLeft ) || game.IsKeyDown( Key.WinRight ))
				: (game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight ));
			
			if( key == Key.Tab ) TabKey();
			else if( key == Key.Down ) DownKey( clipboardDown );
			else if( key == Key.Up ) UpKey( clipboardDown );
			else if( key == Key.Left ) LeftKey( clipboardDown );
			else if( key == Key.Right ) RightKey( clipboardDown );
			else if( key == Key.BackSpace ) BackspaceKey( clipboardDown );
			else if( key == Key.Delete ) DeleteKey();
			else if( key == Key.Home ) HomeKey();
			else if( key == Key.End ) EndKey();
			else if( game.Network.ServerSupportsFullCP437 &&
			        key == game.InputHandler.Keys[KeyBind.ExtInput] )
				altText.SetActive( !altText.Active );
			else if( clipboardDown && !OtherKey( key ) ) return false;
			
			return true;
		}
		
		void TabKey() {
			int pos = caretPos == -1 ? buffer.Length - 1 : caretPos;
			int start = pos;
			char[] value = buffer.value;
			
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
				if( caretPos == -1 ) pos++;
				int len = pos - start;
				for( int i = 0; i < len; i++ )
					buffer.DeleteAt( start );
				if( caretPos != -1 ) caretPos -= len;
				AppendText( matches[0] );
			} else if( matches.Count > 1 ) {
				StringBuffer sb = new StringBuffer( 64 );
				int index = 0;
				sb.Append( ref index, "&e" );
				sb.AppendNum( ref index, matches.Count );
				sb.Append( ref index, " matching names: " );
				
				foreach( string match in matches ) {
					if( (match.Length + 1 + sb.Length) > LineLength ) break;
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
				if( caretPos == -1 ) caretPos = buffer.Length - 1;
				int len = buffer.GetBackLength( caretPos );
				if( len == 0 ) return;
				
				caretPos -= len;
				if( caretPos < 0 ) caretPos = 0;
				for( int i = 0; i <= len; i++ )
					buffer.DeleteAt( caretPos );
				
				if( caretPos >= buffer.Length ) caretPos = -1;
				if( caretPos == -1 &&  buffer.Length > 0 ) {
				    buffer.value[buffer.Length] = ' ';
				} else if( caretPos >= 0 && buffer.value[caretPos] != ' ' ) {
					buffer.InsertAt( caretPos, ' ' );
				}
				Recreate();
			} else if( !buffer.Empty && caretPos != 0 ) {
				if( !BackspaceColourCode())
					DeleteChar();
				Recreate();
			}
		}
		
		bool BackspaceColourCode() {
			// If text is XYZ%eH, backspaces to XYZ.
			int index = caretPos == -1 ? buffer.Length - 1 : caretPos;
			if( index <= 1 ) return false;
			
			if( buffer.value[index - 1] != '%' || !game.Drawer2D.ValidColour( buffer.value[index] ) )
				return false;
			DeleteChar(); DeleteChar();
			return true;
		}
		
		void DeleteChar() {
			if( caretPos == -1 ) {
				buffer.DeleteAt( buffer.Length - 1 );
			} else {
				caretPos--;
				buffer.DeleteAt( caretPos );
			}
		}
		
		void DeleteKey() {
			if( !buffer.Empty && caretPos != -1 ) {
				buffer.DeleteAt( caretPos );
				if( caretPos >= buffer.Length ) caretPos = -1;
				Recreate();
			}
		}
		
		void LeftKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 )
					caretPos = buffer.Length - 1;
				caretPos -= buffer.GetBackLength( caretPos );
				CalculateCaretData();
				return;
			}
			
			if( !buffer.Empty ) {
				if( caretPos == -1 ) caretPos = buffer.Length;
				caretPos--;
				if( caretPos < 0 ) caretPos = 0;
				CalculateCaretData();
			}
		}
		
		void RightKey( bool controlDown ) {
			if( controlDown ) {
				caretPos += buffer.GetForwardLength( caretPos );
				if( caretPos >= buffer.Length ) caretPos = -1;
				CalculateCaretData();
				return;
			}
			
			if( !buffer.Empty && caretPos != -1 ) {
				caretPos++;
				if( caretPos >= buffer.Length ) caretPos = -1;
				CalculateCaretData();
			}
		}
		
		string originalText;
		void UpKey( bool controlDown ) {
			if( controlDown ) {
				int pos = caretPos == -1 ? buffer.Length : caretPos;
				if( pos < LineLength ) return;
				
				caretPos = pos - LineLength;
				CalculateCaretData();
				return;
			}
			
			if( typingLogPos == game.Chat.InputLog.Count )
				originalText = buffer.ToString();
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				buffer.Clear();
				buffer.Append( 0, game.Chat.InputLog[typingLogPos] );
				caretPos = -1;
				Recreate();
			}
		}
		
		void DownKey( bool controlDown ) {
			if( controlDown ) {
				if( caretPos == -1 || caretPos >= (lines - 1) * LineLength ) return;
				caretPos += LineLength;
				CalculateCaretData();
				return;
			}
			
			if( game.Chat.InputLog.Count > 0 ) {
				typingLogPos++;
				buffer.Clear();
				if( typingLogPos >= game.Chat.InputLog.Count ) {
					typingLogPos = game.Chat.InputLog.Count;
					if( originalText != null )
						buffer.Append( 0, originalText );
				} else {
					buffer.Append( 0, game.Chat.InputLog[typingLogPos] );
				}
				caretPos = -1;
				Recreate();
			}
		}
		
		void HomeKey() {
			if( buffer.Empty ) return;
			caretPos = 0;
			CalculateCaretData();
		}
		
		void EndKey() {
			caretPos = -1;
			CalculateCaretData();
		}
		
		static char[] trimChars = {'\r', '\n', '\v', '\f', ' ', '\t', '\0'};
		bool OtherKey( Key key ) {
			if( key == Key.V && buffer.Length < TotalChars ) {
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
					if( IsValidInputChar( text[i] ) ) continue;
					const string warning = "&eClipboard contained some characters that can't be sent.";
					game.Chat.Add( warning, MessageType.ClientStatus4 );
					text = RemoveInvalidChars( text );
					break;
				}
				AppendText( text );
				return true;
			} else if( key == Key.C ) {
				if( buffer.Empty ) return true;
				try {
					game.window.ClipboardText = buffer.ToString();
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
				if( !IsValidInputChar( c ) ) continue;
				chars[length++] = c;
			}
			return new String( chars, 0, length );
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( altText.Active && altText.Bounds.Contains( mouseX, mouseY ) ) {
				altText.HandlesMouseClick( mouseX, mouseY, button );
				UpdateAltTextY();
			} else if( button == MouseButton.Left ) {
				SetCaretToCursor( mouseX, mouseY );
			}
			return true;
		}
		
		void UpdateAltTextY() {
			int blockSize = (int)(23 * 2 * game.GuiHotbarScale);
			int height = Math.Max( Height + YOffset, blockSize ) + YOffset;
			altText.texture.Y1 = game.Height - (height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
		}
		
		unsafe void SetCaretToCursor( int mouseX, int mouseY ) {
			mouseX -= inputTex.X1; mouseY -= inputTex.Y1;
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			IDrawer2D drawer = game.Drawer2D;
			int offset = 0, elemHeight = defaultHeight;
			string oneChar = new String( 'A', 1 );
			
			for( int y = 0; y < lines; y++ ) {
				string line = parts[y];
				int xOffset = y == 0 ? defaultWidth : 0;
				if( line == null ) continue;
				
				for( int x = 0; x < line.Length; x++ ) {
					args.Text = line.Substring( 0, x );
					int trimmedWidth = drawer.MeasureChatSize( ref args ).Width + xOffset;
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
				offset += line.Length;
			}
			caretPos = -1;
			CalculateCaretData();
		}
	}
}