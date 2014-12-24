using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed class TextInputWidget : Widget {
		
		public TextInputWidget( Game window ) : base( window ) {
			HorizontalDocking = Docking.LeftOrTop;
			VerticalDocking = Docking.BottomOrRight;
			handlers[0] = new InputHandler( BackspaceKey, Key.BackSpace, 10 );
			handlers[1] = new InputHandler( DeleteKey, Key.Delete, 10 );
			handlers[2] = new InputHandler( LeftKey, Key.Left, 10 );
			handlers[3] = new InputHandler( RightKey, Key.Right, 10 );
			handlers[4] = new InputHandler( UpKey, Key.Up, 5 );
			handlers[5] = new InputHandler( DownKey, Key.Down, 5 );
			typingLogPos = Window.ChatInputLog.Count; // Index of newest entry + 1.
		}
		
		Texture chatInputTexture, chatCaretTexture;
		Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		int caretPos = -1;
		int typingLogPos = 0;
		public int ChatInputYOffset;
		public string chatInputText = "";
		
		public override void Render( double delta ) {
			chatInputTexture.Render( GraphicsApi );
			chatCaretTexture.Render( GraphicsApi );
			TickInput( delta );
		}

		public override void Init() {
			X = 10;
			DrawTextArgs caretArgs = new DrawTextArgs( GraphicsApi, "_", Color.White, false );
			chatCaretTexture = Utils2D.MakeTextTexture( "Arial", 12, FontStyle.Bold, 0, 0, caretArgs );
			
			if( chatInputText.Length == 0 ) {
				caretPos = -1;
			}
			Size size = Utils2D.MeasureSize( chatInputText, "Arial", 12, false );
			
			if( caretPos == -1 ) {
				chatCaretTexture.X1 = 10 + size.Width;
				size.Width += chatCaretTexture.Width;
			} else {
				Size trimmedSize = Utils2D.MeasureSize( chatInputText.Substring( 0, caretPos ), "Arial", 12, false );
				chatCaretTexture.X1 = 10 + trimmedSize.Width;
				Size charSize = Utils2D.MeasureSize( chatInputText.Substring( caretPos, 1 ), "Arial", 12, false );
				chatCaretTexture.Width = charSize.Width;
			}
			size.Height = Math.Max( size.Height, chatCaretTexture.Height );
			
			int y = Window.Height - ChatInputYOffset - size.Height / 2;
			using( Bitmap bmp = new Bitmap( size.Width, size.Height ) ) {
				using( Graphics g = Graphics.FromImage( bmp ) ) {
					Utils2D.DrawRect( g, backColour, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( GraphicsApi, chatInputText, Color.White, false );
					Utils2D.DrawText( g, "Arial", 12, args );
				}
				chatInputTexture = Utils2D.Make2DTexture( GraphicsApi, bmp, 10, y );
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
			Window.SendChat( chatInputText );
			typingLogPos = Window.ChatInputLog.Count; // Index of newest entry + 1.
			chatInputText = "";
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
					chatInputText += key;
				} else {
					chatInputText = chatInputText.Insert( caretPos, new String( key, 1 ) );
					caretPos++;
				}
				Dispose();
				Init();
				return true;
			}
			return false;
		}
		
		void BackspaceKey() {
			if( chatInputText.Length > 0 ) {
				if( caretPos == -1 ) {
					chatInputText = chatInputText.Remove( chatInputText.Length - 1, 1 );
					Dispose();
					Init();
				} else if( caretPos > 0 ) {
					caretPos--;
					chatInputText = chatInputText.Remove( caretPos, 1 );
					Dispose();
					Init();
				}
			}
		}
		
		void DeleteKey() {
			if( chatInputText.Length > 0 && caretPos != -1 ) {
				chatInputText = chatInputText.Remove( caretPos, 1 );
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void RightKey() {
			if( chatInputText.Length > 0 && caretPos != -1 ) {
				caretPos++;
				if( caretPos >= chatInputText.Length ) caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void LeftKey() {
			if( chatInputText.Length > 0 ) {
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
				chatInputText = Window.ChatInputLog[typingLogPos];
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void DownKey() {
			if( Window.ChatInputLog.Count > 0 ) {
				typingLogPos++;
				if( typingLogPos >= Window.ChatInputLog.Count ) {
					typingLogPos = Window.ChatInputLog.Count;
					chatInputText = "";
				} else {
					chatInputText = Window.ChatInputLog[typingLogPos];
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
			bool controlDown = Window.IsKeyDown( Key.LControl) || Window.IsKeyDown( Key.RControl );
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
					chatInputText += text;
				} else {
					chatInputText = chatInputText.Insert( caretPos, text );
					caretPos += text.Length;
				}
				Dispose();
				Init();
				return true;
			} else if( key == Key.C && controlDown ) {
				if( !String.IsNullOrEmpty( chatInputText ) ) {
					Clipboard.SetText( chatInputText );
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