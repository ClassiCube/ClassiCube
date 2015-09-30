﻿using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed class TextInputWidget : Widget {
		
		public TextInputWidget( Game game, Font font, Font boldFont ) : base( game ) {
			HorizontalDocking = Docking.LeftOrTop;
			VerticalDocking = Docking.BottomOrRight;
			typingLogPos = game.ChatInputLog.Count; // Index of newest entry + 1.
			this.font = font;
			this.boldFont = boldFont;
			chatInputText = new StringBuffer( 64 );
		}
		
		Texture chatInputTexture, chatCaretTexture;
		Color backColour = Color.FromArgb( 120, 60, 60, 60 );
		int caretPos = -1;
		int typingLogPos = 0;
		public int ChatInputYOffset;
		internal StringBuffer chatInputText;
		readonly Font font, boldFont;
		
		public override void Render( double delta ) {
			chatInputTexture.Render( graphicsApi );
			chatCaretTexture.Render( graphicsApi );
		}

		public override void Init() {
			X = 10;
			DrawTextArgs caretArgs = new DrawTextArgs( "_", Color.White, false );
			chatCaretTexture = game.Drawer2D.MakeTextTexture( boldFont, 0, 0, ref caretArgs );
			string value = chatInputText.GetString();
			
			if( chatInputText.Empty ) {
				caretPos = -1;
			}
			Size size = game.Drawer2D.MeasureSize( value, font, false );
			
			if( caretPos == -1 ) {
				chatCaretTexture.X1 = 10 + size.Width;
				size.Width += chatCaretTexture.Width;
				DrawString( size, value, true );
			} else {
				string subString = chatInputText.GetSubstring( caretPos );
				Size trimmedSize = game.Drawer2D.MeasureSize( subString, font, false );
				
				chatCaretTexture.X1 = 10 + trimmedSize.Width;
				Size charSize = game.Drawer2D.MeasureSize( new String( value[caretPos], 1 ), font, false );
				chatCaretTexture.Width = charSize.Width;
				DrawString( size, value, false );
			}
		}
		
		void DrawString( Size size, string value, bool skipCheck ) {
			size.Height = Math.Max( size.Height, chatCaretTexture.Height );
			int y = game.Height - ChatInputYOffset - size.Height / 2;
			
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					drawer.DrawRect( backColour, 0, 0, bmp.Width, bmp.Height );
					DrawTextArgs args = new DrawTextArgs( value, Color.White, false );
					args.SkipPartsCheck = skipCheck;
					drawer.DrawText(  font, ref args, 0, 0 );
					chatInputTexture = drawer.Make2DTexture( bmp, size, 10, y );
				}
			}
			
			chatCaretTexture.Y1 = chatInputTexture.Y1;
			Y = y;
			Width = size.Width;
			Height = size.Height;
		}

		public override void Dispose() {
			graphicsApi.DeleteTexture( ref chatCaretTexture );
			graphicsApi.DeleteTexture( ref chatInputTexture );
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
			game.SendChat( chatInputText.ToString() );
			typingLogPos = game.ChatInputLog.Count; // Index of newest entry + 1.
			chatInputText.Clear();
			Dispose();
		}
		
		#region Input handling
		
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
			if( game.ChatInputLog.Count > 0 ) {
				typingLogPos--;
				if( typingLogPos < 0 ) typingLogPos = 0;
				chatInputText.Clear();
				chatInputText.Append( 0, game.ChatInputLog[typingLogPos] );
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		void DownKey() {
			if( game.ChatInputLog.Count > 0 ) {
				typingLogPos++;
				chatInputText.Clear();
				if( typingLogPos >= game.ChatInputLog.Count ) {
					typingLogPos = game.ChatInputLog.Count;
				} else {
					chatInputText.Append( 0, game.ChatInputLog[typingLogPos] );
				}
				caretPos = -1;
				Dispose();
				Init();
			}
		}
		
		bool OtherKey( Key key ) {
			bool controlDown = game.IsKeyDown( Key.ControlLeft ) || game.IsKeyDown( Key.ControlRight );
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