// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Drawing;
using OpenTK.Input;
#if ANDROID
using Android.Graphics;
#endif

namespace ClassicalSharp.Gui.Widgets {
	public abstract class InputWidget : Widget {
		
		public InputWidget( Game game, Font font ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.BottomOrRight;
			buffer = new WrappableStringBuffer( Utils.StringLength * lines );
			
			DrawTextArgs args = new DrawTextArgs( "_", font, true );
			caretTex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			caretTex.Width = (short)((caretTex.Width * 3) / 4);
			defaultCaretWidth = caretTex.Width;
			
			args = new DrawTextArgs( "> ", font, true );
			Size defSize = game.Drawer2D.MeasureChatSize( ref args );
			defaultWidth = Width = defSize.Width;
			defaultHeight = Height = defSize.Height;
			this.font = font;
		}		
		
		internal const int lines = 3;
		internal WrappableStringBuffer buffer;		
		protected int caretPos = -1;
		
		Texture inputTex, caretTex, prefixTex;
		readonly Font font;
		int defaultCaretWidth, defaultWidth, defaultHeight;
		FastColour caretCol;
		static FastColour backColour = new FastColour( 0, 0, 0, 127 );
		
		public override void Render( double delta ) {
			gfx.Texturing = false;
			int y = Y, x = X;
			
			for( int i = 0; i < sizes.Length; i++ ) {
				if( i > 0 && sizes[i].Height == 0 ) break;
				bool caretAtEnd = caretTex.Y1 == y && (indexX == LineLength || caretPos == -1);
				int drawWidth = sizes[i].Width + (caretAtEnd ? caretTex.Width : 0);
				// Cover whole window width to match original classic behaviour
				if( game.PureClassic )
					drawWidth = Math.Max( drawWidth, game.Width - X * 4 );
				
				gfx.Draw2DQuad( x, y, drawWidth + 10, defaultHeight, backColour );
				y += sizes[i].Height;
			}
			gfx.Texturing = true;
			
			inputTex.Render( gfx );
			caretTex.Render( gfx, caretCol );
		}

		internal string[] parts = new string[lines];
		int[] partLens = new int[lines];
		Size[] sizes = new Size[lines];
		int maxWidth = 0;
		int indexX, indexY;
		
		internal int LineLength { get { return game.Server.SupportsPartialMessages ? 64 : 62; } }
		internal int TotalChars { get { return LineLength * lines; } }
		
		public override void Init() {
			X = 5;
			buffer.WordWrap( game.Drawer2D, ref parts, ref partLens, LineLength, TotalChars );
			
			CalculateLineSizes();
			RemakeTexture();
			UpdateCaret();
		}
		
		/// <summary> Calculates the sizes of each line in the text buffer. </summary>
		public void CalculateLineSizes() {
			for( int y = 0; y < sizes.Length; y++ )
				sizes[y] = Size.Empty;
			sizes[0].Width = defaultWidth;
			maxWidth = defaultWidth;
			
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			for( int y = 0; y < lines; y++ ) {
				int offset = y == 0 ? defaultWidth : 0;
				args.Text = parts[y];
				sizes[y] += game.Drawer2D.MeasureChatSize( ref args );
				maxWidth = Math.Max( maxWidth, sizes[y].Width );
			}
			if( sizes[0].Height == 0 ) sizes[0].Height = defaultHeight;
		}
		
		/// <summary> Calculates the location and size of the caret character </summary>
		public void UpdateCaret() {
			if( caretPos >= buffer.Length ) caretPos = -1;
			buffer.MakeCoords( caretPos, partLens, out indexX, out indexY );
			DrawTextArgs args = new DrawTextArgs( null, font, true );

			if( indexX == LineLength ) {
				caretTex.X1 = 10 + sizes[indexY].Width;
				caretCol = FastColour.Yellow;
				caretTex.Width = (short)defaultCaretWidth;
			} else {
				args.Text = parts[indexY].Substring( 0, indexX );
				Size trimmedSize = game.Drawer2D.MeasureChatSize( ref args );
				if( indexY == 0 ) trimmedSize.Width += defaultWidth;
				caretTex.X1 = 10 + trimmedSize.Width;
				caretCol = FastColour.Scale( FastColour.White, 0.8f );
				
				string line = parts[indexY];
				args.Text = indexX < line.Length ? new String( line[indexX], 1 ) : "";
				int caretWidth = indexX < line.Length ?
					game.Drawer2D.MeasureChatSize( ref args ).Width : defaultCaretWidth;
				caretTex.Width = (short)caretWidth;
			}
			caretTex.Y1 = sizes[0].Height * indexY + Y;
			
			// Update the colour of the caret
			IDrawer2D drawer = game.Drawer2D;
			char code = GetLastColour( indexX, indexY );
			if( code != '\0' ) caretCol = drawer.Colours[code];
		}
		
		/// <summary> Remakes the raw texture containg all the chat lines. </summary>
		public void RemakeTexture() {
			int totalHeight = 0;
			for( int i = 0; i < lines; i++ )
				totalHeight += sizes[i].Height;
			Size size = new Size( maxWidth, totalHeight );
			
			int realHeight = 0;
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) )
				using( IDrawer2D drawer = game.Drawer2D )
			{
				drawer.SetBitmap( bmp );
				DrawTextArgs args = new DrawTextArgs( "> ", font, true );
				drawer.DrawChatText( ref args, 0, 0 );
				
				for( int i = 0; i < parts.Length; i++ ) {
					if( parts[i] == null ) break;
					args.Text = parts[i];
					char lastCol = GetLastColour( 0, i );
					if( !IDrawer2D.IsWhiteColour( lastCol ) )
						args.Text = "&" + lastCol + args.Text;
					
					int offset = i == 0 ? defaultWidth : 0;
					drawer.DrawChatText( ref args, offset, realHeight );
					realHeight += sizes[i].Height;
				}
				inputTex = drawer.Make2DTexture( bmp, size, 10, 0 );
			}
			
			Height = realHeight == 0 ? defaultHeight : realHeight;
			Y = game.Height - Height - YOffset;
			inputTex.Y1 = Y;
			Width = size.Width;
		}
		
		protected char GetLastColour( int indexX, int indexY ) {
			int x = indexX;
			IDrawer2D drawer = game.Drawer2D;
			for( int y = indexY; y >= 0; y-- ) {
				string part = parts[y];
				char code = drawer.LastColour( part, x );
				if( code != '\0' ) return code;
				if( y > 0 ) x = parts[y - 1].Length;
			}
			return '\0';
		}

		public override void Dispose() {
			gfx.DeleteTexture( ref inputTex );
		}
		
		public void DisposeFully() {
			Dispose();
			gfx.DeleteTexture( ref caretTex );
			gfx.DeleteTexture( ref prefixTex );
		}

		public override void MoveTo( int newX, int newY ) {
			int dx = newX - X, dy = newY - Y;
			X = newX; Y = newY;
			caretTex.Y1 += dy;
			inputTex.Y1 += dy;
		}
		
		/// <summary> Invoked when the user presses enter. </summary>
		public virtual void EnterInput() {
			Clear();
			Height = defaultHeight;
		}
		
		
		/// <summary> Clears all the characters from the text buffer </summary>
		/// <remarks> Deletes the native texture. </remarks>
		public void Clear() {
			buffer.Clear();
			for( int i = 0; i < parts.Length; i++ )
				parts[i] = null;
			
			caretPos = -1;
			Dispose();
		}

		/// <summary> Appends a sequence of characters to current text buffer. </summary>
		/// <remarks> Potentially recreates the native texture. </remarks>
		public void Append( string text ) {
			int appended = 0;
			foreach( char c in text ) {
				if( AppendChar( c ) ) appended++;
			}
			
			if( appended == 0 ) return;
			Recreate();
		}
		
		/// <summary> Appends a single character to current text buffer. </summary>
		/// <remarks> Potentially recreates the native texture. </remarks>
		public void Append( char c ) {
			if( !AppendChar( c ) ) return;
			Recreate();
		}
		
		bool AppendChar( char c ) {
			if( buffer.Length == TotalChars ) return false;
			
			if( caretPos == -1 ) {
				buffer.InsertAt( buffer.Length, c );
			} else {
				buffer.InsertAt( caretPos, c );
				caretPos++;
				if( caretPos >= buffer.Length ) caretPos = -1;
			}
			return true;
		}
		
		
		protected bool ControlDown() {
			return OpenTK.Configuration.RunningOnMacOS ?
				(game.IsKeyDown( Key.WinLeft ) || game.IsKeyDown( Key.WinRight ))
				: (game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight ));
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( game.HideGui ) return true;
			
			if( Utils.IsValidInputChar( key, game ) ) {
				Append( key );
				return true;
			}
			return false;
		}
		
		public override bool HandlesKeyDown( Key key ) {
			if( game.HideGui ) return key < Key.F1 || key > Key.F35;
			bool clipboardDown = ControlDown();
			
			if( key == Key.Left ) LeftKey( clipboardDown );
			else if( key == Key.Right ) RightKey( clipboardDown );
			else if( key == Key.BackSpace ) BackspaceKey( clipboardDown );
			else if( key == Key.Delete ) DeleteKey();
			else if( key == Key.Home ) HomeKey();
			else if( key == Key.End ) EndKey();
			else if( clipboardDown && !OtherKey( key ) ) return false;
			
			return true;
		}
		
		public override bool HandlesMouseClick( int mouseX, int mouseY, MouseButton button ) {
			if( button == MouseButton.Left )
				SetCaretToCursor( mouseX, mouseY );
			return true;
		}
		
		
		#region Input handling		
		
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
				int index = caretPos == -1 ? buffer.Length - 1 : caretPos;				
				if( CheckColour( index - 1 ) ) {
					DeleteChar(); // backspace XYZ%e to XYZ
					index -= 1;
				} else if( CheckColour( index - 2 ) ) {
					DeleteChar(); DeleteChar(); // backspace XYZ%eH to XYZ
					index -= 2;
				}
				
				if( index > 0 ) DeleteChar();
				Recreate();
			}
		}

		bool CheckColour( int index ) {
			if( index < 0 ) return false;
			char code = buffer.value[index], col = buffer.value[index + 1];
			return (code == '%' || code == '&') && game.Drawer2D.ValidColour( col );
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
				UpdateCaret();
				return;
			}
			
			if( !buffer.Empty ) {
				if( caretPos == -1 ) caretPos = buffer.Length;
				caretPos--;
				if( caretPos < 0 ) caretPos = 0;
				UpdateCaret();
			}
		}
		
		void RightKey( bool controlDown ) {
			if( controlDown ) {
				caretPos += buffer.GetForwardLength( caretPos );
				if( caretPos >= buffer.Length ) caretPos = -1;
				UpdateCaret();
				return;
			}
			
			if( !buffer.Empty && caretPos != -1 ) {
				caretPos++;
				if( caretPos >= buffer.Length ) caretPos = -1;
				UpdateCaret();
			}
		}
		
		void HomeKey() {
			if( buffer.Empty ) return;
			caretPos = 0;
			UpdateCaret();
		}
		
		void EndKey() {
			caretPos = -1;
			UpdateCaret();
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
					if( Utils.IsValidInputChar( text[i], game ) ) continue;
					const string warning = "&eClipboard contained some characters that can't be sent.";
					game.Chat.Add( warning, MessageType.ClientStatus4 );
					text = RemoveInvalidChars( text );
					break;
				}
				Append( text );
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
				if( !Utils.IsValidInputChar( c, game ) ) continue;
				chars[length++] = c;
			}
			return new String( chars, 0, length );
		}
		
		
		unsafe void SetCaretToCursor( int mouseX, int mouseY ) {
			mouseX -= inputTex.X1; mouseY -= inputTex.Y1;
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			IDrawer2D drawer = game.Drawer2D;
			int offset = 0, elemHeight = defaultHeight;
			string oneChar = new String( 'A', 1 );
			
			for( int y = 0; y < parts.Length; y++ ) {
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
					if( GuiElement.Contains( trimmedWidth, y * elemHeight, elemWidth, elemHeight, mouseX, mouseY ) ) {
						caretPos = offset + x;
						UpdateCaret(); return;
					}
				}
				offset += line.Length;
			}
			caretPos = -1;
			UpdateCaret();
		}
		
		#endregion
	}
}