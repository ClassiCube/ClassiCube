using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed class TextInputWidget : Widget {
				
		public TextInputWidget( Game window, Font font, Font boldFont ) : base( window ) {
			HorizontalDocking = Docking.LeftOrTop;
			VerticalDocking = Docking.BottomOrRight;
			handlers[0] = new InputHandler( BackspaceKey, Key.BackSpace, 10 );
			handlers[1] = new InputHandler( DeleteKey, Key.Delete, 10 );
			handlers[2] = new InputHandler( LeftKey, Key.Left, 10 );
			handlers[3] = new InputHandler( RightKey, Key.Right, 10 );
			handlers[4] = new InputHandler( UpKey, Key.Up, 5 );
			handlers[5] = new InputHandler( DownKey, Key.Down, 5 );
			typingLogPos = Window.ChatInputLog.Count; // Index of newest entry + 1.
			this.font = font;
			this.boldFont = boldFont;
			chatInputText = new UnsafeString( 64 );
		}
		
		Texture chatInputTexture, chatCaretTexture;
		Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		int caretPos = -1;
		int typingLogPos = 0;
		public int ChatInputYOffset;
		internal UnsafeString chatInputText;
		readonly Font font, boldFont;
		
		public override void Render( double delta ) {
			chatInputTexture.Render( GraphicsApi );
			chatCaretTexture.Render( GraphicsApi );
			TickInput( delta );
		}

		public override void Init() {
			X = 10;
			DrawTextArgs caretArgs = new DrawTextArgs( GraphicsApi, "_", Color.White, false );
			chatCaretTexture = Utils2D.MakeTextTexture( boldFont, 0, 0, ref caretArgs );
			string value = chatInputText.value;
			if( Utils2D.needWinXpFix )
				value = value.TrimEnd( Utils2D.trimChars );
			
			if( chatInputText.Empty ) {
				caretPos = -1;
			}
			Size size = Utils2D.MeasureSize( value, font, false );
			
			if( caretPos == -1 ) {
				chatCaretTexture.X1 = 10 + size.Width;
				size.Width += chatCaretTexture.Width;
			} else {
				Size trimmedSize = Utils2D.MeasureSize( value.Substring( 0, caretPos ), font, false );
				chatCaretTexture.X1 = 10 + trimmedSize.Width;
				Size charSize = Utils2D.MeasureSize( value.Substring( caretPos, 1 ), font, false );
				chatCaretTexture.Width = charSize.Width;
			}
			size.Height = Math.Max( size.Height, chatCaretTexture.Height );
			
			int y = Window.Height - ChatInputYOffset - size.Height / 2;
			using( Bitmap bmp = Utils2D.CreatePow2Bitmap( size ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, 0, 0, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( GraphicsApi, value, Color.White, false );
					Utils2D.DrawText( g, font, ref args, 0, 0 );
				}
				chatInputTexture = Utils2D.Make2DTexture( GraphicsApi, bmp, size, 10, y );
			}
			chatCaretTexture.Y1 = chatInputTexture.Y1;
			Y = y;
			Width = size.Width;
			Width = size.Height;
		}

		public override void Dispose() {
			GraphicsApi.DeleteTexture( ref chatCaretTexture );
			GraphicsApi.DeleteTexture( ref chatInputTexture );
		}

		public override void MoveTo( int newX, int newY ) {
			int deltaX = newX - X;
			int deltaY = newY - Y;
			X = newX;
			Y = newY;
			chatCaretTexture.Y1 += deltaY;
			chatInputTexture.Y1 += deltaY;
		}
		
		static bool IsInvalidChar( char c ) {
			// Make sure we're in the printable text range from 0x20 to 0x7E
			return c < ' ' || c == '&' || c > '~';
		}
		
		public void SendTextInBufferAndReset() {
			Window.SendChat( chatInputText.ToString() );
			typingLogPos = Window.ChatInputLog.Count; // Index of newest entry + 1.
			chatInputText.Clear();
			Dispose();
		}
		
		#region Input handling
		InputHandler[] handlers = new InputHandler[6];
		
		class InputHandler {
			public Action KeyFunction;
			public DateTime LastDown;
			public Key AssociatedKey;
			public double accumulator, period;
			
			public InputHandler( Action func, Key key, int frequency ) {
				KeyFunction = func;
				AssociatedKey = key;
				LastDown = DateTime.MinValue;
				period = 1.0 / frequency;
			}
			
			public bool HandlesKeyDown( Key key ) {
				if( key != AssociatedKey ) return false;
				LastDown = DateTime.UtcNow;
				KeyFunction();
				return true;
			}
			
			public void KeyTick( Game window ) {
				if( LastDown == DateTime.MinValue ) return;
				if( window.IsKeyDown( AssociatedKey ) &&
				   ( DateTime.UtcNow - LastDown ).TotalSeconds >= period ) {
					KeyFunction();
				}
			}
			
			public bool HandlesKeyUp( Key key ) {
				if( key != AssociatedKey ) return false;
				LastDown = DateTime.MinValue;
				return true;
			}
		}
		
		void TickInput( double delta ) {
			for( int i = 0; i < handlers.Length; i++ ) {
				InputHandler handler = handlers[i];
				handler.accumulator += delta;
				while( handler.accumulator > handler.period ) {
					handler.KeyTick( Window );
					handler.accumulator -= handler.period;
				}
			}
		}
		
		public override bool HandlesKeyPress( char key ) {
			if( chatInputText.Length < 64 && !IsInvalidChar( key ) ) {
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
		
		void BackspaceKey() {
			if( !chatInputText.Empty ) {
				if( caretPos == -1 ) {
					chatInputText.DeleteAt( chatInputText.Length - 1 );
					Dispose();
					Init();
				} else if( caretPos > 0 ) {
					caretPos--;
					chatInputText.DeleteAt( caretPos );
					Dispose();
					Init();
				}
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
			if( Window.ChatInputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				chatInputText.Clear();
				chatInputText.Append( 0, Window.ChatInputLog[typingLogPos] );
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void DownKey() {
			if( Window.ChatInputLog.Count > 0 ) {
				typingLogPos++;
				chatInputText.Clear();
				if( typingLogPos >= Window.ChatInputLog.Count ) {
					typingLogPos = Window.ChatInputLog.Count;
				} else {
					chatInputText.Append( 0, Window.ChatInputLog[typingLogPos] );
				}
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		public override bool HandlesKeyDown( Key key ) {
			for( int i = 0; i < handlers.Length; i++ ) {
				if( handlers[i].HandlesKeyDown( key ) ) return true;
			}
			bool controlDown = Window.IsKeyDown( Key.ControlLeft ) || Window.IsKeyDown( Key.ControlRight );
			if( key == Key.V && controlDown && chatInputText.Length < 64 ) {
				string text = Clipboard.GetText();
				if( String.IsNullOrEmpty( text ) ) return true;
				
				for( int i = 0; i < text.Length; i++ ) {
					if( IsInvalidChar( text[i] ) ) {
						Utils.LogWarning( "Clipboard text contained characters that can't be sent." );
						return true;
					}
				}
				
				if( chatInputText.Length + text.Length > 64 ) {
					text = text.Substring( 0, 64 - chatInputText.Length );
				}
				
				if( caretPos == -1 ) {
					chatInputText.Append( chatInputText.Length, text );
				} else {
					chatInputText.Append( caretPos, text );
					caretPos += text.Length;
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
		
		public override bool HandlesKeyUp( Key key ) {
			for( int i = 0; i < handlers.Length; i++ ) {
				if( handlers[i].HandlesKeyUp( key ) ) return true;
			}
			return false;
		}
		
		#endregion
	}
}