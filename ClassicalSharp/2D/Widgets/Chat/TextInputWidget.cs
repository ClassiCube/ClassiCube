using System;
using System.Drawing;
using OpenTK.Input;
using System.Windows.Forms;

namespace ClassicalSharp {
	
	public sealed partial class TextInputWidget : Widget {
		
		const int len = 64 * 3;
		const int lines = 3;
		AltTextInputWidget altText;
		public TextInputWidget( Game game, Font font, Font boldFont ) : base( game ) {
			HorizontalAnchor = Anchor.LeftOrTop;
			VerticalAnchor = Anchor.BottomOrRight;
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			
			this.font = font;
			this.boldFont = boldFont;
			chatInputText = new WrappableStringBuffer( len );
			DrawTextArgs args = new DrawTextArgs( "_", boldFont, false );
			defaultHeight = game.Drawer2D.MeasureChatSize( ref args ).Height;
			Height = defaultHeight;
			altText = new AltTextInputWidget( game, font, boldFont, this );
			altText.Init();
		}
		
		public int RealHeight { get { return Height + altText.Height; } }
		
		Texture chatInputTexture, caretTexture;
		int caretPos = -1, typingLogPos = 0;
		public int YOffset, defaultHeight;
		internal WrappableStringBuffer chatInputText;
		readonly Font font, boldFont;
		
		static FastColour normalCaretCol = FastColour.White,
		nextCaretCol = FastColour.Yellow;
		FastColour caretCol;
		static FastColour backColour = new FastColour( 60, 60, 60, 200 );
		public override void Render( double delta ) {
			chatInputTexture.Render( graphicsApi );
			caretTexture.Render( graphicsApi, caretCol );
			if( altText.Active )
				altText.Render( delta );
		}

		string[] parts = new string[lines];
		int[] partLens = new int[lines];
		Size[] sizes = new Size[lines];
		int maxWidth = 0;
		
		public override void Init() {
			X = 5;
			DrawTextArgs args = new DrawTextArgs( "_", boldFont, false );
			caretTexture = game.Drawer2D.UseBitmappedChat ?
				game.Drawer2D.MakeBitmappedTextTexture( ref args, 0, 0 ) :
				game.Drawer2D.MakeTextTexture( ref args, 0, 0 );
			chatInputText.WordWrap( ref parts, ref partLens, 64 );
			
			maxWidth = 0;
			args = new DrawTextArgs( null, font, false );
			for( int i = 0; i < lines; i++ ) {
				args.Text = parts[i];
				sizes[i] = game.Drawer2D.MeasureChatSize( ref args );
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
				Size trimmedSize = game.Drawer2D.MeasureChatSize( ref args );
				caretTexture.X1 = 10 + trimmedSize.Width;
				
				string line = parts[indexY];
				args.Text = indexX < line.Length ? new String( line[indexX], 1 ) : " ";
				Size charSize = game.Drawer2D.MeasureChatSize( ref args );
				caretTexture.Width = charSize.Width;
				
				caretTexture.Y1 = sizes[0].Height * indexY;
				caretCol = normalCaretCol;
			}
			DrawString();
			altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
		}
		
		void DrawString() {
			int totalHeight = 0;
			for( int i = 0; i < lines; i++ )
				totalHeight += sizes[i].Height;
			Size size = new Size( maxWidth, totalHeight );
			
			int realHeight = 0;
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) ) {
				using( IDrawer2D drawer = game.Drawer2D ) {
					drawer.SetBitmap( bmp );
					DrawTextArgs args = new DrawTextArgs( null, font, false );
					
					for( int i = 0; i < parts.Length; i++ ) {
						if( parts[i] == null ) break;
						args.Text = parts[i];
						
						drawer.Clear( backColour, 0, realHeight, sizes[i].Width, sizes[i].Height );
						drawer.DrawChatText( ref args, 0, realHeight );
						realHeight += sizes[i].Height;
					}
					chatInputTexture = drawer.Make2DTexture( bmp, size, 10, 0 );
				}
			}
			
			Height = realHeight == 0 ? defaultHeight : realHeight;
			Y = game.Height - Height - YOffset;
			chatInputTexture.Y1 = Y;
			caretTexture.Y1 += Y;
			Width = size.Width;
		}

		public override void Dispose() {
			graphicsApi.DeleteTexture( ref caretTexture );
			graphicsApi.DeleteTexture( ref chatInputTexture );
		}

		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X, diffY = newY - Y;
			X = newX; Y = newY;
			caretTexture.Y1 += diffY;
			chatInputTexture.Y1 += diffY;
			
			altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
		}
		
		bool IsValidChar( char c ) {
			if( c == '&' ) return false;
			if( c >= ' ' && c <= '~' ) return true; // ascii
			
			bool isCP437 = Utils.ControlCharReplacements.IndexOf( c ) >= 0 ||
				Utils.ExtendedCharReplacements.IndexOf( c ) >= 0;
			bool supportsCP437 = game.Network.ServerSupportsFullCP437;
			return supportsCP437 && isCP437;
		}
		
		public void SendTextInBufferAndReset() {
			SendInBuffer();
			typingLogPos = game.Chat.InputLog.Count; // Index of newest entry + 1.
			chatInputText.Clear();
			caretPos = -1;
			Dispose();
			Height = defaultHeight;
			altText.SetActive( false );
		}
		
		void SendInBuffer() {
			if( chatInputText.Empty ) return;
			
			if( game.Network.ServerSupportsPatialMessages ) {
				// don't automatically word wrap the message.
				string allText = chatInputText.GetString();
				while( allText.Length > 64 ) {
					game.Chat.Send( allText.Substring( 0, 64 ), true );
					allText = allText.Substring( 64 );
				}
				game.Chat.Send( allText, false );
				return;
			}
			
			int packetsCount = 0;
			for( int i = 0; i < parts.Length; i++ ) {
				if( parts[i] == null ) break;
				packetsCount++;
			}
			// split up into both partial and final packet.
			for( int i = 0; i < packetsCount - 1; i++ ) {
				game.Chat.Send( parts[i], true );
			}
			game.Chat.Send( parts[packetsCount - 1], false );
		}
		
		public void Clear() {
			chatInputText.Clear();
			for( int i = 0; i < parts.Length; i++ ) {
				parts[i] = null;
			}
		}
		
		public void AppendText( string text ) {
			if( chatInputText.Length + text.Length > len ) {
				text = text.Substring( 0, len - chatInputText.Length );
			}
			if( text == "" ) return;
			
			if( caretPos == -1 ) {
				chatInputText.InsertAt( chatInputText.Length, text );
			} else {
				chatInputText.InsertAt( caretPos, text );
				caretPos += text.Length;
				if( caretPos >= chatInputText.Length ) caretPos = -1;
			}
			Dispose();
			Init();
		}
		
		public void AppendChar( char c ) {
			if( chatInputText.Length == len ) return;
			
			if( caretPos == -1 ) {
				chatInputText.InsertAt( chatInputText.Length, c );
			} else {
				chatInputText.InsertAt( caretPos, c );
				caretPos++;
				if( caretPos >= chatInputText.Length ) caretPos = -1;
			}
			Dispose();
			Init();
		}
	}
}