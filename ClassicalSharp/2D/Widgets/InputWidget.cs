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
			
			buffer = new WrappableStringBuffer( Utils.StringLength * MaxLines );
			lines = new string[MaxLines];
			lineSizes = new Size[MaxLines];
			
			DrawTextArgs args = new DrawTextArgs( "_", font, true );
			caretTex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			caretTex.Width = (short)((caretTex.Width * 3) / 4);
			caretWidth = caretTex.Width;
			
			args = new DrawTextArgs( "> ", font, true );
			Size size = game.Drawer2D.MeasureChatSize( ref args );
			prefixWidth = Width = size.Width;
			prefixHeight = Height = size.Height;
			this.font = font;
		}
		
		internal WrappableStringBuffer buffer;
		protected int caret = -1;
		
		Texture inputTex, caretTex, prefixTex;
		readonly Font font;
		int caretWidth, prefixWidth, prefixHeight;
		FastColour caretColour;
		static FastColour backColour = new FastColour( 0, 0, 0, 127 );
		
		public override void Render( double delta ) {
			gfx.Texturing = false;
			int y = Y, x = X;
			
			for( int i = 0; i < lineSizes.Length; i++ ) {
				if( i > 0 && lineSizes[i].Height == 0 ) break;
				bool caretAtEnd = caretTex.Y1 == y && (caretCol == MaxCharsPerLine || caret == -1);
				int drawWidth = lineSizes[i].Width + (caretAtEnd ? caretTex.Width : 0);
				// Cover whole window width to match original classic behaviour
				if( game.PureClassic )
					drawWidth = Math.Max( drawWidth, game.Width - X * 4 );
				
				gfx.Draw2DQuad( x, y, drawWidth + 10, prefixHeight, backColour );
				y += lineSizes[i].Height;
			}
			gfx.Texturing = true;
			
			inputTex.Render( gfx );
			caretTex.Render( gfx, caretColour );
		}
		
		/// <summary> The maximum number of lines that may be entered. </summary>
		public abstract int MaxLines { get; }
		
		/// <summary> The maximum number of characters that can fit on one line. </summary>
		public abstract int MaxCharsPerLine { get; }

		protected string[] lines; // raw text of each line
		protected Size[] lineSizes; // size of each line in pixels
		int caretCol, caretRow;	 // coordinates of caret
		int maxWidth = 0; // maximum width of any line
		
		public override void Init() {
			X = 5;
			buffer.WordWrap( game.Drawer2D, lines, MaxCharsPerLine );
			
			CalculateLineSizes();
			RemakeTexture();
			UpdateCaret();
		}
		
		/// <summary> Calculates the sizes of each line in the text buffer. </summary>
		public void CalculateLineSizes() {
			for( int y = 0; y < lineSizes.Length; y++ )
				lineSizes[y] = Size.Empty;
			lineSizes[0].Width = prefixWidth;
			maxWidth = prefixWidth;
			
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			for( int y = 0; y < MaxLines; y++ ) {
				args.Text = lines[y];
				lineSizes[y] += game.Drawer2D.MeasureChatSize( ref args );
				maxWidth = Math.Max( maxWidth, lineSizes[y].Width );
			}
			if( lineSizes[0].Height == 0 ) lineSizes[0].Height = prefixHeight;
		}
		
		/// <summary> Calculates the location and size of the caret character </summary>
		public void UpdateCaret() {
			if( caret >= buffer.Length ) caret = -1;
			buffer.GetCoords( caret, lines, out caretCol, out caretRow );
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			IDrawer2D drawer = game.Drawer2D;

			if( caretCol == MaxCharsPerLine ) {
				caretTex.X1 = 10 + lineSizes[caretRow].Width;
				caretColour = FastColour.Yellow;
				caretTex.Width = (short)caretWidth;
			} else {
				args.Text = lines[caretRow].Substring( 0, caretCol );
				Size trimmedSize = drawer.MeasureChatSize( ref args );
				if( caretRow == 0 ) trimmedSize.Width += prefixWidth;
				
				caretTex.X1 = 10 + trimmedSize.Width;
				caretColour = FastColour.Scale( FastColour.White, 0.8f );
				
				string line = lines[caretRow];
				if( caretCol < line.Length ) {
					args.Text = new String( line[caretCol], 1 );
					caretTex.Width = (short)drawer.MeasureChatSize( ref args ).Width;
				} else {
					caretTex.Width = (short)caretWidth;
				}
			}
			caretTex.Y1 = lineSizes[0].Height * caretRow + Y;
			
			// Update the colour of the caret			
			char code = GetLastColour( caretCol, caretRow );
			if( code != '\0' ) caretColour = drawer.Colours[code];
		}
		
		/// <summary> Remakes the raw texture containg all the chat lines. </summary>
		public void RemakeTexture() {
			int totalHeight = 0;
			for( int i = 0; i < MaxLines; i++ )
				totalHeight += lineSizes[i].Height;
			Size size = new Size( maxWidth, totalHeight );
			
			int realHeight = 0;
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) )
				using( IDrawer2D drawer = game.Drawer2D )
			{
				drawer.SetBitmap( bmp );
				DrawTextArgs args = new DrawTextArgs( "> ", font, true );
				drawer.DrawChatText( ref args, 0, 0 );
				
				for( int i = 0; i < lines.Length; i++ ) {
					if( lines[i] == null ) break;
					args.Text = lines[i];
					char lastCol = GetLastColour( 0, i );
					if( !IDrawer2D.IsWhiteColour( lastCol ) )
						args.Text = "&" + lastCol + args.Text;
					
					int offset = i == 0 ? prefixWidth : 0;
					drawer.DrawChatText( ref args, offset, realHeight );
					realHeight += lineSizes[i].Height;
				}
				inputTex = drawer.Make2DTexture( bmp, size, 10, 0 );
			}
			
			Height = realHeight == 0 ? prefixHeight : realHeight;
			Y = game.Height - Height - YOffset;
			inputTex.Y1 = Y;
			Width = size.Width;
		}
		
		protected char GetLastColour( int indexX, int indexY ) {
			int x = indexX;
			IDrawer2D drawer = game.Drawer2D;
			for( int y = indexY; y >= 0; y-- ) {
				string part = lines[y];
				char code = drawer.LastColour( part, x );
				if( code != '\0' ) return code;
				if( y > 0 ) x = lines[y - 1].Length;
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

		public override void CalculatePosition() {
			int oldX = X, oldY = Y;
			base.CalculatePosition();
			
			caretTex.X1 += X - oldX;
			caretTex.Y1 += Y - oldY;
			inputTex.X1 += X - oldX;
			inputTex.Y1 += Y - oldY;
		}
		
		/// <summary> Invoked when the user presses enter. </summary>
		public virtual void EnterInput() {
			Clear();
			Height = prefixHeight;
		}
		
		
		/// <summary> Clears all the characters from the text buffer </summary>
		/// <remarks> Deletes the native texture. </remarks>
		public void Clear() {
			buffer.Clear();
			for( int i = 0; i < lines.Length; i++ )
				lines[i] = null;
			
			caret = -1;
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
			int totalChars = MaxCharsPerLine * lines.Length;
			if( buffer.Length == totalChars ) return false;
			
			if( caret == -1 ) {
				buffer.InsertAt( buffer.Length, c );
			} else {
				buffer.InsertAt( caret, c );
				caret++;
				if( caret >= buffer.Length ) caret = -1;
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
				if( caret == -1 ) caret = buffer.Length - 1;
				int len = buffer.GetBackLength( caret );
				if( len == 0 ) return;
				
				caret -= len;
				if( caret < 0 ) caret = 0;
				for( int i = 0; i <= len; i++ )
					buffer.DeleteAt( caret );
				
				if( caret >= buffer.Length ) caret = -1;
				if( caret == -1 &&  buffer.Length > 0 ) {
					buffer.value[buffer.Length] = ' ';
				} else if( caret >= 0 && buffer.value[caret] != ' ' ) {
					buffer.InsertAt( caret, ' ' );
				}
				Recreate();
			} else if( !buffer.Empty && caret != 0 ) {
				int index = caret == -1 ? buffer.Length - 1 : caret;
				if( CheckColour( index - 1 ) ) {
					DeleteChar(); // backspace XYZ%e to XYZ
				} else if( CheckColour( index - 2 ) ) {
					DeleteChar(); DeleteChar(); // backspace XYZ%eH to XYZ
				}
				
				DeleteChar();
				Recreate();
			}
		}

		bool CheckColour( int index ) {
			if( index < 0 ) return false;
			char code = buffer.value[index], col = buffer.value[index + 1];
			return (code == '%' || code == '&') && game.Drawer2D.ValidColour( col );
		}
		
		void DeleteChar() {
			if( buffer.Length == 0 ) return;
			
			if( caret == -1 ) {
				buffer.DeleteAt( buffer.Length - 1 );
			} else {
				caret--;
				buffer.DeleteAt( caret );
			}
		}
		
		void DeleteKey() {
			if( !buffer.Empty && caret != -1 ) {
				buffer.DeleteAt( caret );
				if( caret >= buffer.Length ) caret = -1;
				Recreate();
			}
		}
		
		void LeftKey( bool controlDown ) {
			if( controlDown ) {
				if( caret == -1 )
					caret = buffer.Length - 1;
				caret -= buffer.GetBackLength( caret );
				UpdateCaret();
				return;
			}
			
			if( !buffer.Empty ) {
				if( caret == -1 ) caret = buffer.Length;
				caret--;
				if( caret < 0 ) caret = 0;
				UpdateCaret();
			}
		}
		
		void RightKey( bool controlDown ) {
			if( controlDown ) {
				caret += buffer.GetForwardLength( caret );
				if( caret >= buffer.Length ) caret = -1;
				UpdateCaret();
				return;
			}
			
			if( !buffer.Empty && caret != -1 ) {
				caret++;
				if( caret >= buffer.Length ) caret = -1;
				UpdateCaret();
			}
		}
		
		void HomeKey() {
			if( buffer.Empty ) return;
			caret = 0;
			UpdateCaret();
		}
		
		void EndKey() {
			caret = -1;
			UpdateCaret();
		}
		
		static char[] trimChars = {'\r', '\n', '\v', '\f', ' ', '\t', '\0'};
		bool OtherKey( Key key ) {
			int totalChars = MaxCharsPerLine * lines.Length;
			if( key == Key.V && buffer.Length < totalChars ) {
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
			int offset = 0, elemHeight = prefixHeight;
			string oneChar = new String( 'A', 1 );
			
			for( int y = 0; y < lines.Length; y++ ) {
				string line = lines[y];
				int xOffset = y == 0 ? prefixWidth : 0;
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
						caret = offset + x;
						UpdateCaret(); return;
					}
				}
				offset += line.Length;
			}
			caret = -1;
			UpdateCaret();
		}
		
		#endregion
	}
}