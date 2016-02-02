using System;
using System.Collections.Generic;
using System.Drawing;

namespace ClassicalSharp {
	
	public sealed partial class TextGroupWidget : Widget {
		
		public void SetText( int index, string text ) {
			graphicsApi.DeleteTexture( ref Textures[index] );
			DrawTextArgs args = new DrawTextArgs( text, font, true );
			urlBounds[index] = null;
			
			if( !String.IsNullOrEmpty( text ) ) {
				Texture tex = NextToken( text, 0 ) == -1 ? DrawSimple( ref args ) :
					DrawAdvanced( ref args, index, text );
				tex.X1 = CalcOffset( game.Width, tex.Width, XOffset, HorizontalAnchor );
				tex.Y1 = CalcY( index, tex.Height );
				Textures[index] = tex;
				lines[index] = text;
			} else {
				Textures[index] = new Texture( -1, 0, 0, 0, defaultHeight, 0, 0 );
				lines[index] = null;
			}
			UpdateDimensions();
		}
		
		Texture DrawSimple( ref DrawTextArgs args ) {
			return game.Drawer2D.MakeChatTextTexture( ref args, 0, 0 );
		}
		
		Texture DrawAdvanced( ref DrawTextArgs args, int index, string text ) {
			string[] items = Split( index, text );
			Size total = Size.Empty;
			Size[] partSizes = new Size[items.Length];
			
			for( int i = 0; i < items.Length; i++ ) {
				args.Text = items[i];
				args.Font = (i & 1) == 0 ? font : underlineFont;
				partSizes[i] = game.Drawer2D.MeasureChatSize( ref args );
				total.Height = Math.Max( partSizes[i].Height, total.Height );
				total.Width += partSizes[i].Width;
			}
			
			using( IDrawer2D drawer = game.Drawer2D )
				using( Bitmap bmp = IDrawer2D.CreatePow2Bitmap( total ) )
			{
				drawer.SetBitmap( bmp );
				int x = 0;
				
				for( int i = 0; i < items.Length; i++ ) {
					args.Text = items[i];
					args.Font = (i & 1) == 0 ? font : underlineFont;
					Size size = partSizes[i];
					
					drawer.DrawChatText( ref args, x, 0 );
					urlBounds[index][i].X = x;
					urlBounds[index][i].Width = size.Width;
					x += size.Width;
				}
				return drawer.Make2DTexture( bmp, total, 0, 0 );
			}
		}
		
		string[] Split( int index, string line ) {
			int start = 0, lastEnd = 0, count = 0;
			string[] items = new string[GetTokensCount( line )];
			Rectangle[] parts = new Rectangle[items.Length];
			
			while( (start = NextToken( line, start )) >= 0 ) {
				int nextEnd = line.IndexOf( ' ', start );
				if( nextEnd == -1 )
					nextEnd = line.Length;
				
				parts[count].Y = lastEnd << 12 | (start - lastEnd);
				items[count++] = GetPart( line, lastEnd, start ); // word bit
				parts[count].Y = start << 12 | (nextEnd - start);
				items[count++] = GetPart( line, start, nextEnd ); // url bit
				start = nextEnd;
				lastEnd = nextEnd;
			}
			
			if( lastEnd < line.Length ) {
				parts[count].Y = lastEnd << 12 | (line.Length - lastEnd);
				items[count++] = GetPart( line, lastEnd, line.Length );
			}
			urlBounds[index] = parts;
			return items;
		}
		
		string GetPart( string line, int start, int end ) {
			string part = line.Substring( start, end - start );
			int colIndex = line.LastIndexOf( '&', start, start );
			// We may split up a line into say %e<word><url>
			// url and word both need to have %e at the start.
			
			if( colIndex >= 0 && colIndex < line.Length - 1 ) {
				if( game.Drawer2D.ValidColour( line[colIndex + 1] ) )
					part = "&" + line[colIndex + 1] + part;
			}
			return part;
		}
		
		int NextToken( string line, int start ) {
			int nextHttp = line.IndexOf( "http://", start );
			int nextHttps = line.IndexOf( "https://", start );
			return nextHttp == -1 ? nextHttps : nextHttp;
		}
		
		int GetTokensCount( string line ) {
			int start = 0, lastEnd = 0, count = 0;
			while( (start = NextToken( line, start )) >= 0 ) {
				int nextEnd = line.IndexOf( ' ', start );
				if( nextEnd == -1 )
					nextEnd = line.Length;
				
				start = nextEnd;
				lastEnd = nextEnd;
				count += 2;
			}
			
			if( lastEnd < line.Length )
				count++;
			return count;
		}
	}
}