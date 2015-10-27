using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed class TextInputWidget : Widget {
		
		const int len = 64 * 3;
		const int lines = 3;
		public TextInputWidget( Game game, Font font, Font boldFont ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.BottomOrRight;
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			this.font = font;
			this.boldFont = boldFont;
			chatInputText = new StringBuffer( len );
		}
		
		Texture chatInputTexture, caretTexture;
		FastColour backColour = new FastColour( 60, 60, 60, 200 );
		int caretPos = -1;
		int typingLogPos = 0;
		public int YOffset;
		internal StringBuffer chatInputText;
		readonly Font font, boldFont;
		
		static FastColour normalCaretCol = FastColour.White,
		nextCaretCol = FastColour.Yellow;
		FastColour caretCol;
		public override void Render( double delta ) {
			chatInputTexture.Render( graphicsApi );
			caretTexture.Render( graphicsApi, caretCol );
		}

		string[] parts = new string[lines];
		int[] partLens = new int[lines];
		Size[] sizes = new Size[lines];
		int maxWidth = 0;
		
		public override void Init() {
			X = 10;
			DrawTextArgs args = new DrawTextArgs( "_", boldFont, false );
			caretTexture = game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
			chatInputText.WordWrap( ref parts, ref partLens, 64 );
			
			maxWidth = 0;
			args = new DrawTextArgs( null, font, false );
			for( int i = 0; i < lines; i++ ) {
				args.Text = parts[i];
				sizes[i] = game.Drawer2D.MeasureSize( ref args );
				maxWidth = Math.Max( maxWidth, sizes[i].Width );
			}
			
			int realIndex = caretPos;
			if( chatInputText.Empty || caretPos == -1 || caretPos >= chatInputText.Length ) {
				caretPos = -1; realIndex = 500000;
			}
			
			int sum = 0, indexX = -1, indexY = 0;			
			for( int i = 0; i < lines; i++ ) {
				if( partLens[i] == 0 ) break;
				
				indexY = i;
				if( realIndex < sum + partLens[i] ) {
					indexX = realIndex - sum;
					break;
				}
				sum += partLens[i];
			}		
			if( indexX == -1 ) indexX = partLens[indexY];

			if( indexX == 64 ) {
				caretTexture.X1 = 10 + sizes[indexY].Width;
				sizes[indexY].Width += caretTexture.Width;
				
				maxWidth = Math.Max( maxWidth, sizes[indexY].Width );
				caretTexture.Y1 = sizes[0].Height * indexY;
				caretCol = nextCaretCol;
			} else {
				args.Text = parts[indexY].Substring( 0, indexX );
				Size trimmedSize = game.Drawer2D.MeasureSize( ref args );
				caretTexture.X1 = 10 + trimmedSize.Width;
				
				args.Text = new String( parts[indexY][indexX], 1 );
				Size charSize = game.Drawer2D.MeasureSize( ref args );
				caretTexture.Width = charSize.Width;
				
				caretTexture.Y1 = sizes[0].Height * indexY;
				caretCol = normalCaretCol;
			}
			DrawString();
		}
		
		void DrawString() {
			int totalHeight = 0;
			for( int i = 0; i < lines; i++ ) {
				totalHeight += sizes[i].Height;
			}
			Size size = new Size( maxWidth, totalHeight );
			int y = game.Height - size.Height - YOffset;
			
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					DrawTextArgs args = new DrawTextArgs( null, font, false );
					
					int yyy = 0;
					for( int i = 0; i < parts.Length; i++ ) {
						if( parts[i] == null ) break;
						args.Text = parts[i];
						
						drawer.Clear( backColour, 0, yyy, sizes[i].Width, sizes[i].Height );
						drawer.DrawText( ref args, 0, yyy );
						yyy += sizes[i].Height;
					}
					chatInputTexture = drawer.Make2DTexture( bmp, size, 10, y );
				}
			}
			
			caretTexture.Y1 = y + caretTexture.Y1;
			Y = y;
			Width = size.Width;
			Height = size.Height;
		}

		public override void Dispose() {
			graphicsApi.DeleteTexture( ref caretTexture );
			graphicsApi.DeleteTexture( ref chatInputTexture );
		}

		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			X = newX;
			Y = newY;
			caretTexture.Y1 += deltaY;
			chatInputTexture.Y1 += deltaY;
		}
		
		static bool IsInvalidChar( char c ) {
			// Make sure we're in the printable text range from 0x20 to 0x7E
			return c < ' ' || c == '&' || c > '~';
		}
		
		static char[] trimChars = { ' ' };
		public void SendTextInBufferAndReset() {
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i] == null ) break;
				game.Chat.Send( parts[i].TrimEnd( trimChars ) );
			}
			
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			chatInputText.Clear();
			caretPos = -1;
			Dispose();
			Height = 0;
		}
		
		public void Clear() {
			chatInputText.Clear();
			for( int i = 0; i < parts.Length; i++ ) {
				parts[i] = null;
			}
		}
		
		#region Input handling
		
		public override bool HandlesKeyPress( char key ) {
			if( chatInputText.Length < len && !IsInvalidChar( key ) ) {
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
			if( key == Key.Down ) DownKey();
			else if( key == Key.Up ) UpKey();
			else if( key == Key.Left ) LeftKey();
			else if( key == Key.Right ) RightKey();
			else if( key == Key.BackSpace ) BackspaceKey();
			else if( key == Key.Delete ) DeleteKey();
			else if( !OtherKey( key ) ) return false;
			
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
		
		void UpKey() {
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
				} else {
					chatInputText.Append( 0, game.Chat.InputLog[typingLogPos] );
				}
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		bool OtherKey( Key key ) {
			bool controlDown = game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight );
			if( key == Key.V && controlDown && chatInputText.Length < len ) {
				string text = Clipboard.GetText();
				if( String.IsNullOrEmpty( text ) ) return true;
				
				for( int i = 0; i < text.Length; i++ ) {
					if( IsInvalidChar( text[i] ) ) {
						game.Chat.Add( "&eClipboard contained characters that can't be sent." );
						return true;
					}
				}
				
				if( chatInputText.Length + text.Length > len ) {
					text = text.Substring( 0, len - chatInputText.Length );
				}
				
				if( caretPos == -1 ) {
					chatInputText.Append( chatInputText.Length, text );
				} else {
					chatInputText.Append( caretPos, text );
					caretPos += text.Length;
					if( caretPos >= chatInputText.Length ) caretPos = -1;
				}
				Dispose();
				Init();
				return true;
			} else if( key == Key.C && controlDown ) {
				if( !chatInputText.Empty ) {
					Clipboard.SetText( chatInputText.ToString() );
				}
				return true;
			}
			return false;
		}
		#endregion
	}
}