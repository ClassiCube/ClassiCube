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
			
			chatInputText = new WrappableStringBuffer( len );
			DrawTextArgs args = new DrawTextArgs( "A", boldFont, true );
			Size defSize = game.Drawer2D.MeasureChatSize( ref args );
			defaultWidth = defSize.Width; defaultHeight = defSize.Height;
			Width = defaultWidth; Height = defaultHeight;
			
			args = new DrawTextArgs( "_", boldFont, true );
			caretTex = game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
			
			this.font = font;
			this.boldFont = boldFont;
			altText = new AltTextInputWidget( game, font, boldFont, this );
			altText.Init();
		}
		
		public int RealHeight { get { return Height + altText.Height; } }
		
		Texture inputTex, caretTex;
		int caretPos = -1, typingLogPos = 0;
		public int YOffset;
		int defaultWidth, defaultHeight;
		internal WrappableStringBuffer chatInputText;
		readonly Font font, boldFont;

		FastColour caretCol;
		static FastColour backColour = new FastColour( 60, 60, 60, 200 );
		public override void Render( double delta ) {
			graphicsApi.Texturing = false;
			int y = Y, x = X;
			for( int i = 0; i < sizes.Length; i++ ) {
				int offset = (caretTex.Y1 == y) ? defaultWidth : 0;
				graphicsApi.Draw2DQuad( x + 5, y, sizes[i].Width + offset, sizes[i].Height, backColour );
				y += sizes[i].Height;
			}
			if( sizes.Length == 0 || sizes[0] == Size.Empty )
				graphicsApi.Draw2DQuad( x + 5, y, defaultWidth, defaultHeight, backColour );
			graphicsApi.Texturing = true;
			
			inputTex.Render( graphicsApi );
			caretTex.Render( graphicsApi, caretCol );
			if( altText.Active )
				altText.Render( delta );
		}

		string[] parts = new string[lines];
		int[] partLens = new int[lines];
		Size[] sizes = new Size[lines];
		int maxWidth = 0;
		int indexX, indexY;
		
		public override void Init() {
			X = 5;
			
			chatInputText.WordWrap( ref parts, ref partLens, 64 );			
			maxWidth = 0;
			DrawTextArgs args = new DrawTextArgs( null, font, true );
			for( int i = 0; i < lines; i++ ) {
				args.Text = parts[i];
				sizes[i] = game.Drawer2D.MeasureChatSize( ref args );
				maxWidth = Math.Max( maxWidth, sizes[i].Width );
			}
			
			int realIndex = caretPos;
			if( chatInputText.Empty || caretPos == -1 || caretPos >= chatInputText.Length ) {
				caretPos = -1; realIndex = 500000;
			}
			
			int sum = 0; indexX = -1; indexY = 0;
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
				caretTex.X1 = 10 + sizes[indexY].Width;
				sizes[indexY].Width += caretTex.Width;
				
				maxWidth = Math.Max( maxWidth, sizes[indexY].Width );
				caretTex.Y1 = sizes[0].Height * indexY;
				caretCol = FastColour.Yellow;
			} else {
				args.Text = parts[indexY].Substring( 0, indexX );
				Size trimmedSize = game.Drawer2D.MeasureChatSize( ref args );
				caretTex.X1 = 10 + trimmedSize.Width;
				
				string line = parts[indexY];
				args.Text = indexX < line.Length ? new String( line[indexX], 1 ) : " ";
				Size charSize = game.Drawer2D.MeasureChatSize( ref args );
				caretTex.Width = charSize.Width;
				
				caretTex.Y1 = sizes[0].Height * indexY;
				caretCol = FastColour.White;
				CalculateCaretCol();
			}
			DrawString();
			altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
			CalculateCaretCol();
		}
		
		void DrawString() {
			int totalHeight = 0;
			for( int i = 0; i < lines; i++ )
				totalHeight += sizes[i].Height;
			Size size = new Size( maxWidth, totalHeight );
			
			int realHeight = 0;
			using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( size ) )
				using( IDrawer2D drawer = game.Drawer2D )
			{
				drawer.SetBitmap( bmp );
				DrawTextArgs args = new DrawTextArgs( null, font, true );
				
				for( int i = 0; i < parts.Length; i++ ) {
					if( parts[i] == null ) break;
					args.Text = parts[i];
					
					drawer.DrawChatText( ref args, 0, realHeight );
					realHeight += sizes[i].Height;
				}
				inputTex = drawer.Make2DTexture( bmp, size, 10, 0 );
			}
			
			Height = realHeight == 0 ? defaultHeight : realHeight;
			Y = game.Height - Height - YOffset;
			inputTex.Y1 = Y;
			caretTex.Y1 += Y;
			Width = size.Width;
		}
		
		void CalculateCaretCol() {
			int x = indexX;
			for( int y = indexY; y >= 0; y-- ) {
				if( x == partLens[y] ) x = partLens[y] - 1;
				int start = parts[y].LastIndexOf( '&', x, x + 1 );
				
				int hex;
				bool validIndex = start >= 0 && start < partLens[y] - 1;
				
				if( validIndex && Utils.TryParseHex( parts[y][start + 1], out hex ) ) {
					caretCol = FastColour.GetHexEncodedCol( hex );
					return;
				}
				if( y > 0 ) x = partLens[y - 1] - 1;
			}
		}

		public override void Dispose() {		
			graphicsApi.DeleteTexture( ref inputTex );
		}
		
		public void DisposeFully() {
			Dispose();
			graphicsApi.DeleteTexture( ref caretTex );
		}

		public override void MoveTo( int newX, int newY ) {
			int diffX = newX - X, diffY = newY - Y;
			X = newX; Y = newY;
			caretTex.Y1 += diffY;
			inputTex.Y1 += diffY;
			
			altText.texture.Y1 = game.Height - (YOffset + Height + altText.texture.Height);
			altText.Y = altText.texture.Y1;
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
			string allText = chatInputText.GetString();
			game.Chat.InputLog.Add( allText );
			
			if( game.Network.ServerSupportsPatialMessages ) {
				// don't automatically word wrap the message.				
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